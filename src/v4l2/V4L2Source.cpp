#include "V4L2Source.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <linux/videodev2.h>
#include "Fract.hpp"
#include <libyuv.h>
#include "../util.hpp"

#include "MemoryMapped.hpp"
#include "V4L2Context.hpp"
#include "ConversionTable.hpp"
#include "V4L2Ioctls.hpp"

namespace cemu_capture
{
    constexpr static uint32_t ToV4L2Format(ImageFormat format)
    {
        switch (format)
        {
            using enum ImageFormat;
        case MJPG:
            return V4L2_PIX_FMT_MJPEG;
        case NV12:
            return V4L2_PIX_FMT_NV12;
        case YUYV:
            return V4L2_PIX_FMT_YUYV;
        case RGB24:
            return V4L2_PIX_FMT_RGB24;
        case ARGB32:
            return V4L2_PIX_FMT_ARGB32;
        case NV21:
            return V4L2_PIX_FMT_NV21;
        case UYVY:
            return V4L2_PIX_FMT_UYVY;
        default:
            return 0;
        }
    }

    constexpr static ImageFormat FromV4L2Format(uint32_t format)
    {
        switch (format)
        {
            using enum ImageFormat;
        case V4L2_PIX_FMT_MJPEG:
            return MJPG;
        case V4L2_PIX_FMT_NV12:
            return NV12;
        case V4L2_PIX_FMT_YUYV:
            return YUYV;
        case V4L2_PIX_FMT_RGB24:
            return RGB24;
        case V4L2_PIX_FMT_ARGB32:
            return ARGB32;
        case V4L2_PIX_FMT_NV21:
            return NV21;
        case V4L2_PIX_FMT_UYVY:
            return UYVY;
        default:
            return Unspecified;
        }
    }

    std::string_view FormatToString(ImageFormat format)
    {
        switch (format)
        {
            using enum ImageFormat;
        case Unspecified:
            return "None";
        case NV12:
            return "NV12";
        case YUYV:
            return "YUYV";
        case MJPG:
            return "MJPG";
        case RGB24:
            return "RGB24";
        case ARGB32:
            return "ARGB32";
        default:
            throw std::invalid_argument("Invalid format");
        }
    }

    std::string FourCCToString(uint32_t fourcc)
    {
        return std::string{
            static_cast<char>((fourcc >> 0) & 0xFF),
            static_cast<char>((fourcc >> 8) & 0xFF),
            static_cast<char>((fourcc >> 16) & 0xFF),
            static_cast<char>((fourcc >> 24) & 0xFF)
        };
    }

    constexpr static v4l2_buf_type GetBufType(cemu_capture::ImageFormat format)
    {
        return format == ImageFormat::NV12 ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
    }

    constexpr static std::optional<uint32_t> PropertyToCID(StreamIntProperty prop)
    {
        switch (prop)
        {
        case StreamIntProperty::Brightness:
            return V4L2_CID_BRIGHTNESS;
        case StreamIntProperty::Hue:
            return V4L2_CID_HUE;
        case StreamIntProperty::Saturation:
            return V4L2_CID_SATURATION;
        case StreamIntProperty::Contrast:
            return V4L2_CID_CONTRAST;
        case StreamIntProperty::Gamma:
            return V4L2_CID_GAMMA;
        case StreamIntProperty::Sharpness:
            return V4L2_CID_SHARPNESS;
        default:
            return std::nullopt;
        }
    }

