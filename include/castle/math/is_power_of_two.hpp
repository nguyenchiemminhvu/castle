// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file is_power_of_two.hpp
 * @brief Tests whether an integral value is a non-zero power of two.
 * @details Uses the classic `value & (value - 1)` bit test for integral types
 * without depending on the STL. This is most useful for validating sizes that
 * enable mask-based arithmetic, such as ring buffers and page-aligned blocks.
 * The function is intended for non-negative inputs; signed negative values are
 * not meaningful powers of two in this API.
 *
 * @code
 * #include "castle/math/is_power_of_two.hpp"
 *
 * constexpr bool ok = castle::math::is_power_of_two(16U);
 * constexpr bool zero_case = castle::math::is_power_of_two(0U);
 * @endcode
 */

#ifndef CASTLE_MATH_IS_POWER_OF_TWO_HPP
#define CASTLE_MATH_IS_POWER_OF_TWO_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Returns whether an integral value is a non-zero power of two.
 * @tparam T Integral type.
 * @param value Value to test.
 * @return `true` when `value` is non-zero and has exactly one bit set;
 * otherwise `false`.
 * @note `0` is never considered a power of two, and `1` is considered a power
 * of two.
 * @warning Negative signed inputs are outside the intended contract; the
 * expression `value - 1` can overflow for the minimum signed value.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value, bool>::type
is_power_of_two(T value) CASTLE_NOEXCEPT
{
    return value != T{0} && (value & static_cast<T>(value - T{1})) == T{0};
}

}
}

#endif
