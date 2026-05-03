#include <cassert>
#include <mfobjects.h>
#include <vector>

#include "cemu_capture.hpp"

namespace cemu_capture::conversion
{
	bool CanConvert(ImageFormat in, ImageFormat out);
	void Convert(IMFMediaBuffer& buffer, ImageFormat inputFormat, ImageFormat outputFormat, unsigned outputStride, const Dimensions& dims, std::vector<uint8_t>& output);
} // namespace cemu_capture::conversion