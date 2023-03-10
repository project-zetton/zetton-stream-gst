#include <unistd.h>

#include <thread>

#include "zetton_common/util/log.h"
#include "zetton_stream/base/stream_options.h"
#include "zetton_stream/base/stream_uri.h"
#include "zetton_stream_gst/sink/gst_stream_output.h"

void generate_smpte_with_snow(cv::Mat& image) {
  int height = image.rows;
  int width = image.cols;

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      cv::Vec3b& pixel = image.at<cv::Vec3b>(i, j);
      // Set pixel color based on the position in the image
      if (i < height / 7) {
        // White bar
        pixel = cv::Vec3b(255, 255, 255);
      } else if (i < 2 * height / 7) {
        // Yellow bar
        pixel = cv::Vec3b(0, 255, 255);
      } else if (i < 3 * height / 7) {
        // Cyan-blue bar
        pixel = cv::Vec3b(255, 255, 0);
      } else if (i < 4 * height / 7) {
        // Green bar
        pixel = cv::Vec3b(0, 255, 0);
      } else if (i < 5 * height / 7) {
        // Magenta bar
        pixel = cv::Vec3b(255, 0, 255);
      } else if (i < 6 * height / 7) {
        // Red bar
        pixel = cv::Vec3b(0, 0, 255);
      } else {
        // Blue bar
        pixel = cv::Vec3b(255, 0, 0);
      }

      // Add snow in the bottom right corner
      if (i > height * 3 / 4 && j > width * 3 / 4) {
        int r = rand() % 256;  // Random red value
        int g = rand() % 256;  // Random green value
        int b = rand() % 256;  // Random blue value
        pixel = cv::Vec3b(r, g, b);
      }
    }
  }
}

int main(int argc, char** argv) {
  // prepare stream url
  std::string url = "rtp://192.168.89.163:5000/";
  zetton::stream::StreamOptions options;
  options.resource = url;
  options.platform = zetton::stream::StreamPlatformType::PLATFORM_CPU;
  options.codec = zetton::stream::StreamCodec::CODEC_H264;
  options.pixel_format = zetton::stream::StreamPixelFormat::PIXEL_FORMAT_BGR;
  options.async = true;
  options.width = 1280;
  options.height = 720;
  options.bit_rate = 10240;
  options.frame_rate = 30;

  // print options
  options.resource.Print();

  // init streamer
  std::shared_ptr<zetton::stream::GstStreamOutput> output;
  output = std::make_shared<zetton::stream::GstStreamOutput>();
  output->Init(options);

  // start capturing
  std::thread t([&]() {
    cv::Mat frame = cv::Mat::zeros(options.height, options.width, CV_8UC3);
    while (true) {
      // generate random frame
      generate_smpte_with_snow(frame);
      // draw current time
      auto t = std::time(nullptr);
      auto tm = *std::localtime(&t);
      std::ostringstream oss;
      oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
      auto time_str = oss.str();
      // draw on frame
      cv::putText(frame, time_str, cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX,
                  1, cv::Scalar(0, 0, 0), 5);
      // push frame to stream
      if (output->Render(frame)) {
        AINFO_F("Send frame: {}x{}", frame.cols, frame.rows);
      }
      usleep(30000);
    }
  });
  t.detach();

  // start streamer
  output->Start();

  // stop streamer
  output->Stop();

  return 0;
}
