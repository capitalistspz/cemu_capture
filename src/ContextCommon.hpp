#ifndef CEMU_CAPTURE_CONTEXT_COMMON_HPP
#define CEMU_CAPTURE_CONTEXT_COMMON_HPP
#include <../include/cemu_capture.hpp>

#include <format>
#include <functional>
#include <string_view>
#include <utility>

namespace cemu_capture
{
    class ContextCommon : public Context
    {
        std::function<void(LogLevel, std::string_view)> m_logFun;
    public:
        template <typename... Args>
        void Log(LogLevel level, std::format_string<Args...> fmt, Args&&... args)
        {
            if (m_logFun)
                m_logFun(level, std::format(fmt, std::forward<Args>(args)...));
        }

        void SetLogCallback(std::function<void(LogLevel, std::string_view)> fn) final
        {
            m_logFun = std::move(fn);
        }
        ~ContextCommon() override = default;
    };
}
#endif
