#ifndef CEMU_CAPTURE_CAPTURE_HPP
#define CEMU_CAPTURE_CAPTURE_HPP

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <span>

namespace cemu_capture
{
    enum class ImageFormat
    {
        Unspecified,
        NV12,
        NV21,
        YUYV,
        UYVY,
        MJPG,
        RGB24,
        // As opposed to ARGB8888
        ARGB32
    };

    enum class StreamIntProperty
    {
        Brightness,
        Hue,
        Saturation,
        Contrast,
        Gamma,
        Sharpness,
        OutputStride
    };

    enum class CaptureErrorPolicy
    {
        // Does not trigger callback and does not update data for Source::Capture
        IgnoreFrame,
        // Triggers callback with bad frame data, updates data for Source::Capture
        PushBadFrame
    };

    enum class CaptureErrorType
    {
        None,
        DataCorrupted,
    };

    enum class LogLevel
    {
        Info,
        Warning,
        Error
    };

    struct SourceInfo
    {
        std::string id;
        std::string name;
    };

    struct Dimensions
    {
        unsigned width;
        unsigned height;
    };

    struct StreamFormat
    {
        Dimensions dimensions;
        double framerate;
        ImageFormat format;
    };

    class Source
    {
    public:
        // Tries to set format as passed in, returns actual format, if possible
        // Make sure format is supported, check by calling Device::EnumerateStreamFormats
        virtual std::optional<StreamFormat> StartStreaming(const StreamFormat& formatInfo) = 0;
        virtual void StopStreaming() = 0;
        virtual std::vector<StreamFormat> EnumerateStreamFormats() = 0;

        // Do not call from within the capture callback
        // Can be called safely from any thread
        virtual void Capture(std::vector<uint8_t>& outputBuffer) = 0;
        virtual void SetCaptureCallback(std::function<void(Source&, CaptureErrorType, std::span<const uint8_t> bytes)>) = 0;

        [[nodiscard]] virtual bool CanConvert(ImageFormat from, ImageFormat to) const = 0;
        virtual void SetOutputFormat(ImageFormat outputFormat) = 0;
        virtual void SetCaptureErrorPolicy(CaptureErrorPolicy) = 0;

        // Throws std::invalid_argument if property does not exist
        virtual void SetProperty(StreamIntProperty property, int propertyValue) = 0;
        [[nodiscard]] virtual int GetProperty(StreamIntProperty property) = 0;

        [[nodiscard]] virtual SourceInfo GetInfo() const = 0;
        virtual ~Source() = default;
    };

    class Context
    {
    public:
        static std::shared_ptr<Context> Create();
        virtual std::vector<SourceInfo> EnumerateSources() = 0;
        virtual std::shared_ptr<Source> OpenDevice(std::string const& id) = 0;
        virtual void SetLogCallback(std::function<void(LogLevel, std::string_view)> fn) = 0;
        virtual ~Context() = default;
    };
}
#endif
