// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file round.hpp
 * @brief Floating-point to integer rounding, ties away from zero.
 * @details Quantising a physical value onto an integer grid (fixed-point
 * fields, protocol data fields, ADC/DAC codes) needs one well-defined rounding
 * rule without pulling in `<math.h>` `lround()` and its `long` result type.
 * `round_half_away()` rounds to the nearest integer and resolves `x.5` ties
 * away from zero, the rule of C `round()`, directly into the caller's integer
 * type.
 *
 * @code
 * #include "castle/math/round.hpp"
 *
 * constexpr int32_t up = castle::math::round_half_away<int32_t>(2.5);    // 3
 * constexpr int32_t down = castle::math::round_half_away<int32_t>(-2.5); // -3
 * const int64_t units = castle::math::round_half_away<int64_t>(1.23456 / 0.0001); // 12346
 * @endcode
 */

#ifndef CASTLE_MATH_ROUND_HPP
#define CASTLE_MATH_ROUND_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Rounds a floating-point value to the nearest integer, ties away from zero.
 * @tparam Int Destination integer type.
 * @tparam Float Floating-point source type (deduced).
 * @param value Value to round.
 * @return The nearest `Int`; `2.5 -> 3`, `-2.5 -> -3`.
 * @note `O(1)`, `constexpr`, no hidden state.
 * @warning `value` must be finite and its rounded result must fit `Int`;
 * otherwise the conversion is undefined behaviour. Range-check (and reject
 * NaN) before calling.
 */
template <typename Int, typename Float>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<Int>::value && meta::is_floating_point<Float>::value, Int>::type
round_half_away(Float value) CASTLE_NOEXCEPT
{
    CASTLE_CONSTEXPR Float half = static_cast<Float>(0.5);
    return static_cast<Int>((value >= static_cast<Float>(0)) ? (value + half) : (value - half)); // LCOV_EXCL_BR_LINE
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_ROUND_HPP
