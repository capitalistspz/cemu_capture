#ifndef CEMU_CAPTURE_UTIL_HPP
#define CEMU_CAPTURE_UTIL_HPP
#include <memory>
#include <type_traits>
#include <utility>


#if defined(__GNUC__)
#define UNREACHABLE __builtin_unreachable()
#elif defined(MSVC)
#define UNREACHABLE __assume(0)
#endif

namespace cemu_capture
{
    template <typename T, auto Deleter>
    using unique_ptr_cd = std::unique_ptr<T, decltype([](T* p) { Deleter(p); })>;

#if __cpp_lib_out_ptr >= 202106L
    using std::out_ptr;
#else
    template <typename SmartPointer>
    auto out_ptr(SmartPointer& p)
    {
        class out_ptr_t
        {
            SmartPointer& m_smartPtr;
            typename SmartPointer::pointer m_ptr = nullptr;

        public:
            explicit out_ptr_t(SmartPointer& s) : m_smartPtr(s)
            {
            }

            out_ptr_t(out_ptr_t&&) = delete;
            out_ptr_t(out_ptr_t const&) = delete;
            // ReSharper disable once CppNonExplicitConversionOperator
            explicit(false)
            operator std::add_pointer_t<typename SmartPointer::pointer>()
            { // NOLINT(*-explicit-constructor)
                return &m_ptr;
            }

            ~out_ptr_t() { m_smartPtr.reset(m_ptr); }
        };

        return out_ptr_t(p);
    }

    // Because references to packed vars are evil
    constexpr auto no_bind(auto v)
    {
        return v;
    }
#endif
}
#endif
