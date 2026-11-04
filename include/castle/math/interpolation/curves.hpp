// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file curves.hpp
 * @brief Stateless interpolation kernels: bilinear, cubic Hermite, Catmull-Rom and cubic Bezier.
 * @details These are the closed-form building blocks used by the table-based
 * interpolators in this module and are equally useful on their own for
 * animation, path smoothing and calibration maps. Every function is a pure,
 * allocation-free `O(1)` evaluation of a polynomial in the normalized
 * parameter `t`; `t` is intentionally not clamped, so values outside `[0, 1]`
 * extrapolate the polynomial. Linear interpolation itself lives in
 * `castle/math/interpolation/lerp.hpp`.
 *
 * @code
 * #include "castle/math/interpolation/curves.hpp"
 *
 * constexpr float mid = castle::math::catmull_rom(0.0F, 1.0F, 2.0F, 3.0F, 0.5F); // 1.5
 * constexpr float cell = castle::math::bilinear(0.0F, 1.0F, 2.0F, 3.0F, 0.5F, 0.5F); // 1.5
 * @endcode
 */
#ifndef CASTLE_MATH_INTERPOLATION_CURVES_HPP
#define CASTLE_MATH_INTERPOLATION_CURVES_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/interpolation/lerp.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Bilinear interpolation inside one unit cell.
 * @tparam T Floating-point type.
 * @param f00 Value at `(x, y) = (0, 0)`.
 * @param f10 Value at `(1, 0)`.
 * @param f01 Value at `(0, 1)`.
 * @param f11 Value at `(1, 1)`.
 * @param tx Normalized position along x.
 * @param ty Normalized position along y.
 * @return Interpolated value; `lerp` along x on both rows, then along y.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
bilinear(T f00, T f10, T f01, T f11, T tx, T ty) CASTLE_NOEXCEPT
{
    return castle::math::lerp(castle::math::lerp(f00, f10, tx),
                              castle::math::lerp(f01, f11, tx),
                              ty);
}

/**
 * @brief Evaluates a cubic Hermite segment.
 * @tparam T Floating-point type.
 * @param p0 Value at `t = 0`.
 * @param m0 Tangent at `t = 0`, expressed per unit of `t`.
 * @param p1 Value at `t = 1`.
 * @param m1 Tangent at `t = 1`, expressed per unit of `t`.
 * @param t Normalized parameter.
 * @return The unique cubic matching both end values and end tangents.
 * @note For a segment of width `h` in x, pass `h * slope` as the tangents.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
cubic_hermite(T p0, T m0, T p1, T m1, T t) CASTLE_NOEXCEPT
{
    CASTLE_CONST T t2 = t * t;
    CASTLE_CONST T t3 = t2 * t;
    return (static_cast<T>(2) * t3 - static_cast<T>(3) * t2 + static_cast<T>(1)) * p0
         + (t3 - static_cast<T>(2) * t2 + t) * m0
         + (static_cast<T>(3) * t2 - static_cast<T>(2) * t3) * p1
         + (t3 - t2) * m1;
}

/**
 * @brief Evaluates the derivative of a cubic Hermite segment with respect to `t`.
 * @tparam T Floating-point type.
 * @param p0 Value at `t = 0`.
 * @param m0 Tangent at `t = 0`, expressed per unit of `t`.
 * @param p1 Value at `t = 1`.
 * @param m1 Tangent at `t = 1`, expressed per unit of `t`.
 * @param t Normalized parameter.
 * @return `d/dt` of `cubic_hermite(p0, m0, p1, m1, t)`.
 * @note Divide by the segment width `h` to obtain the slope with respect to x.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
cubic_hermite_derivative(T p0, T m0, T p1, T m1, T t) CASTLE_NOEXCEPT
{
    CASTLE_CONST T t2 = t * t;
    return (static_cast<T>(6) * t2 - static_cast<T>(6) * t) * p0
         + (static_cast<T>(3) * t2 - static_cast<T>(4) * t + static_cast<T>(1)) * m0
         + (static_cast<T>(6) * t - static_cast<T>(6) * t2) * p1
         + (static_cast<T>(3) * t2 - static_cast<T>(2) * t) * m1;
}

/**
 * @brief Evaluates a uniform Catmull-Rom segment between `p1` and `p2`.
 * @tparam T Floating-point type.
 * @param p0 Control value before the segment.
 * @param p1 Value at `t = 0`.
 * @param p2 Value at `t = 1`.
 * @param p3 Control value after the segment.
 * @param t Normalized parameter along `p1 -> p2`.
 * @return Interpolated value; tangents are `(p2 - p0) / 2` and `(p3 - p1) / 2`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
catmull_rom(T p0, T p1, T p2, T p3, T t) CASTLE_NOEXCEPT
{
    return castle::math::cubic_hermite(p1,
                                       static_cast<T>(0.5) * (p2 - p0),
                                       p2,
                                       static_cast<T>(0.5) * (p3 - p1),
                                       t);
}

/**
 * @brief Evaluates a cubic Bezier curve.
 * @tparam T Floating-point type.
 * @param p0 Start control value (reached at `t = 0`).
 * @param p1 First handle.
 * @param p2 Second handle.
 * @param p3 End control value (reached at `t = 1`).
 * @param t Normalized parameter.
 * @return Bernstein-weighted combination of the four control values.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
cubic_bezier(T p0, T p1, T p2, T p3, T t) CASTLE_NOEXCEPT
{
    CASTLE_CONST T u = static_cast<T>(1) - t;
    CASTLE_CONST T three = static_cast<T>(3);
    return u * u * u * p0
         + three * u * u * t * p1
         + three * u * t * t * p2
         + t * t * t * p3;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_INTERPOLATION_CURVES_HPP
