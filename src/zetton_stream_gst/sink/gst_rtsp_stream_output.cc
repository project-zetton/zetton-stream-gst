#include "zetton_stream_gst/sink/gst_rtsp_stream_output.h"

#include <fmt/core.h>

#include <memory>
#include <string>

#include "zetton_stream/base/stream_options.h"
#include "zetton_stream_gst/util/gst_loop.h"
#include "zetton_stream_gst/util/gst_pipeline.h"

namespace zetton {
namespace stream {

/* this function is periodically run to clean up the expired sessions from the
 * pool. */
static gboolean session_cleanup(GstRtspStreamOutput *nodelet,
                                gboolean ignored) {
  GstRTSPServer *server = nodelet->GetRtspServer();
  GstRTSPSessionPool *pool;
  int num;

  pool = gst_rtsp_server_get_session_pool(server);
  num = gst_rtsp_session_pool_cleanup(pool);
  g_object_unref(pool);

  if (num > 0) {
    std::cout << "Sessions cleaned: " << num << std::endl;
  }

  return TRUE;
}

static void client_options(GstRTSPClient *client, GstRTSPContext *state,
                           GstRtspStreamOutput *nodelet) {
  if (state->uri) {
    nodelet->url_connected(state->uri->abspath);
  }
}

static void client_teardown(GstRTSPClient *client, GstRTSPContext *state,
                            GstRtspStreamOutput *nodelet) {
  if (state->uri) {
    nodelet->url_disconnected(state->uri->abspath);
  }
}

static void new_client(GstRTSPServer *server, GstRTSPClient *client,
                       GstRtspStreamOutput *nodelet) {
  std::cout << "New RTSP client" << std::endl;
  g_signal_connect(client, "options-request", G_CALLBACK(client_options),
                   nodelet);
  g_signal_connect(client, "teardown-request", G_CALLBACK(client_teardown),
                   nodelet);
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media,
                            GstElement **appsrc) {
  if (appsrc) {
    GstElement *pipeline = gst_rtsp_media_get_element(media);

    *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "src");

    /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg(G_OBJECT(*appsrc), "format", "time");

    gst_object_unref(pipeline);
  } else {
    guint i, n_streams;
    n_streams = gst_rtsp_media_n_streams(media);

    for (i = 0; i < n_streams; i++) {
      GstRTSPAddressPool *pool;
      GstRTSPStream *stream;
      gchar *min, *max;

      stream = gst_rtsp_media_get_stream(media, i);

      /* make a new address pool */
      pool = gst_rtsp_address_pool_new();

      min = g_strdup_printf("224.3.0.%d", (2 * i) + 1);
      max = g_strdup_printf("224.3.0.%d", (2 * i) + 2);
      gst_rtsp_address_pool_add_range(pool, min, max, 5000 + (10 * i),
                                      5010 + (10 * i), 1);
      g_free(min);
      g_free(max);

      gst_rtsp_stream_set_address_pool(stream, pool);
      g_object_unref(pool);
    }
  }
}

const char *GstRtspStreamOutput::SupportedExtensions[] = {
    "mkv", "mp4", "qt", "flv", "avi", "h264", "h265", NULL};

bool GstRtspStreamOutput::IsSupportedExtension(const char *ext) {
  return IsGstSupportedExtension(ext, SupportedExtensions);
}

bool GstRtspStreamOutput::Init(const StreamOptions &options) {
  options_ = options;
  auto ret = Open();
  return ret;
}

GstRtspStreamOutput::~GstRtspStreamOutput() { Close(); }

bool GstRtspStreamOutput::Open() {
  // initialize GStreamer
  if (!gst_is_initialized()) {
    gst_init(nullptr, nullptr);
  }

  // get rtsp mountpoint
  if (!options_.resource.location.empty()) {
    mountpoint_ = options_.resource.mountpoint;
  }

  // initialize rtsp server
  rtsp_server_ = create_rtsp_server();

  // setup the full pipeline
  if (BuildPipelineString()) {
    num_of_clients_[mountpoint_] = 0;
    appsrc_[mountpoint_] = nullptr;
    std::cout << "Gstreamer version: " << gst_version_string() << std::endl;
    std::cout << "RTSP url: rtsp://127.0.0.1:8554" << mountpoint_ << std::endl;
    std::cout << "RTSP pipeline:\n" << pipeline_string_ << std::endl;

    // add the pipeline to the rtsp server
    setup_rtsp_server(mountpoint_.c_str(), pipeline_string_.c_str(),
                      (GstElement **)&(appsrc_[mountpoint_]));
    return true;
  }

  return false;
}

void GstRtspStreamOutput::Close() { Stop(); }

bool GstRtspStreamOutput::Start() {
#if 1
  // Create the main loop
  loop_ = g_main_loop_new(nullptr, false);

  // Run the loop in a separate thread
  g_main_loop_run(loop_);
#else
  start_gst_main_loop(loop_, loop_thread_);
#endif

  return true;
}

void GstRtspStreamOutput::Stop() {
  // but if pipeline not created appsrc will be deleted here
  for (std::pair<std::string, GstAppSrc *> element : appsrc_) {
    if (element.second != nullptr) {
      gst_object_unref(GST_OBJECT(element.second));
      element.second = nullptr;
    }
  }

  if (loop_ != nullptr) {
    g_main_loop_quit(loop_);
    if (loop_thread_.joinable()) loop_thread_.join();
  }
}

bool GstRtspStreamOutput::Render(const cv::Mat &frame) {
  GstBuffer *buf;
  GstCaps *caps;
  // char *gst_type, *gst_format = (char *)"";
  // g_print("Image encoding: %s\n", msg->encoding.c_str());
  if (appsrc_[mountpoint_] != nullptr && subs_[mountpoint_]) {
    // Set caps from message
    caps = gst_caps_new_from_image(frame);
    gst_app_src_set_caps(appsrc_[mountpoint_], caps);

    size_t sizeInBytes = frame.total() * frame.elemSize();
    //    size_t sizeInBytes = frame.step[0] * frame.rows;
    buf = gst_buffer_new_allocate(nullptr, sizeInBytes, nullptr);
    gst_buffer_fill(buf, 0, frame.data, sizeInBytes);
    GST_BUFFER_FLAG_SET(buf, GST_BUFFER_FLAG_LIVE);

    gst_app_src_push_buffer(appsrc_[mountpoint_], buf);
    return true;
  }
  return false;
}

bool GstRtspStreamOutput::Render(void *image, uint32_t width, uint32_t height) {
  cv::Mat frame(height, width, CV_8UC3, static_cast<uint8_t *>(image));
  if (!frame.empty()) {
    auto ret = Render(frame);
    return ret;
  }
  return false;
}

void GstRtspStreamOutput::SetStatus(const char *str) {}

bool GstRtspStreamOutput::BuildPipelineString() {
  // Build different parts of the pipeline
  std::string source, caps, encoding, mux;
  if (!BuildGstPipelineSource(options_, source,
                              StreamProtocolType::PROTOCOL_APPSRC)) {
    AERROR_F("Failed to build pipeline head");
    return false;
  }
  if (!BuildGstPipelineCaps(options_, caps)) {
    AERROR_F("Failed to build pipeline caps");
    return false;
  }
  if (!BuildGstPipelineEncoding(options_, encoding)) {
    AERROR_F("Failed to build pipeline encoding");
    return false;
  }
  if (!BuildGstPipelineMuxing(options_, mux)) {
    AERROR_F("Failed to build pipeline tail");
    return false;
  }

  // Combine the pipeline
  pipeline_string_ = fmt::format("( {} ! videoconvert ! videoscale {} {} )",
                                 source, encoding, mux);

  return true;
}

GstRTSPServer *GstRtspStreamOutput::create_rtsp_server() {
  GstRTSPServer *server;
  /* create a server instance */
  server = gst_rtsp_server_new();
  /* attach the server to the default main context */
  gst_rtsp_server_attach(server, nullptr);
  g_signal_connect(server, "client-connected", G_CALLBACK(new_client), this);
  /* add a timeout for the session cleanup */
  g_timeout_add_seconds(2, (GSourceFunc)session_cleanup, this);

  return server;
}

void GstRtspStreamOutput::setup_rtsp_server(const char *url,
                                            const char *sPipeline,
                                            GstElement **appsrc) {
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory;

  /* get the mount points for this server, every server has a default object
   * that be used to map uri mount points to media factories */
  mounts = gst_rtsp_server_get_mount_points(rtsp_server_);

  /* make a media factory for a test stream. The default media factory can use
   * gst-launch syntax to create pipelines.
   * any launch line works as long as it contains elements named pay%d. Each
   * element with pay%d names will be a stream */
  factory = gst_rtsp_media_factory_new();
  gst_rtsp_media_factory_set_launch(factory, sPipeline);

  /* notify when our media is ready, This is called whenever someone asks for
   * the media and a new pipeline is created */
  g_signal_connect(factory, "media-configure", (GCallback)media_configure,
                   appsrc);

  gst_rtsp_media_factory_set_shared(factory, TRUE);

  /* attach the factory to the url */
  gst_rtsp_mount_points_add_factory(mounts, url, factory);

  /* don't need the ref to the mounts anymore */
  g_object_unref(mounts);
}

/* Modified from
 * https://github.com/ProjectArtemis/gst_video_server/blob/master/src/server_nodelet.cpp
 */
GstCaps *GstRtspStreamOutput::gst_caps_new_from_image(const cv::Mat &frame) {
  return gst_caps_new_simple(
      "video/x-raw", "format", G_TYPE_STRING,
      StreamPixelFormatToStr(options_.pixel_format), "width", G_TYPE_INT,
      frame.cols, "height", G_TYPE_INT, frame.rows, "framerate",
      GST_TYPE_FRACTION, static_cast<int>(options_.frame_rate), 1, nullptr);
}

void GstRtspStreamOutput::url_connected(const std::string &url) {
  std::cout << "Client connected: " << url << std::endl;
  // Check which stream the client has connected to
  if (url == mountpoint_) {
    subs_[url] = true;
    num_of_clients_[url]++;
  }
}

void GstRtspStreamOutput::url_disconnected(const std::string &url) {
  std::cout << "Client disconnected: " << url << std::endl;
  // Check which stream the client has disconnected from
  if (url == mountpoint_) {
    if (num_of_clients_[url] > 0) num_of_clients_[url]--;
    if (num_of_clients_[url] == 0) {
      // No-one else is connected.
      subs_[url] = false;
      appsrc_[url] = nullptr;
    }
  }
}

}  // namespace stream
}  // namespace zetton
