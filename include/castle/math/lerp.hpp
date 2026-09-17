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
 * #include "castle/math/lerp.hpp"
 *
 * constexpr float midpoint = castle::math::lerp(10.0f, 20.0f, 0.5f);
 * constexpr float future = castle::math::lerp(10.0f, 20.0f, 1.5f);
 * @endcode
 */

#ifndef CASTLE_MATH_LERP_HPP
#define CASTLE_MATH_LERP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

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

}
}

#endif
