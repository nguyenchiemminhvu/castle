// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file lerp.hpp
 * @brief Simple floating-point linear interpolation.
 * @details Computes `a + t * (b - a)` for floating-point types without
 * clamping `t`. This supports both interpolation (`0 <= t <= 1`) and
 * extrapolation (`t` outside that range). The formula is intentionally simple
 * and deterministic, but unlike `std::lerp` it does not apply extra steps to
 * reduce rounding error or intermediate overflow.
 *
 * @code
 * #include "castle/math/interpolation/lerp.hpp"
 *
 * constexpr float midpoint = castle::math::lerp(10.0f, 20.0f, 0.5f);
 * constexpr float future = castle::math::lerp(10.0f, 20.0f, 1.5f);
 * @endcode
 */

#ifndef CASTLE_MATH_INTERPOLATION_LERP_HPP
#define CASTLE_MATH_INTERPOLATION_LERP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/math/interpolation/knots.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Linearly interpolates between two floating-point values.
 * @tparam T Floating-point type.
 * @param a Start value used when `t == 0`.
 * @param b End value used when `t == 1`.
 * @param t Interpolation factor.
 * @return `a + t * (b - a)`.
 * @note `t` is intentionally not clamped, so values outside `[0, 1]` perform
 * extrapolation.
 * @warning This direct formula can lose precision or overflow for extreme
 * inputs more readily than a numerically specialized `lerp` implementation.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
lerp(T a, T b, T t) CASTLE_NOEXCEPT
{
    return a + t * (b - a);
}

/**
 * @brief Linearly interpolates a sampled curve at `x`.
 * @tparam T Floating-point type.
 * @param xs Strictly increasing abscissas, `count` values.
 * @param ys Ordinates, `count` values.
 * @param count Number of samples.
 * @param x Query position.
 * @return Interpolated value; `ys[0]` before the first sample and
 *         `ys[count - 1]` after the last. Returns zero when a pointer is null
 *         or `count` is zero.
 * @note `O(log count)`. `xs` is not validated, so it must be strictly increasing.
 * @warning Accuracy is that of the plain `lerp` formula; use `linear_table`
 * when validation or extrapolation policies are needed.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
linear_interpolate(CASTLE_CONST T* xs, CASTLE_CONST T* ys, size_type count, T x) CASTLE_NOEXCEPT
{
    if ((xs == nullptr) || (ys == nullptr) || (count == 0U))
    {
        return static_cast<T>(0);
    }

    if (x <= xs[0])
    {
        return ys[0];
    }

    CASTLE_CONST size_type last = count - 1U;
    if (x >= xs[last])
    {
        return ys[last];
    }

    CASTLE_CONST size_type i = detail::find_segment(xs, count, x);
    return castle::math::lerp(ys[i], ys[i + 1U], (x - xs[i]) / (xs[i + 1U] - xs[i]));
}

}
}

#endif
