#include "zetton_stream_gst/sink/gst_snoop_output.h"

#include <functional>
#include <utility>

GstSnoopOutput::GstSnoopOutput(std::string pipeline_src,
                               std::string pipeline_sink)
    : loop_(nullptr),
      src_(nullptr),
      sink_(nullptr),
      pipeline_src_(std::move(pipeline_src)),
      pipeline_sink_(std::move(pipeline_sink)) {}

GstSnoopOutput::~GstSnoopOutput() { Stop(); }

bool GstSnoopOutput::Init() {
  // Initializes the GMainLoop
  loop_ = g_main_loop_new(NULL, FALSE);

  // Setting up source pipeline, we read from a file and convert to our desired
  gchar *string = NULL;
  string = g_strdup_printf("%s", pipeline_src_.c_str());
  src_ = gst_parse_launch(string, NULL);
  g_free(string);

  // If source pipeline is faulty, unref loop and return false
  if (src_ == NULL) {
    g_print("Bad source\n");
    g_main_loop_unref(loop_);
    return false;
  }

  g_print("Capture bin launched\n");

  // to be notified of messages from this pipeline, mostly EOS
  GstBus *bus = NULL;
  bus = gst_element_get_bus(src_);
  gst_bus_add_watch(bus, (GstBusFunc)OnSourceMessage, this);
  gst_object_unref(bus);

  // we use appsink in push mode, it sends us a signal when data is available
  // and we pull out the data in the signal callback.
  GstElement *testsink = NULL;
  testsink = gst_bin_get_by_name(GST_BIN(src_), "testsink");
  g_object_set(G_OBJECT(testsink), "emit-signals", TRUE, "sync", FALSE, NULL);
  g_signal_connect(testsink, "new-sample", G_CALLBACK(OnNewSampleFromSink),
                   this);
  gst_object_unref(testsink);

  // setting up sink pipeline, we push video data into this pipeline that will
  // then copy in file using the default filesink. We have no blocking
  // behaviour on the src which means that we will push the entire file into
  // memory.
  string = g_strdup_printf("%s", pipeline_sink_.c_str());
  sink_ = gst_parse_launch(string, NULL);
  g_free(string);

  // If sink pipeline is faulty, unref source pipeline and loop and return false
  if (sink_ == NULL) {
    g_print("Bad sink\n");
    gst_object_unref(src_);
    g_main_loop_unref(loop_);
    return false;
  }
  g_print("Play bin launched\n");

  // Obtains the testsource element by name from the bin
  GstElement *testsource = NULL;
  testsource = gst_bin_get_by_name(GST_BIN(sink_), "testsource");
  // configures the source to push data in time-based format
  g_object_set(testsource, "format", GST_FORMAT_TIME, NULL);
  // uncomment this line to block when appsrc has buffered enough
  g_object_set(testsource, "block", TRUE, NULL);
  gst_object_unref(testsource);

  // Adds a watch on the bus to receive messages from the sink pipeline, mostly
  // EOS
  bus = gst_element_get_bus(sink_);
  gst_bus_add_watch(bus, (GstBusFunc)OnSinkMessage, this);
  gst_object_unref(bus);

  g_print("Going to set state to play\n");

  // Returns true upon successful initialization
  return true;
}

void GstSnoopOutput::Start() {
  // launch the sink pipeline first, this will block until the source pipeline
  // goes to PLAYING
  gst_element_set_state(sink_, GST_STATE_PLAYING);
  gst_element_set_state(src_, GST_STATE_PLAYING);

  // let's run, this loop will quit when the sink pipeline goes EOS or when an
  // error occurs in the source or sink pipelines.
  g_print("Let's run!\n");
  g_main_loop_run(loop_);
  g_print("Going out\n");
}

