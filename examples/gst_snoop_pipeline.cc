#include <opencv2/opencv.hpp>
#include <thread>

#include "zetton_stream_gst/sink/gst_snoop_output.h"

int main(int argc, char *argv[]) {
  // 0. prepare
  // 0.1. init gstreamer
  gst_init(&argc, &argv);

  // 0.2. init snoop pipe
  std::string pipeline_src =
      "videotestsrc num-buffers=600 ! "
      "video/x-raw,width=(int)640,height=(int)480,"
      "format=(string)RGB,framerate=(fraction)30/1 ! queue ! appsink "
      "name=testsink";
  // use fakesink instead of fpsdisplaysink to run on headless server
  std::string pipeline_sink =
      "appsrc name=testsource caps=video/x-raw,width=(int)640,height=(int)480,"
      "format=(string)RGB,framerate=(fraction)30/1 ! queue ! videoconvert ! "
      "fakesink";
  GstSnoopOutput pipe(pipeline_src, pipeline_sink);
  pipe.Init();

  // 1. register callback
  // pipe.RegisterCallback([](GstMapInfo *map) {
  //   g_print("%lu\n", map->size);
  // });
  pipe.RegisterCallback([](GstMapInfo *map) {
    // on screen display
    cv::Mat frame(480, 640, CV_8UC3, (char *)map->data, cv::Mat::AUTO_STEP);
    cv::line(frame, cv::Point(0, 240), cv::Point(640, 240),
             cv::Scalar(0, 255, 0), 2);
    cv::putText(frame, "Hello World", cv::Point(10, 30),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

    // do canny to simulate a time consuming operation
    cv::Mat src_gray, detected_edges, dst;
    cv::cvtColor(frame, src_gray, cv::COLOR_BGR2GRAY);
    cv::blur(src_gray, detected_edges, cv::Size(3, 3));
    cv::Canny(detected_edges, detected_edges, 0, 0, 3,
              true);  // true: use L2 norm
  });

  // 2. run pipeline
  // 2.1. start pipeline
  std::cout << ">>> start pipeline" << std::endl;
  pipe.Start();

  // 2.2. sleep 10 seconds
  std::cout << ">>> sleep" << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(10));

  // 2.3. stop pipeline
  std::cout << ">>> stop pipeline" << std::endl;
  pipe.Stop();

  return 0;
}
