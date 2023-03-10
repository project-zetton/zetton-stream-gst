#include <gst/gst.h>

#include <thread>

namespace zetton {
namespace stream {

/// \brief Initialize GStreamer and streaming thread
/// \param loop GMainLoop instance
/// \param loop_thread thread of GMainLoop
void start_gst_main_loop(GMainLoop* loop, std::thread& loop_thread);

}  // namespace stream
}  // namespace zetton
