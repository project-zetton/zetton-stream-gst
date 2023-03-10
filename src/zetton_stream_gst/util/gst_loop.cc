#include "zetton_stream_gst/util/gst_loop.h"

namespace zetton {
namespace stream {

void start_gst_main_loop(GMainLoop* loop, std::thread& loop_thread) {
  if (!gst_is_initialized()) {
    //    std::cout << "Initializing gstreamer" << std::endl;
    gst_init(nullptr, nullptr);
  }

  // create and run main event loop
  loop = g_main_loop_new(nullptr, FALSE);
  g_assert(loop);
  loop_thread = std::thread([&]() {
    //    std::cout << "GMainLoop started." << std::endl;
    // blocking
    g_main_loop_run(loop);
    // terminated!
    g_main_loop_unref(loop);
    loop = nullptr;
    //    std::cout << "GMainLoop terminated." << std::endl;
  });
  // may cause a problem on non Linux, but i'm not care.
  pthread_setname_np(loop_thread.native_handle(), "g_main_loop_run");
}

}  // namespace stream
}  // namespace zetton