void GstSnoopOutput::Stop() {
  // set the source and sink pipelines to NULL
  gst_element_set_state(src_, GST_STATE_NULL);
  gst_element_set_state(sink_, GST_STATE_NULL);

  if (src_) {
    // unref the source pipeline
    gst_object_unref(src_);
  }
  if (sink_) {
    // unref the sink pipeline
    gst_object_unref(sink_);
  }
  if (loop_) {
    // unref the loop
    g_main_loop_unref(loop_);
  }
}

void GstSnoopOutput::RegisterCallback(
    std::function<void(GstMapInfo *)> callback) {
  // Register callback function
  callback_ = std::move(callback);
}

GstFlowReturn GstSnoopOutput::ModifyInData(GstMapInfo *map) {
  // dummy function to modify data
  gsize dataLength;
  guint8 *rdata;

  dataLength = map->size;
  rdata = map->data;
  g_print("%s dataLen = %lu\n", __func__, dataLength);

  // Modify half of frame to plane white
  // for (gsize i = 0; i <= dataLength / 2; i++) {
  //   rdata[i] = 0xff;
  // }

  return GST_FLOW_OK;
}

GstFlowReturn GstSnoopOutput::OnNewSampleFromSink(GstElement *elt,
                                                  GstSnoopOutput *user_data) {
  GstSample *sample;
  GstBuffer *app_buffer, *buffer;
  GstElement *source;
  GstFlowReturn ret;
  GstMapInfo map;
  //   guint8 *rdata;
  //   int dataLength;
  //   int i;
  g_print("%s\n", __func__);

  // get the sample from appsink
  sample = gst_app_sink_pull_sample(GST_APP_SINK(elt));
  buffer = gst_sample_get_buffer(sample);

  // copy the buffer to a new buffer
  app_buffer = gst_buffer_copy_deep(buffer);
  // map the buffer
  gst_buffer_map(app_buffer, &map, GST_MAP_WRITE);

  // modify the buffer data
  if (user_data->callback_) {
    user_data->callback_(&map);
  } else {
    user_data->ModifyInData(&map);
  }

  // get the source element by name from the bin
  source = gst_bin_get_by_name(GST_BIN(user_data->sink_), "testsource");
  // push the buffer into the appsrc
  ret = gst_app_src_push_buffer(GST_APP_SRC(source), app_buffer);

  // unref the sample and the buffer
  gst_sample_unref(sample);
  gst_buffer_unmap(app_buffer, &map);
  gst_object_unref(source);

  return ret;
}

gboolean GstSnoopOutput::OnSourceMessage(GstBus *bus, GstMessage *message,
                                         GstSnoopOutput *user_data) {
  GstElement *source;
  g_print("%s\n", __func__);
  g_print("%s\n", GST_MESSAGE_TYPE_NAME(message));

  // Process messages from the source pipeline
  switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
      // EOS from the source pipeline, this means we need to push EOS into the
      // sink pipeline
      g_print("The source got dry\n");
      source = gst_bin_get_by_name(GST_BIN(user_data->sink_), "testsource");
      gst_app_src_end_of_stream(GST_APP_SRC(source));
      gst_object_unref(source);
      break;
    case GST_MESSAGE_ERROR:
      // Error from the source pipeline, quit the loop
      g_print("Received error\n");
      g_main_loop_quit(user_data->loop_);
      break;
    default:
      break;
  }
  return TRUE;
}

gboolean GstSnoopOutput::OnSinkMessage(GstBus *bus, GstMessage *message,
                                       GstSnoopOutput *user_data) {
  // Process messages from the sink pipeline
  g_print("%s\n", __func__);
  switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
      // EOS from the sink pipeline, quit the loop
      g_print("Finished playback\n");
      g_main_loop_quit(user_data->loop_);
      break;
    case GST_MESSAGE_ERROR:
      // Error from the sink pipeline, quit the loop
      g_print("Received error\n");
      g_main_loop_quit(user_data->loop_);
      break;
    default:
      break;
  }
  return TRUE;
}
