#pragma once
#include <linux/videodev2.h>

#include "cemu_capture.hpp"


class V4L2EnumerationHandle : public cemu_capture::EnumerationHandle
{
public:
    V4L2EnumerationHandle(std::string path, const v4l2_capability& caps)
        : m_path(std::move(path)), m_name(reinterpret_cast<const char*>(caps.card)),
          m_uniqueString(m_path + m_name + reinterpret_cast<const char*>(caps.bus_info))
    {
    }
    ~V4L2EnumerationHandle() override = default;

    std::string_view GetName() const override
    {
        return m_name;
    }

    std::string_view GetUniqueString() const override
    {
        return m_path + m_name;
    }

    std::string_view GetPath() const
    {
        return m_path;
    }

private:
    std::string m_path;
    std::string m_name;
    std::string m_uniqueString;
};
