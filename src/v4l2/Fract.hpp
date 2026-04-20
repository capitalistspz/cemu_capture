#ifndef CEMU_CAPTURE_V4L2_FRACT_HPP
#define CEMU_CAPTURE_V4L2_FRACT_HPP
#include <linux/videodev2.h>
#include <cstdint>

namespace cemu_capture
{
    template <typename I>
    constexpr I gcd(I a, I b)
    {
        I res = 1;
        for (I d = 1; d <= a && d <= b; ++d)
        {
            if (((a % d) == 0) && ((b % d) == 0))
                res = d;
        }
        return res;
    }

    constexpr v4l2_fract FramerateDoubleToIntervalFract(double framerate)
    {
        return {32768, static_cast<uint32_t>(framerate * 32768)};
    }

    constexpr double IntervalFractToFramerateDouble(v4l2_fract f)
    {
        return static_cast<double>(f.denominator) / f.numerator;
    }

    constexpr v4l2_fract Simplify(v4l2_fract a)
    {
        auto common = gcd(a.numerator, a.denominator);

        return {a.numerator / common, a.denominator / common};
    }

    constexpr static v4l2_fract operator+(v4l2_fract a, v4l2_fract b)
    {
        v4l2_fract res{a.numerator * b.denominator + a.denominator * b.numerator, a.denominator * b.denominator};

        return Simplify(res);
    }

    constexpr bool operator==(v4l2_fract a, v4l2_fract b)
    {
        auto as = Simplify(a);
        auto bs = Simplify(b);
        return as.numerator == bs.numerator && as.denominator == bs.denominator;
    }

    static_assert(v4l2_fract{1, 6} + v4l2_fract{1, 4} == v4l2_fract{5, 12});
}
#endif
