// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file square.hpp
 * @brief Small helper for squaring a value.
 *
 * This header returns `value * value` without widening or hidden policy
 * changes. Use it when naming the operation improves readability in embedded
 * formulas, calibration code, or constexpr arithmetic.
 *
 * Example:
 * @code
 * const int area_term = castle::math::square(12);
 * const float error_power = castle::math::square(1.5f);
 * (void)area_term;
 * (void)error_power;
 * @endcode
 */
#ifndef CASTLE_MATH_SQUARE_HPP
#define CASTLE_MATH_SQUARE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Returns the square of a value.
 * @tparam T Arithmetic type used for the multiplication.
 * @param value Input value.
 * @return `value * value`.
 * @note The result keeps type `T`; no widening or saturation is applied.
 * @warning Overflow and precision follow the normal rules of `T`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T square(T value) CASTLE_NOEXCEPT
{
    return value * value;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_SQUARE_HPP
