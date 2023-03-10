#pragma once

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <mutex>
#include <opencv2/opencv.hpp>
#include <queue>
#include <thread>

#include "zetton_stream/interface/base_stream_sink.h"

namespace zetton {
namespace stream {

class GstStreamOutput : public BaseStreamSink {
 public:
  ~GstStreamOutput() override;

  bool Init(const StreamOptions& options) override;
  bool Open() override;
  void Close() override;

  /// \brief Function to start streaming video to the GStreamer pipeline
  /// \details This function creates the GStreamer pipeline, starts the
  /// pipeline, registers a callback function to push data to appsrc, and
  /// starts the main loop.
  /// \note This function should be called after the pipeline string has been
  /// built
  /// \return true if successful, false otherwise
  bool Start();

  void Stop();

  bool Render(void* image, uint32_t width, uint32_t height) override;
  bool Render(const cv::Mat& frame);

  void SetStatus(const char* str) override;

 protected:
  bool BuildPipelineString();

  /// \brief Function to push data to appsrc
  /// \note This function is called when the appsrc needs data, we add an idle
  /// handler to the mainloop to push the data into the appsrc
  /// \param src Pointer to appsrc element
  /// \param size Size of the buffer to push
  /// \param user_data User data passed to this function
  /// \return gboolean Returns TRUE if data was pushed successfully, FALSE
  /// otherwise
  static gboolean PushDataToAppSrc(GstAppSrc* src, guint size,
                                   gpointer user_data);

  void StartGstreamerPipeline();

  /// \brief Function to finalize and clean up
  /// \note This function should be called after all threads have been joined
  void StopGstreamerPipeline();

 protected:
  /// \brief Flag indicating whether GStreamer has been initialized
  bool initialized_ = false;
  /// \brief GStreamer pipeline string
  std::string pipeline_sink_;

  /// \brief Mutex to protect the queue of captured frames
  std::mutex queue_mutex_;
  /// \brief Queue of captured frames
  std::queue<void*> frame_queue_;

  /// \brief Pointer to appsrc element
  GstAppSrc* source_;
  /// \brief Pointer to the entire GStreamer pipeline
  GstElement* pipeline_;
  /// \brief Pointer to GMainLoop
  GMainLoop* loop_;
  /// \brief Pointer to GStreamer bus
  GstBus* bus_;
  /// \brief Thread for GStreamer mainloop
  std::thread loop_thread_;
};

}  // namespace stream
}  // namespace zetton
