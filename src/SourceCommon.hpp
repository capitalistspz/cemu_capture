#ifndef CEMU_CAPTURE_SOURCE_COMMON_HPP
#define CEMU_CAPTURE_SOURCE_COMMON_HPP
#include "cemu_capture.hpp"

namespace cemu_capture
{
    class SourceCommon : public Source
    {
        std::function<void(Source&, std::span<const std::uint8_t>)> m_captureFun;
    public:
        void SetCaptureCallback(std::function<void(Source&, std::span<const uint8_t> data)> fn) override
        {
            m_captureFun = std::move(fn);
        }
        void InvokeCaptureCallback(std::span<const std::uint8_t> data)
        {
            if (m_captureFun)
                m_captureFun(*this, data);
        }

        ~SourceCommon() override = default;
    };
}
#endif
