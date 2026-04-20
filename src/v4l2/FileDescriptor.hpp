#ifndef CEMU_CAPTURE_FILE_DESCRIPTOR_HPP
#define CEMU_CAPTURE_FILE_DESCRIPTOR_HPP
#include <cerrno>
#include <utility>
#include <unistd.h>
#include <sys/ioctl.h>

namespace cemu_capture
{
    class FileDescriptor
    {
    public:
        explicit FileDescriptor(int fd) noexcept : m_fd(fd)
        {
        }

        FileDescriptor(const FileDescriptor&) = delete;

        FileDescriptor(FileDescriptor&& other) noexcept : m_fd(std::exchange(other.m_fd, -1))
        {
        }

        ~FileDescriptor() noexcept { if (m_fd >= 0) ::close(m_fd); }

        int XIoctl(unsigned long request, auto... args)
        {
            while (true)
            {
                auto res = ::ioctl(m_fd, request, std::forward<decltype(args)>(args)...);
                if (res == -1 && errno == EINTR)
                    continue;
                return res;
            }
        }

        [[nodiscard]] int Get() const noexcept { return m_fd; }
        [[nodiscard]] bool Invalid() const noexcept { return m_fd < 0; }

    private:
        int m_fd;
    };
}
#endif
