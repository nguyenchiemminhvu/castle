// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sign.hpp
 * @brief Small helper for extracting the mathematical sign of a value.
 *
 * Use this function when control logic, steering decisions, or direction flags
 * need a compact `-1`, `0`, or `+1` result without allocating state or
 * depending on larger numeric utilities.
 *
 * Example:
 * @code
 * const int dir_a = castle::math::sign(-3);
 * const int dir_b = castle::math::sign(0U);
 * (void)dir_a;
 * (void)dir_b;
 * @endcode
 */
#ifndef CASTLE_MATH_SIGN_HPP
#define CASTLE_MATH_SIGN_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Returns the sign of a comparable value.
 * @tparam T Type supporting comparison with `T{0}`.
 * @param value Value to classify.
 * @return `-1` when `value < 0`, `0` when `value == 0`, and `+1` when
 *         `value > 0`.
 * @note Unsigned types can only produce `0` or `+1`.
 * @warning The template assumes `T{0}`, `value > T{0}`, and `value < T{0}`
 *          are all valid expressions.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
int sign(T value) CASTLE_NOEXCEPT
{
    return (value > T{0}) ? 1 : ((value < T{0}) ? -1 : 0);
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_SIGN_HPP
