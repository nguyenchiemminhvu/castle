// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file powi.hpp
 * @brief Deterministic exponentiation by squaring for non-negative exponents.
 *
 * This header computes `base^exponent` in `O(log exponent)` multiplications,
 * making it suitable for embedded control math, fixed-point scaling, and
 * constexpr expressions where repeated multiplication would be unnecessarily
 * expensive.
 *
 * Example:
 * @code
 * const int gain = castle::math::powi(3, 4U);
 * const float squared = castle::math::powi(1.5f, 2U);
 * (void)gain;
 * (void)squared;
 * @endcode
 *
 * @note Arithmetic stays in `T`; there is no widening, saturation, or
 *       overflow detection.
 */
#ifndef CASTLE_MATH_POWI_HPP
#define CASTLE_MATH_POWI_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Raises a value to a non-negative integer exponent.
 * @tparam T Arithmetic type used for the multiplication chain.
 * @param base Base value.
 * @param exponent Non-negative exponent.
 * @return `base` multiplied by itself `exponent` times, or `T{1}` when
 *         `exponent == 0`.
 * @note Complexity is `O(log exponent)`.
 * @warning Overflow and precision loss follow the normal rules of `T`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T powi(T base, unsigned exponent) CASTLE_NOEXCEPT
{
    T result = T{1};
    T factor = base;
    unsigned remaining = exponent;

    while (remaining != 0U)
    {
        if ((remaining & 1U) != 0U)
        {
            result = static_cast<T>(result * factor);
        }
        remaining >>= 1U;
        if (remaining != 0U)
        {
            factor = static_cast<T>(factor * factor);
        }
    }
    return result;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_POWI_HPP
