#include <catch2/catch_test_macros.hpp>

#include "zetton_stream_gst/sink/gst_stream_output.h"

TEST_CASE("GstStreamOutput initialization", "[GstStreamOutput]") {
  zetton::stream::StreamOptions options;
  zetton::stream::GstStreamOutput output;

  SECTION("Initialization succeeds") {
    std::string url = "rtp://localhost:5000/";
    options.resource = url;
    options.platform = zetton::stream::StreamPlatformType::PLATFORM_CPU;
    options.codec = zetton::stream::StreamCodec::CODEC_H264;
    options.pixel_format = zetton::stream::StreamPixelFormat::PIXEL_FORMAT_BGR;
    options.async = true;
    options.width = 1280;
    options.height = 720;
    options.bit_rate = 10240;
    options.frame_rate = 30;

    REQUIRE(output.Init(options));
  }

  SECTION("Initialization fails with invalid options") {
    options.resource = "invalid_uri";
    REQUIRE_FALSE(output.Init(options));
  }
}
