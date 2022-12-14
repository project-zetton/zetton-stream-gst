#include "zetton_stream_gst/sink/cv_gst_stream_output.h"

#include <memory>
#include <string>

#include "zetton_stream_gst/util/gst_pipeline.h"

namespace zetton {
namespace stream {

const char* CvGstStreamOutput::SupportedExtensions[] = {
    "mkv", "mp4", "qt", "flv", "avi", "h264", "h265", nullptr};

bool CvGstStreamOutput::IsSupportedExtension(const char* ext) {
  return IsGstSupportedExtension(ext, SupportedExtensions);
}

bool CvGstStreamOutput::Init(const StreamOptions& options) { return false; }
bool CvGstStreamOutput::Open() { return false; }
void CvGstStreamOutput::Close() {}
bool CvGstStreamOutput::Render(void* image, uint32_t width, uint32_t height) {
  return false;
}

void CvGstStreamOutput::SetStatus(const char* str) {}

}  // namespace stream
}  // namespace zetton
