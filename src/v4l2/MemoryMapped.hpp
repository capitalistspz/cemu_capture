#ifndef CEMU_CAPTURE_MEMORY_MAPPED_HPP
#define CEMU_CAPTURE_MEMORY_MAPPED_HPP
#include <span>
#include <sys/mman.h>

namespace cemu_capture
{
    template <typename T>
    class MemoryMapped
    {
    public:
        MemoryMapped() noexcept : m_data(static_cast<T*>(MAP_FAILED)), m_size(0)
        {
        }

        MemoryMapped(int fd, std::size_t size, int prot, int mode, long offset, void* addr = nullptr) noexcept
            : m_data(static_cast<T*>(::mmap(addr, size, prot, mode, fd, offset))), m_size(size)
        {
        }

        MemoryMapped(const MemoryMapped&) = delete;

        MemoryMapped(MemoryMapped&& other) noexcept :
            m_data(other.m_data),
            m_size(other.m_size)
        {
            other.m_data = static_cast<T*>(MAP_FAILED);
            other.m_size = 0;
        }

        ~MemoryMapped() noexcept
        {
            if (m_data != static_cast<T*>(MAP_FAILED))
                ::munmap(m_data, m_size);
        }

        [[nodiscard]] bool is_valid() const noexcept
        {
            return m_data != static_cast<T*>(MAP_FAILED);
        }

        [[nodiscard]] T* data() noexcept
        {
            return m_data;
        }

        [[nodiscard]] const T* data() const noexcept
        {
            return m_data;
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return m_size;
        }

        T* begin() noexcept
        {
            return m_data;
        }

        const T* cbegin() const noexcept
        {
            return m_data;
        }

        T* end() noexcept
        {
            return m_data + m_size;
        }

        const T* cend() const noexcept
        {
            return m_data + m_size;
        }

        explicit(false) operator std::span<T>() noexcept
        {
            return std::span(m_data, m_size);
        }

        explicit(false) operator std::span<const T>() const noexcept
        {
            return std::span(m_data, m_size);
        }

    private:
        T* m_data;
        std::size_t m_size;
    };
}
#endif
