#include "zetton_stream_gst/sink/gst_stream_output.h"

#include <zetton_stream/base/stream_uri.h>

#include <cstdint>

#include "zetton_common/util/log.h"
#include "zetton_stream/util/ocv.h"
#include "zetton_stream_gst/util/gst_loop.h"
#include "zetton_stream_gst/util/gst_pipeline.h"

namespace zetton {
namespace stream {

bool GstStreamOutput::Init(const StreamOptions& options) {
  options_ = options;
  auto ret = Open();
  return ret;
}

GstStreamOutput::~GstStreamOutput() { Close(); }

bool GstStreamOutput::Open() {
  // setup the full pipeline
  auto ret = BuildPipelineString();
  if (!ret) {
    AERROR_F("Failed to build pipeline string");
    return false;
  }
  AINFO_F("GStreamer pipeline: {}", pipeline_sink_);

  // Initialize GStreamer if not yet
  if (!initialized_) {
    // Initialize GStreamer
    if (!gst_is_initialized()) {
      gst_init(nullptr, nullptr);
    }

    // Create the elements
    pipeline_ = gst_parse_launch(pipeline_sink_.c_str(), nullptr);

    // Check pipeline
    if (!pipeline_) {
      std::cerr << "pipeline is null" << std::endl;
      return false;
    }

    // Get appsrc
    // FIXME: do not use hard-coded name
    source_ = GST_APP_SRC(gst_bin_get_by_name(GST_BIN(pipeline_), "src"));

    // Start the pipeline
    gst_element_set_state(pipeline_, GST_STATE_PLAYING);

    // Set callback for appsrc
    g_signal_connect(source_, "need-data", G_CALLBACK(PushDataToAppSrc), this);

    // Print out current pipeline
    // gst_debug_bin_to_dot_file(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
    //                           "pipeline");

    initialized_ = true;
  }

  return true;
}

void GstStreamOutput::Close() { Stop(); }

bool GstStreamOutput::Start() {
#if 1
  // Create the main loop
  loop_ = g_main_loop_new(nullptr, false);

  // Set up the bus
  // bus_ = gst_element_get_bus(pipeline_);
  // gst_bus_add_watch(
  //     bus_,
  //     [](GstBus* bus, GstMessage* message, gpointer loop_ptr) -> gboolean {
  //       GMainLoop* loop = static_cast<GMainLoop*>(loop_ptr);
  //       switch (GST_MESSAGE_TYPE(message)) {
  //         case GST_MESSAGE_ERROR:
  //         case GST_MESSAGE_EOS:
  //           g_main_loop_quit(loop);
  //           break;
  //         default:
  //           break;
  //       }
  //       return true;
  //     },
  //     loop_);

  // Run the loop in a separate thread
  g_main_loop_run(loop_);
#else
  start_gst_main_loop(loop_, loop_thread_);
#endif

  return true;
}

void GstStreamOutput::Stop() {
  // Set pipeline to "null" state
  if (pipeline_ != nullptr) {
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
  }

  // Destroy main loop
  if (loop_ != nullptr) {
    g_main_loop_quit(loop_);
    if (loop_thread_.joinable()) loop_thread_.join();
  }
}

bool GstStreamOutput::Render(void* image, uint32_t width, uint32_t height) {
  // Check image
  if (!image) {
    AERROR_F("image is empty");
    return false;
  } else if (width != options_.width || height != options_.height) {
    AERROR_F("image size is not matched with options: {}x{} vs {}x{}", width,
             height, options_.width, options_.height);
    return false;
  }

  // Push captured image into queue
  queue_mutex_.lock();
  frame_queue_.push(image);
  queue_mutex_.unlock();

  return true;
}

bool GstStreamOutput::Render(const cv::Mat& frame) {
  // Check frame
  if (!CheckFrame(frame, options_)) {
    AERROR_F("frame is invalid");
    return false;
  }

  // Push captured frame into queue
  queue_mutex_.lock();
  frame_queue_.push(frame.data);
  queue_mutex_.unlock();

  return true;
}

void GstStreamOutput::SetStatus(const char* str) {}

bool GstStreamOutput::BuildPipelineString() {
  // Build different parts of the pipeline
  std::string source, caps, encoding, mux, sink;
  if (!BuildGstPipelineSource(options_, source,
                              StreamProtocolType::PROTOCOL_APPSRC)) {
    AERROR_F("Failed to build pipeline head");
    return false;
  }
  if (!BuildGstPipelineCaps(options_, caps,
                            StreamProtocolType::PROTOCOL_APPSRC)) {
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
  if (!BuildGstPipelineSink(options_, sink)) {
    AERROR_F("Failed to build pipeline sink");
    return false;
  }

  // Combine the pipeline
  pipeline_sink_ = fmt::format("( {} {} ! videoscale ! videoconvert {} {} {} )",
                               source, caps, encoding, mux, sink);

  return true;
}

gboolean GstStreamOutput::PushDataToAppSrc(GstAppSrc* src, guint size,
                                           gpointer user_data) {
  void* frame = nullptr;

  auto self = static_cast<GstStreamOutput*>(user_data);

  // Wait until the queue is not empty
  while (self->frame_queue_.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(
        self->options_.frame_rate > 0
            ? static_cast<int>(1000 / self->options_.frame_rate)
            : 100));
  }

  // Get a frame from the queue
  self->queue_mutex_.lock();
  if (!self->frame_queue_.empty()) {
    frame = self->frame_queue_.front();
    self->frame_queue_.pop();
  }
  self->queue_mutex_.unlock();

  // Check if the frame is empty
  if (!frame) {
    AERROR_F("frame is empty");
    return false;
  }

  // Convert the frame to a GStreamer buffer
  GstBuffer* buffer;
  GstMapInfo map;
  buffer = gst_buffer_new_allocate(
      nullptr,
      self->options_.width * self->options_.height * self->options_.channels,
      nullptr);
  gst_buffer_map(buffer, &map, GST_MAP_WRITE);
  memcpy(
      map.data, frame,
      self->options_.width * self->options_.height * self->options_.channels);
  gst_buffer_unmap(buffer, &map);

  // Push the buffer to the pipeline
  GstFlowReturn ret;
  g_signal_emit_by_name(src, "push-buffer", buffer, &ret);

  // Free the buffer
  gst_buffer_unref(buffer);

  return true;
}

void GstStreamOutput::StartGstreamerPipeline() {}

void GstStreamOutput::StopGstreamerPipeline() {}

}  // namespace stream
}  // namespace zetton