    void V4L2Source::AllocateAndQueueBuffers(size_t n, const v4l2_format& format)
    {
        v4l2_requestbuffers reqBuffers{};
        reqBuffers.count = n;
        reqBuffers.type = format.type;
        reqBuffers.memory = V4L2_MEMORY_MMAP;
        auto res = vidioc::reqbufs(m_fd, &reqBuffers);

        m_mappedBuffers.clear();

        if (res != 0)
        {
            m_ctx->Log(LogLevel::Error, "Failed to request buffers: {}", std::strerror(errno));
            return;
        }


        for (auto i = 0u; i < reqBuffers.count; ++i)
        {
            v4l2_buffer buf{};
            buf.type = format.type;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            res = vidioc::querybuf(m_fd, &buf);

            assert(res != -1);

            if (buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
            {
                auto mapping =
                    MemoryMapped<std::uint8_t>(m_fd.Get(), buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                                               buf.m.offset);
                assert(mapping.is_valid());

                m_mappedBuffers.emplace_back(std::move(mapping));
            }
            else if (buf.type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
            {
                for (auto planeIndex = 0; planeIndex < format.fmt.pix_mp.num_planes; ++planeIndex)
                {
                    auto mapping =
                        MemoryMapped<std::uint8_t>(m_fd.Get(), buf.m.planes[i].length, PROT_READ | PROT_WRITE,
                                                   MAP_SHARED,
                                                   buf.m.planes[i].m.mem_offset);
                    assert(mapping.is_valid());
                    m_mappedBuffers.emplace_back(std::move(mapping));
                }
            }
            res = vidioc::qbuf(m_fd, &buf);
            if (res != 0)
            {
                throw std::runtime_error(std::string("Failed to enqueue buffers: ") + std::strerror(errno));
            }
        }
    }

    V4L2Source::V4L2Source(std::shared_ptr<V4L2Context> context, FileDescriptor fd, SourceInfo deviceInfo) :
        m_ctx(std::move(context)),
        m_fd(std::move(fd)), m_deviceInfo(std::move(deviceInfo))
    {
    }

    void V4L2Source::Capture(std::vector<uint8_t>& outputBuffer)
    {
        std::scoped_lock lock(m_mutex);
        outputBuffer.assign(m_outputBuffer.begin(), m_outputBuffer.end());
    }

    std::optional<StreamFormat> V4L2Source::StartStreaming(const StreamFormat& formatInfo)
    {
        std::scoped_lock lock(m_mutex);
        if (m_stream)
        {
            m_ctx->Log(LogLevel::Error, "Tried to start stream when already streaming");
            return std::nullopt;
        }
        v4l2_format format{};
        format.type = GetBufType(formatInfo.format);
        const auto v4l2PixFmt = ToV4L2Format(formatInfo.format);

        StreamFormat actualFormat{};

        // Poor man's pattern matching
        auto setFormat = [&](auto& fmtStruct) -> bool
        {
            fmtStruct.width = formatInfo.dimensions.width;
            fmtStruct.height = formatInfo.dimensions.height;
            fmtStruct.pixelformat = v4l2PixFmt;
            auto res = vidioc::s_fmt(m_fd, &format);
            if (res != 0)
            {
                if (errno == EINVAL)
                    m_ctx->Log(LogLevel::Warning, "Device does not support {} image formats",
                               format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ? "multiplanar" : "uniplanar");
                else
                    m_ctx->Log(LogLevel::Error, "Failed to set format: {}", std::strerror(errno));
                return false;
            }
            m_ctx->Log(LogLevel::Info, "Requested {} {}x{}, got {} {}x{}",
                       FourCCToString(v4l2PixFmt), formatInfo.dimensions.width, formatInfo.dimensions.height,
                       FourCCToString(fmtStruct.pixelformat), no_bind(fmtStruct.width), no_bind(fmtStruct.height));
            actualFormat.dimensions.width = fmtStruct.width;
            actualFormat.dimensions.height = fmtStruct.height;
            actualFormat.format = FromV4L2Format(fmtStruct.pixelformat);
            return true;
        };

        if (format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
        {
            if (!setFormat(format.fmt.pix))
                return std::nullopt;
        }
        else
        {
            if (!setFormat(format.fmt.pix_mp))
                return std::nullopt;
        }

        v4l2_streamparm streamparam{};
        streamparam.type = format.type;
        streamparam.parm.capture.timeperframe = FramerateDoubleToIntervalFract(formatInfo.framerate);
        int res = vidioc::s_parm(m_fd, &streamparam);

        if (res != 0)
        {
            m_ctx->Log(LogLevel::Error, "Failed to set framerate: {}", std::strerror(errno));
        }
        else
        {
            actualFormat.framerate = IntervalFractToFramerateDouble(streamparam.parm.capture.timeperframe);
        }

        AllocateAndQueueBuffers(20, format);

        constexpr int enable = 1;
        // epoll_wait considers the fd errored if stream is not on
        res = vidioc::streamon(m_fd, &enable);

        if (res != 0)
        {
            m_ctx->Log(LogLevel::Error, "Failed to start stream: {}", std::strerror(errno));
            return std::nullopt;
        }
        m_stream.emplace(format);
        m_ctx->AddDevice(shared_from_this());
        return actualFormat;
    }

    void V4L2Source::StopStreaming()
    {
        std::scoped_lock lock(m_mutex);
        constexpr int enable = 1;
        vidioc::streamoff(m_fd, &enable);
        m_stream.reset();
        m_mappedBuffers.clear();
    }

    int V4L2Source::GetFd()
    {
        return m_fd.Get();
    }

    constexpr static std::optional<std::size_t> SizeByFormat(ImageFormat fmt, unsigned bytesPerLine, unsigned height)
    {
        const auto planeSize = bytesPerLine * height;
        switch (fmt)
        {
        case ImageFormat::NV12:
        case ImageFormat::NV21:
            return planeSize + (planeSize >> 1);
        case ImageFormat::YUYV:
        case ImageFormat::UYVY:
            return planeSize * 2;
        case ImageFormat::RGB24:
            return planeSize * 3;
        case ImageFormat::ARGB32:
            return planeSize * 4;
        default:
            return std::nullopt;
        }
    }

    void V4L2Source::UpdateData()
    {
        std::scoped_lock lock(m_mutex);
        if (!m_stream)
            return;
        const auto streamFormat = m_stream->format;
        v4l2_buffer buffer{};
        buffer.type = streamFormat.type;
        buffer.memory = V4L2_MEMORY_MMAP;
        const auto res = vidioc::dqbuf(m_fd, &buffer);
        if (res == -1)
            return;
        uint32_t width;
        uint32_t height;
        uint32_t inputFormat;
        if (streamFormat.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
        {
            const auto& pixMp = streamFormat.fmt.pix_mp;
            width = pixMp.width;
            height = pixMp.height;
            inputFormat = pixMp.pixelformat;
        }
        else
        {
            const auto& pix = streamFormat.fmt.pix;
            width = pix.width;
            height = pix.height;
            inputFormat = pix.pixelformat;
        }
        const auto outputStride = (m_outputStride != 0) ? m_outputStride : width;
        const auto outputFormat = (m_outputFormat == 0) ? inputFormat : m_outputFormat;

        if (streamFormat.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
        {
            const auto& currentMappedBuffer = m_mappedBuffers[buffer.index];
            const auto size = SizeByFormat(FromV4L2Format(outputFormat), outputStride, height);

            if (size)
            {
                m_outputBuffer.resize(*size);
                for (const auto& conversion : conversion::UNIPLANAR_TABLE)
                {
                    if (conversion.inputType != inputFormat || conversion.outputType != outputFormat)
                        continue;
                    conversion.converter(streamFormat.fmt.pix, outputStride, currentMappedBuffer, m_outputBuffer);
                    break;
                }
            }
            else
            {
                m_outputBuffer.assign(currentMappedBuffer.cbegin(), currentMappedBuffer.cend());
                if (m_logIfCannotConvert && inputFormat != m_outputFormat)
                {
                    m_ctx->Log(LogLevel::Info, "Conversion failed");
                    m_logIfCannotConvert = false;
                }
            }
        }
        else if (m_stream->format.type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
        {
            const auto planeCount = streamFormat.fmt.pix_mp.num_planes;
            //TODO: I hate this
            std::vector<std::span<const uint8_t>> currentBuffers;
            for (auto i = 0; i != planeCount; ++i)
            {
                currentBuffers.emplace_back(m_mappedBuffers[buffer.index + i]);
            }
            for (const auto& conversion : conversion::MULTIPLANAR_TABLE)
            {
                if (conversion.inputType != inputFormat || conversion.outputType != outputFormat)
                    continue;
                conversion.converter(streamFormat.fmt.pix_mp, outputStride, currentBuffers, m_outputBuffer);
                break;
            }
        }
        else
        {
            assert(false);
        }

        InvokeCaptureCallback(m_outputBuffer);
        const auto qres = vidioc::qbuf(m_fd, &buffer);
        assert(qres != -1);
    }

    void V4L2Source::SetProperty(StreamIntProperty property, int propertyValue)
    {
        const auto cid = PropertyToCID(property);
        std::scoped_lock lock(m_mutex);
        if (cid)
        {
            v4l2_control ctrl{.id = *cid, .value = propertyValue};
            const auto res = vidioc::s_ctrl(m_fd, &ctrl);
            if (res == -1)
            {
                if (errno == EINVAL || errno == ERANGE)
                {
                    throw std::invalid_argument("Unsupported property or invalid value");
                }
                throw std::runtime_error(std::strerror(errno));
            }
        }
        else if (property == StreamIntProperty::OutputStride)
        {
            if (propertyValue < 0)
                throw std::invalid_argument("Stride cannot be negative");
            m_outputStride = static_cast<unsigned>(propertyValue);
        }
        else
        {
            throw std::invalid_argument("Unsupported property");
        }
    }

    int V4L2Source::GetProperty(StreamIntProperty property)
    {
        const auto cid = PropertyToCID(property);
        std::scoped_lock lock(m_mutex);
        if (cid)
        {
            v4l2_control ctrl{.id = *cid};
            const auto res = vidioc::g_ctrl(m_fd, &ctrl);
            if (res == -1)
            {
                if (errno == EINVAL)
                    throw std::invalid_argument("Unsupported property");
                throw std::runtime_error(std::strerror(errno));
            }
            return ctrl.value;
        }
        else if (property == StreamIntProperty::OutputStride)
        {
            return static_cast<int>(m_outputStride);
        }
        else
        {
            throw std::invalid_argument("Unsupported property");
        }
    }

    SourceInfo V4L2Source::GetInfo() const
    {
        return m_deviceInfo;
    }

    bool V4L2Source::CanConvert(ImageFormat from, ImageFormat to) const
    {
        const auto v4l2From = ToV4L2Format(from);
        const auto v4l2To = ToV4L2Format(to);

        const auto predicate = [&](const auto& def) { return def.inputType == v4l2From && def.outputType == v4l2To; };

        if (std::ranges::any_of(conversion::UNIPLANAR_TABLE, predicate))
            return true;
        if (std::ranges::any_of(conversion::MULTIPLANAR_TABLE, predicate))
            return true;
        return false;
    }

    void V4L2Source::SetOutputFormat(ImageFormat outputFormat)
    {
        const auto newFormat = ToV4L2Format(outputFormat);
        if (newFormat == 0 && outputFormat != ImageFormat::Unspecified)
            throw std::invalid_argument("Unknown output format");
        std::scoped_lock lock(m_mutex);
        m_outputFormat = newFormat;
        m_logIfCannotConvert = true;
    }

    std::vector<StreamFormat> V4L2Source::EnumerateStreamFormats()
    {
        std::vector<StreamFormat> streamFormats;
        for (const auto bufType : {V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE})
        {
            v4l2_fmtdesc formatDescEnum{};
            formatDescEnum.type = bufType;
            std::vector<int> v4l2Formats;

            for (formatDescEnum.index = 0; vidioc::enum_fmt(m_fd, &formatDescEnum) == 0; ++formatDescEnum.index)
            {
                const auto imageFormat = FromV4L2Format(formatDescEnum.pixelformat);
                if (imageFormat == ImageFormat::Unspecified)
                    continue;
                v4l2_frmsizeenum frmSizeEnum{};
                frmSizeEnum.pixel_format = formatDescEnum.pixelformat;
                for (frmSizeEnum.index = 0; vidioc::enum_framesizes(m_fd, &frmSizeEnum) == 0; ++frmSizeEnum.index)
                {
                    if (frmSizeEnum.type != V4L2_FRMSIZE_TYPE_DISCRETE)
                        continue;
                    v4l2_frmivalenum frmivalEnum{};
                    frmivalEnum.pixel_format = formatDescEnum.pixelformat;
                    frmivalEnum.width = frmSizeEnum.discrete.width;
                    frmivalEnum.height = frmSizeEnum.discrete.height;
                    frmivalEnum.type = bufType;
                    for (frmivalEnum.index = 0; vidioc::enum_frameintervals(m_fd, &frmivalEnum) == 0; ++
                         frmivalEnum.index)
                    {
                        if (frmivalEnum.type == V4L2_FRMIVAL_TYPE_DISCRETE)
                        {
                            streamFormats.emplace_back(
                                Dimensions{frmivalEnum.width, frmivalEnum.height},
                                IntervalFractToFramerateDouble(frmivalEnum.discrete),
                                imageFormat);
                        }
                    }
                }
            }
        }
        return streamFormats;
    }
}
