#include <cassert>
#include <fstream>
#include <libyuv.h>
#include <cemu_capture.hpp>
#include <print>
#include <thread>

void LogCallback(cemu_capture::LogLevel, std::string_view);
std::string_view FormatToString(cemu_capture::ImageFormat);

int main()
{
    auto context = cemu_capture::Context::Create();
    context->SetLogCallback(LogCallback);

    const auto sourceInfos = context->EnumerateSources();
    if (sourceInfos.empty())
    {
        std::println("No devices found");
        return 1;
    }

    for (const auto& info : sourceInfos)
    {
        std::println("{} {}", info.id, info.name);
    }

    auto source = context->OpenDevice(sourceInfos.front().id);
    for (const auto& [dimensions, framerate, format] : source->EnumerateStreamFormats())
    {
        std::println("{} {}x{}@{} fps", FormatToString(format), dimensions.width, dimensions.height, framerate);
    }
    source->SetOutputFormat(cemu_capture::ImageFormat::NV12);


    const auto actualFormat = source->StartStreaming({
        .dimensions = {640, 480}, .framerate = 30, .format = cemu_capture::ImageFormat::YUYV
    });
    if (!actualFormat)
        return 1;
    auto rgbBuffer = std::vector<uint8_t>(actualFormat->dimensions.width * actualFormat->dimensions.height * 3);

    source->SetCaptureCallback([&](cemu_capture::Source&, std::span<const std::uint8_t> bytes)
    {
        static auto counter = 0u;
        static auto lastTime = std::chrono::high_resolution_clock::now();
        const auto dims = actualFormat->dimensions;
        const auto planeSize = dims.width * dims.height;
        if (auto file = std::ofstream(std::format("output_{}.bgr", counter)))
        {
            // Convert to BGR888
            libyuv::NV12ToRGB24(bytes.data(), dims.width, bytes.data() + planeSize, dims.width, rgbBuffer.data(),
                                dims.width * 3, dims.width, dims.height);
            file.write(reinterpret_cast<const std::ostream::char_type*>(rgbBuffer.data()), rgbBuffer.size());
        }

        const auto now = std::chrono::high_resolution_clock::now();
        std::println("{}ms", std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count());
        lastTime = now;
    });

    std::this_thread::sleep_for(std::chrono::seconds(30));
}

void LogCallback(cemu_capture::LogLevel level, std::string_view sv)
{
    switch (level)
    {
    case cemu_capture::LogLevel::Info:
        std::print("[Info] ");
        break;
    case cemu_capture::LogLevel::Warning:
        std::print("[Warn] ");
        break;
    case cemu_capture::LogLevel::Error:
        std::print("[Err ] ");
        break;
    default:
        assert(false);
    }
    std::println("{}", sv);
}

std::string_view FormatToString(cemu_capture::ImageFormat format)
{
    switch (format)
    {
    case cemu_capture::ImageFormat::Unspecified:
        return "None";
    case cemu_capture::ImageFormat::NV12:
        return "NV12";
    case cemu_capture::ImageFormat::NV21:
        return "NV21";
    case cemu_capture::ImageFormat::YUYV:
        return "YUYV";
    case cemu_capture::ImageFormat::UYVY:
        return "UYVY";
    case cemu_capture::ImageFormat::MJPG:
        return "MJPEG";
    case cemu_capture::ImageFormat::RGB24:
        return "RGB24";
    case cemu_capture::ImageFormat::ARGB32:
        return "ARGB32";
    default:
        throw std::invalid_argument("Invalid format");
    }
}
