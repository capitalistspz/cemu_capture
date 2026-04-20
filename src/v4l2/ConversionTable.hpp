#ifndef CEMU_CAPTURE_V4L2_CONVERSION_TABLE
#define CEMU_CAPTURE_V4L2_CONVERSION_TABLE
#include <cstdint>
#include <span>
#include <libyuv.h>
#include <linux/videodev2.h>

namespace cemu_capture::conversion
{
    using UniplanarInputConverter = void(*)(const v4l2_pix_format& inputFormat, unsigned outputStride,
                                            std::span<const uint8_t> input, std::span<uint8_t> output);

    using MultiplanarInputConverter = void(*)(const v4l2_pix_format_mplane& inputFormat, unsigned outputStride,
                                              std::span<const std::span<const uint8_t>> inputPlanes,
                                              std::span<uint8_t> output);

    struct BaseDef
    {
        uint32_t inputType;
        uint32_t outputType;
    };

    struct UniplanarDef : BaseDef
    {
        UniplanarInputConverter converter;
    };

    struct MultiplanarDef : BaseDef
    {
        MultiplanarInputConverter converter;
    };

    constexpr std::array UNIPLANAR_TABLE{
        UniplanarDef{
            V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_NV12,
            +[](const v4l2_pix_format& pix, unsigned outputStride,
                std::span<const uint8_t> input, std::span<uint8_t> output)
            {
                libyuv::MJPGToNV12(input.data(), input.size(), output.data(), outputStride,
                                   output.data() + pix.height * outputStride, outputStride, pix.width, pix.height,
                                   pix.width, pix.height);
            }
        },
        UniplanarDef{
            V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_NV12,
            +[](const v4l2_pix_format& pix, unsigned outputStride,
                std::span<const uint8_t> input, std::span<uint8_t> output)
            {
                libyuv::YUY2ToNV12(input.data(), pix.bytesperline, output.data(), outputStride,
                                   output.data() + pix.height * outputStride, outputStride, pix.width, pix.height);
            }
        },
        UniplanarDef{
            V4L2_PIX_FMT_UYVY, V4L2_PIX_FMT_NV12,
            +[](const v4l2_pix_format& pix, unsigned outputStride,
                std::span<const uint8_t> input, std::span<uint8_t> output)
            {
                libyuv::UYVYToNV12(input.data(), pix.bytesperline, output.data(), outputStride,
                                   output.data() + pix.height * outputStride, outputStride, pix.width, pix.height);
            }
        },
        UniplanarDef{
            V4L2_PIX_FMT_RGBA32, V4L2_PIX_FMT_NV12,
            +[](const v4l2_pix_format& pix, unsigned outputStride,
                std::span<const uint8_t> input, std::span<uint8_t> output)
            {
                // V4L2 RGBA32 is RGBA8888, libyuv RGBA32 is ABGR8888
                libyuv::ABGRToNV12(input.data(), pix.bytesperline, output.data(), outputStride,
                                   output.data() + pix.height * outputStride, outputStride, pix.width, pix.height);
            }
        }
    };

    constexpr std::array MULTIPLANAR_TABLE{
        MultiplanarDef{
            V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_NV12, +[](const v4l2_pix_format_mplane& pixMp, unsigned outputStride,
                                                      std::span<const std::span<const uint8_t>> input,
                                                      std::span<uint8_t> output)
            {
                libyuv::NV12Copy(input[0].data(), pixMp.plane_fmt[0].bytesperline, input[1].data(),
                                 pixMp.plane_fmt[1].bytesperline, output.data(), outputStride,
                                 output.data() + pixMp.height * outputStride, outputStride, pixMp.width, pixMp.height);
            }
        },
        MultiplanarDef{
            V4L2_PIX_FMT_YUV420, V4L2_PIX_FMT_NV12, +[](const v4l2_pix_format_mplane& pixMp, unsigned outputStride,
                                                        std::span<const std::span<const uint8_t>> input,
                                                        std::span<uint8_t> output)
            {
                libyuv::I420ToNV12(
                    input[0].data(), pixMp.plane_fmt[0].bytesperline,
                    input[1].data(), pixMp.plane_fmt[1].bytesperline,
                    input[2].data(), pixMp.plane_fmt[2].bytesperline,
                    output.data(), outputStride, output.data() + outputStride * pixMp.height, outputStride,
                    pixMp.width, pixMp.height);
            }
        },
        MultiplanarDef{
            V4L2_PIX_FMT_YUV444, V4L2_PIX_FMT_NV12, +[](const v4l2_pix_format_mplane& pixMp, unsigned outputStride,
                                                        std::span<const std::span<const uint8_t>> input,
                                                        std::span<uint8_t> output)
            {
                libyuv::I444ToNV12(
                    input[0].data(), pixMp.plane_fmt[0].bytesperline,
                    input[1].data(), pixMp.plane_fmt[1].bytesperline,
                    input[2].data(), pixMp.plane_fmt[2].bytesperline,
                    output.data(), outputStride, output.data() + outputStride * pixMp.height, outputStride,
                    pixMp.width, pixMp.height);
            }
        },
        MultiplanarDef{
            V4L2_PIX_FMT_NV21, V4L2_PIX_FMT_NV12, +[](const v4l2_pix_format_mplane& pixMp, unsigned outputStride,
                                                      std::span<const std::span<const uint8_t>> input,
                                                      std::span<uint8_t> output)
            {
                libyuv::NV21ToNV12(
                    input[0].data(), pixMp.plane_fmt[0].bytesperline,
                    input[1].data(), pixMp.plane_fmt[1].bytesperline,
                    output.data(), outputStride, output.data() + outputStride * pixMp.height, outputStride,
                    pixMp.width, pixMp.height);
            }
        }
    };
}
#endif
