#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

#include <linux/videodev2.h>

#include "V4L2Source.hpp"
#include "V4L2Context.hpp"
#include "V4L2Ioctls.hpp"
#include "FileDescriptor.hpp"

#include "../util.hpp"

#ifdef CEMU_CAPTURE_USE_LIBSYSTEMD
#include <systemd/sd-device.h>
#endif

namespace cemu_capture
{
    struct EpollData
    {
        std::weak_ptr<V4L2Source> device;
    };

    static bool CanCapture(const v4l2_capability& capability)
    {
        constexpr static auto CAPTURE_CAPS_FLAGS = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE;
        if (capability.capabilities & V4L2_CAP_DEVICE_CAPS)
            return capability.device_caps & CAPTURE_CAPS_FLAGS;
        return capability.capabilities & CAPTURE_CAPS_FLAGS;
    }

    std::shared_ptr<Context> Context::Create()
    {
        return std::make_shared<V4L2Context>();
    }

    V4L2Context::V4L2Context() : m_epollFd(epoll_create1(0))
    {
        if (m_epollFd.Invalid())
        {
            throw std::runtime_error(
                std::string("Failed to create epoll file descriptor: ") + std::strerror(errno));
        }
        m_thread = std::jthread(&V4L2Context::ThreadFunc, this);
        if constexpr(std::same_as<std::jthread::native_handle_type, pthread_t>)
            pthread_setname_np(m_thread.native_handle(), "cemu_capture");
    }

    V4L2Context::~V4L2Context()
    {
        m_thread.request_stop();
        m_hasNewEvent.notify_one();
        m_thread.join();
    }

    std::optional<SourceInfo> V4L2Context::GetDeviceInfo(FileDescriptor& fd, std::string_view deviceFilePath)
    {
        v4l2_capability caps{};
        const auto res = vidioc::querycap(fd, &caps);
        if (res != 0)
        {
            Log(LogLevel::Warning, "Failed to get device info: couldn't query capabilities of {}: {}", deviceFilePath,
                std::strerror(errno));
            return std::nullopt;
        }

        if (CanCapture(caps))
        {
            if (caps.capabilities & V4L2_CAP_STREAMING)
                return SourceInfo{std::string(deviceFilePath), reinterpret_cast<const char*>(caps.card)};
            Log(LogLevel::Warning, "Device {} is capable of video capture, but lacks streaming capability",
                deviceFilePath);
        }
        return std::nullopt;
    }
#ifdef CEMU_CAPTURE_USE_LIBSYSTEMD
    std::vector<SourceInfo> V4L2Context::EnumerateSources()
    {
        std::vector<SourceInfo> deviceInfos;

        unique_ptr_cd<sd_device_enumerator, sd_device_enumerator_unref> enumerator;
        auto res = sd_device_enumerator_new(out_ptr(enumerator));
        if (res < 0)
        {
            Log(LogLevel::Error, "Failed to create device enumerator");
            return deviceInfos;
        }
        res = sd_device_enumerator_add_match_subsystem(enumerator.get(), "video4linux", 1);

        if (res < 0)
        {
            Log(LogLevel::Error, "Failed to set match subsystem during enumeration");
            return deviceInfos;
        }

        for (auto it = sd_device_enumerator_get_device_first(enumerator.get()); it != nullptr; it =
             sd_device_enumerator_get_device_next(enumerator.get()))
        {
            const char* path = nullptr;
            res = sd_device_get_devname(it, &path);
            if (res < 0)
                continue;
            auto fd = FileDescriptor(::open(path, O_RDONLY));
            auto devInfo = GetDeviceInfo(fd, path);
            if (devInfo)
                deviceInfos.emplace_back(std::move(*devInfo));
        }

        return deviceInfos;
    }
#else
    std::vector<DeviceInfo> V4L2Context::EnumerateSources()
    {
        constexpr static auto MAX_DEVICE_PATH_LENGTH = 256;
        constexpr static auto MAX_DEVICE_NO = 128;
        std::vector<DeviceInfo> deviceInfos;
        auto dir = unique_ptr_cd<DIR, ::closedir>(::opendir("/dev"));
        if (!dir)
        {
            LogError("Failed to open /dev directory: {}", std::strerror(errno));
            return deviceInfos;
        }

        dirent* entry = nullptr;
        auto index = 0u;
        while ((entry = ::readdir(dir.get())))
        {
            if (index++ == MAX_DEVICE_NO)
                break;
            if (std::strncmp(entry->d_name, "video", 5) == 0)
            {
                std::array<char, MAX_DEVICE_PATH_LENGTH> name{};
                std::snprintf(name.data(), name.size(), "/dev/%s", entry->d_name);
                auto fd = file_descriptor(::open(name, O_RDONLY));
                auto devInfo = GetDeviceInfo(fd, name);
                if (devInfo)
                    deviceInfos.emplace_back(std::move(*devInfo));
            }
        }
        return deviceInfos;
    }
#endif
    void V4L2Context::AddDevice(const std::shared_ptr<V4L2Source>& device)
    {
        epoll_event ev{
            .events = EPOLLIN,
            .data = {.ptr = new EpollData{device}}
        };
        m_eventMutex.lock();
        if (epoll_ctl(m_epollFd.Get(), EPOLL_CTL_ADD, device->GetFd(), &ev) == 0)
        {
            m_newEvents.push_back(ev);
            m_eventMutex.unlock();
            m_hasNewEvent.test_and_set();
            m_hasNewEvent.notify_one();
        }
        else
        {
            m_eventMutex.unlock();
            Log(LogLevel::Error, "Failed to add device to epoll: {}", std::strerror(errno));
            delete static_cast<EpollData*>(ev.data.ptr);
        }
    }

