#ifndef CEMU_CAPTURE_V4L2_SOURCE_HPP
#define CEMU_CAPTURE_V4L2_SOURCE_HPP

#include <mutex>
#include <optional>
#include <vector>
#include <cemu_capture.hpp>
#include <linux/videodev2.h>
#include "../SourceCommon.hpp"

#include "FileDescriptor.hpp"
#include "MemoryMapped.hpp"

namespace cemu_capture
{
    class V4L2Context;

    struct Stream
    {
        v4l2_format format;
    };

    class V4L2Source final : public SourceCommon, public std::enable_shared_from_this<V4L2Source>
    {
        void AllocateAndQueueBuffers(size_t n, const v4l2_format& format);

    public:
        V4L2Source(std::shared_ptr<V4L2Context>, FileDescriptor fd, SourceInfo info);
        ~V4L2Source() override = default;
        void Capture(std::vector<uint8_t>& outputBuffer) override;
        std::optional<StreamFormat> StartStreaming(const StreamFormat& formatInfo) override;
        void StopStreaming() override;
        void SetOutputFormat(ImageFormat outputFormat) override;
        std::vector<StreamFormat> EnumerateStreamFormats() override;
        void SetProperty(StreamIntProperty property, int propertyValue) override;
        int GetProperty(StreamIntProperty property) override;
        SourceInfo GetInfo() const override;
        bool CanConvert(ImageFormat from, ImageFormat to) const override;
        int GetFd();
        void UpdateData();

    private:
        std::optional<Stream> m_stream;
        std::mutex m_mutex;
        std::vector<MemoryMapped<uint8_t>> m_mappedBuffers;
        std::vector<uint8_t> m_outputBuffer;
        std::shared_ptr<V4L2Context> m_ctx;
        FileDescriptor m_fd;
        unsigned m_outputStride = 0;
        uint32_t m_outputFormat = 0;
        SourceInfo m_deviceInfo;
        bool m_logIfCannotConvert = false;
        uint32_t m_frameSequenceIndex;
    };
}
#endif
