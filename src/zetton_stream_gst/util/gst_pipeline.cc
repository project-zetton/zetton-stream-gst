#include "zetton_stream_gst/util/gst_pipeline.h"

#include <fmt/format.h>
#include <zetton_stream/base/stream_uri.h>

#include "zetton_common/util/log.h"

namespace zetton {
namespace stream {

bool BuildGstPipelineSource(const StreamOptions& options, std::string& source,
                            const StreamProtocolType& protocol) {
  // Get the protocol type
  StreamProtocolType input_protocol = options.resource.protocol;
  if (protocol != StreamProtocolType::PROTOCOL_DEFAULT) {
    input_protocol = protocol;
  }

  // Build the pipeline head part according to the protocol type
  if (input_protocol == StreamProtocolType::PROTOCOL_APPSRC) {
    // appsrc
    source =
        "appsrc name=src do-timestamp=true min-latency=0 max-latency=0 "
        "max-bytes=1000 is-live=true";
  } else {
    AERROR_F("Unsupported protocol for pipeline head part: {}",
             StreamProtocolTypeToStr(input_protocol));
    return false;
  }
  return true;
}

bool BuildGstPipelineCaps(const StreamOptions& options,
                          std::string& pipeline_caps,
                          const StreamProtocolType& protocol) {
  // Get the protocol type
  StreamProtocolType input_protocol = options.resource.protocol;
  if (protocol != StreamProtocolType::PROTOCOL_DEFAULT) {
    input_protocol = protocol;
  }

  pipeline_caps =
      fmt::format("! video/x-raw,framerate={}/1,width={},height={}",
                  options.frame_rate, options.width, options.height);

  if (input_protocol == StreamProtocolType::PROTOCOL_APPSRC) {
    // appsrc
    pipeline_caps += fmt::format(",format=BGR");
  }

  return true;
}

bool BuildGstPipelineEncoding(const StreamOptions& options,
                              std::string& pipeline_encode,
                              const StreamProtocolType& protocol) {
  // Build the pipeline encoding part according to the codec type and platform
  if (options.codec == StreamCodec::CODEC_H264) {
    if (options.platform == StreamPlatformType::PLATFORM_CPU) {
      pipeline_encode = fmt::format(
          " ! queue ! x264enc tune=zerolatency bitrate={} key-int-max=30 ! "
          "video/x-h264, profile=baseline",
          options.bit_rate);
    } else if (options.platform == StreamPlatformType::PLATFORM_ROCKCHIP) {
      pipeline_encode = " ! queue ! mpph264enc ! video/x-h264, profile=high";
    } else if (options.platform == StreamPlatformType::PLATFORM_JETSON) {
      pipeline_encode = fmt::format(
          " ! nvvidconv ! video/x-raw(memory:NVMM),format=I420 ! omxh264enc "
          "bitrate={} ! video/x-h264, stream-format=byte-stream",
          options.bit_rate);
    } else {
      AERROR_F("Unsupported platform for pipeline encoding part: {}",
               StreamPlatformTypeToStr(options.platform));
      return false;
    }
  } else {
    AERROR_F("Unsupported codec for pipeline encoding part: {}",
             StreamCodecToStr(options.codec));
    return false;
  }

  return true;
}

bool BuildGstPipelineDecoding(const StreamOptions& options,
                              std::string& pipeline_decode,
                              const StreamProtocolType& protocol) {
  return false;
}

bool BuildGstPipelineMuxing(const StreamOptions& options,
                            std::string& pipeline_mux,
                            const StreamProtocolType& protocol) {
  // 1. Get the protocol type
  StreamProtocolType input_protocol = options.resource.protocol;
  if (protocol != StreamProtocolType::PROTOCOL_DEFAULT) {
    input_protocol = protocol;
  }

  // 2. Build the pipeline muxing part according to the protocol type and codec
  // type
  // 2.1. Build for RTSP/RTP
  if (input_protocol == StreamProtocolType::PROTOCOL_RTP ||
      input_protocol == StreamProtocolType::PROTOCOL_RTSP) {
    if (options.codec == StreamCodec::CODEC_H264) {
      pipeline_mux = " ! h264parse ! queue ! rtph264pay name=pay0 pt=96";
    } else {
      AERROR_F("Unsupported codec for pipeline muxing part: {}",
               StreamCodecToStr(options.codec));
      return false;
    }
  }
  // 2.2. Build for file
  else if (input_protocol == StreamProtocolType::PROTOCOL_FILE) {
    if (options.codec == StreamCodec::CODEC_H264) {
      pipeline_mux = " ! h264parse ! queue ! mp4mux name=mux";
    } else {
      AERROR_F("Unsupported codec for pipeline muxing part: {}",
               StreamCodecToStr(options.codec));
      return false;
    }
  }
  // 2.X. Build for other protocols
  else {
    AERROR_F("Unsupported protocol for pipeline muxing part: {}",
             StreamProtocolTypeToStr(input_protocol));
    return false;
  }

  return true;
}

bool BuildGstPipelineSink(const StreamOptions& options,
                          std::string& pipeline_sink,
                          const StreamProtocolType& protocol) {
  // 1. Get the protocol type
  StreamProtocolType input_protocol = options.resource.protocol;
  if (protocol != StreamProtocolType::PROTOCOL_DEFAULT) {
    input_protocol = protocol;
  }

  // 2. Build the pipeline head part according to the protocol type
  // 2.1. Build for RTSP/RTP
  if (input_protocol == StreamProtocolType::PROTOCOL_RTP) {
    pipeline_sink =
        fmt::format(" ! udpsink host={} port={}", options.resource.location,
                    options.resource.port);
    if (options.async) {
      pipeline_sink += " sync=false";
    }
  }
  // 2.2. Build for file
  else if (input_protocol == StreamProtocolType::PROTOCOL_FILE) {
    pipeline_sink =
        fmt::format(" ! filesink location={}", options.resource.location);
  }
  // 2.X. Build for other protocols
  else {
    AERROR_F("Unsupported protocol for pipeline head part: {}",
             StreamProtocolTypeToStr(input_protocol));
    return false;
  }

  return true;
}

}  // namespace stream
}  // namespace zetton