    std::shared_ptr<Source> V4L2Context::OpenDevice(const std::string& id)
    {
        auto fd = FileDescriptor(::open(id.c_str(), O_RDWR));

        if (fd.Invalid())
        {
            throw std::runtime_error(std::string("Failed to open device: ") + std::strerror(errno));
        }

        auto info = GetDeviceInfo(fd, id);
        if (!info)
        {
            throw std::runtime_error("Could not query device capabilities or does not support streaming");
        }

        return std::make_shared<V4L2Source>(shared_from_this(), std::move(fd), *info);
    }

    void V4L2Context::ThreadFunc(std::stop_token stopToken)
    {
        std::vector<epoll_event> events;
        while (!stopToken.stop_requested())
        {
            if (events.empty())
            {
                // There's nothing to do if there aren't any events
                m_hasNewEvent.wait(false);
                if (stopToken.stop_requested())
                    break;
                std::unique_lock lock(m_eventMutex);
                std::ranges::copy(m_newEvents, std::back_inserter(events));
                m_newEvents.clear();
            }
            else if (m_hasNewEvent.test()) {
                std::unique_lock lock(m_eventMutex);
                assert(!m_newEvents.empty());
                std::ranges::copy(m_newEvents, std::back_inserter(events));
                m_newEvents.clear();
            }

            const auto nfds = epoll_wait(m_epollFd.Get(), events.data(), static_cast<int>(events.size()), 100);
            if (nfds == 0)
                continue;
            if (nfds == -1)
            {
                if (errno == EINTR)
                    continue;
                throw std::runtime_error(std::string("epoll wait failed: ") + std::strerror(errno));
            }
            bool anyErrored = false;
            for (auto& event : events)
            {
                if (event.events & EPOLLERR)
                {
                    anyErrored = true;
                }
                else if ((event.events & EPOLLIN))
                {
                    const auto data = static_cast<EpollData*>(event.data.ptr);
                    if (const auto device = data->device.lock())
                    {
                        device->UpdateData();
                    }
                }
            }

            if (anyErrored)
            {
                auto [beg, end] = std::ranges::remove_if(events, [this](epoll_event& ev)
                {
                    if (!(ev.events & EPOLLERR))
                        return false;
                    const auto data = static_cast<EpollData*>(ev.data.ptr);
                    if (const auto device = data->device.lock())
                    {
                        const auto err = epoll_ctl(m_epollFd.Get(), EPOLL_CTL_DEL, device->GetFd(),
                                                   &ev);
                        if (err == -1)
                            assert_perror(errno);
                        Log(LogLevel::Error, "Device {} closed", device->GetInfo().id);
                        delete data;
                    }
                    return true;
                });
                events.erase(beg, end);
            }
        }
    }
}
