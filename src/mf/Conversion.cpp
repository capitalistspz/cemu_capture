#include "Conversion.hpp"
#include "../util.hpp"
#include "cemu_capture.hpp"
#include <array>
#include <cassert>
#include <mfobjects.h>
#include <libyuv.h>
#include <algorithm>

namespace cemu_capture::conversion
{
	static void LinewiseCopy(const uint8_t* dataIn, unsigned strideIn, uint8_t* dataOut, unsigned strideOut, const Dimensions& dims)
	{
		assert(dims.width <= strideOut);
		for (auto lineNo = 0u; lineNo < dims.height; ++lineNo)
		{
			std::memcpy(dataOut + strideOut * lineNo, dataIn + strideIn * lineNo, dims.width);
		}
	}
	using Compressed2FixedConverter = void (*)(const Dimensions& dimensions, unsigned outputStride,
											   std::span<uint8_t> input, std::span<uint8_t> output);

	using Fixed2FixedConverter = void (*)(const Dimensions& dimensions, unsigned inputStride, unsigned outputStride,
										  const uint8_t* input, std::span<uint8_t> output);

	struct BaseDef
	{
		ImageFormat inputType;
		ImageFormat outputType;
	};

	struct Compressed2FixedDef : BaseDef
	{
		Compressed2FixedConverter converter;
	};
	struct Fixed2FixedDef : BaseDef
	{
		Fixed2FixedConverter converter;
	};

	constexpr std::array COMPRESSED2FIXED_TABLE{
		Compressed2FixedDef{
			ImageFormat::MJPG, ImageFormat::NV12,
			+[](const Dimensions& dims, unsigned outputStride,
				std::span<uint8_t> input, std::span<uint8_t> output) {
				libyuv::MJPGToNV12(input.data(), input.size(), output.data(), outputStride,
								   output.data() + dims.height * outputStride, outputStride, dims.width, dims.height,
								   dims.width, dims.height);
			}}};

	constexpr std::array FIXED2FIXED_TABLE{
		Fixed2FixedDef{
			ImageFormat::YUYV, ImageFormat::NV12,
			+[](const Dimensions& dims, unsigned inputStride, unsigned outputStride,
				const uint8_t* input, std::span<uint8_t> output) {
				libyuv::YUY2ToNV12(input, inputStride, output.data(), outputStride,
								   output.data() + dims.height * outputStride, outputStride, dims.width, dims.height);
			}},
		Fixed2FixedDef{
			ImageFormat::UYVY, ImageFormat::NV12,
			+[](const Dimensions& dims, unsigned inputStride, unsigned outputStride,
				const uint8_t* input, std::span<uint8_t> output) {
				libyuv::UYVYToNV12(input, inputStride, output.data(), outputStride,
								   output.data() + dims.height * outputStride, outputStride, dims.width, dims.height);
			}},
		Fixed2FixedDef{
			ImageFormat::ARGB32, ImageFormat::NV12,
			+[](const Dimensions& dims, unsigned inputStride, unsigned outputStride,
				const uint8_t* input, std::span<uint8_t> output) {
				libyuv::ARGBToNV12(input, inputStride, output.data(), outputStride,
								   output.data() + dims.height * outputStride, outputStride, dims.width, dims.height);
			}},
		Fixed2FixedDef{
			ImageFormat::NV12, ImageFormat::NV12,
			+[](const Dimensions& dims, unsigned inputStride, unsigned outputStride,
				const uint8_t* input, std::span<uint8_t> output) {
				const auto inputPlaneSize = dims.height * inputStride;
				const auto outputPlaneSize = dims.height * inputStride;
				libyuv::NV12Copy(input, inputStride, input + inputPlaneSize, inputStride,
								 output.data(), outputStride, output.data() + outputPlaneSize, outputStride,
								 dims.width, dims.height);
			}},
		Fixed2FixedDef{
			ImageFormat::YUYV, ImageFormat::YUYV,
			+[](const Dimensions& dims, unsigned inputStride, unsigned outputStride,
				const uint8_t* input, std::span<uint8_t> output) {
				assert(outputStride * dims.height <= output.size());
				LinewiseCopy(input, inputStride, output.data(), outputStride, dims);
			}}

	};

	bool CanConvert(ImageFormat inputFormat, ImageFormat outputFormat)
	{
		const auto conversionPred = [&](const BaseDef& def) {
			return def.inputType == inputFormat && def.outputType == outputFormat;
		};

		if (std::ranges::any_of(FIXED2FIXED_TABLE, conversionPred))
			return true;
		if (std::ranges::any_of(COMPRESSED2FIXED_TABLE, conversionPred))
			return true;
		return false;
	}

	void Convert(IMFMediaBuffer& buffer, ImageFormat inputFormat, ImageFormat outputFormat, unsigned outputStride, const Dimensions& dims, std::vector<uint8_t>& output)
	{
		const auto outputSize = SizeByFormat(outputFormat, outputStride, dims.height);

		const auto conversionPred = [&](const BaseDef& def) {
			return def.inputType == inputFormat && def.outputType == outputFormat;
		};
		bool converted = false;
		if (outputSize)
		{
			output.resize(*outputSize);

			if (IMF2DBuffer* buffer2d; SUCCEEDED(buffer.QueryInterface(&buffer2d)))
			{
				BYTE* line0 = nullptr;
				LONG pitch = 0;
				assert_hres_eval(buffer2d->Lock2D(&line0, &pitch));
				assert(pitch > 0);
				auto targetConversion = std::ranges::find_if(FIXED2FIXED_TABLE, conversionPred);
				if (targetConversion != FIXED2FIXED_TABLE.cend())
				{
					targetConversion->converter(dims, pitch, outputStride, line0, output);
					converted = true;
				}

				buffer2d->Unlock2D();
				buffer2d->Release();
			}
			else
			{
				BYTE* bytes = nullptr;
				DWORD length = 0;
				assert_hres_eval(buffer.Lock(&bytes, nullptr, &length));

				auto targetConversion = std::ranges::find_if(COMPRESSED2FIXED_TABLE, conversionPred);
				if (targetConversion != COMPRESSED2FIXED_TABLE.cend())
				{
					targetConversion->converter(dims, outputStride, std::span(bytes, length), output);
					converted = true;
				}
				buffer.Unlock();
			}
		}
		if (!converted && inputFormat == outputFormat)
		{
			BYTE* bytes = nullptr;
			DWORD length = 0;

			assert_hres_eval(buffer.Lock(&bytes, nullptr, &length));
			output.assign(bytes, bytes + length);
			buffer.Unlock();
		}
		else if (!converted)
		{
			assert(false && "Format not handled");
		}
	}
} // namespace cemu_capture::conversion