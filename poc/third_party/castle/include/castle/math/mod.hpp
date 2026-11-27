// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mod.hpp
 * @brief Positive modulo and interval wrapping helpers for integers.
 *
 * These functions normalize integral values into deterministic periodic ranges
 * without relying on implementation-specific negative-modulo conventions at
 * the call site. Use them for timer counters, phase accumulators, circular
 * indexes, and other wraparound domains.
 *
 * Example:
 * @code
 * const int phase = castle::math::positive_mod(-1, 8);
 * const int wrapped = castle::math::wrap(14, 10, 13);
 * (void)phase;
 * (void)wrapped;
 * @endcode
 *
 * @note `wrap()` operates on the half-open interval `[low, high)`.
 */
#ifndef CASTLE_MATH_MOD_HPP
#define CASTLE_MATH_MOD_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Computes a modulo result in the range `[0, modulus)`.
 * @tparam T Integral type.
 * @param value Value to normalize.
 * @param modulus Positive modulus.
 * @return Non-negative remainder equivalent to `value` modulo `modulus`.
 * @note This is useful when `%` on a negative dividend would otherwise produce
 *       a negative remainder.
 * @warning `modulus` must be greater than zero.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value, T>::type
positive_mod(T value, T modulus) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(modulus > T{0}, // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::math::positive_mod: modulus must be positive"));

    CASTLE_CONST T result = static_cast<T>(value % modulus);
    return (result < T{0}) ? static_cast<T>(result + modulus) : result;
}

/**
 * @brief Wraps an integer into the half-open interval `[low, high)`.
 * @tparam T Integral type.
 * @param value Value to wrap.
 * @param low Inclusive lower bound.
 * @param high Exclusive upper bound.
 * @return Value translated into the target interval with periodic wrapping.
 * @note Common 8/16/32-bit integer widths are widened internally before the
 *       interval arithmetic is performed.
 * @warning `high` must be greater than `low`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value, T>::type
wrap(T value, T low, T high) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(high > low, // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::math::wrap: high must be greater than low"));

    // Widen common embedded integer widths so the interval arithmetic stays defined.
    using calc_type = typename meta::conditional<
        meta::is_unsigned<T>::value && (sizeof(T) <= sizeof(uint32_t)),
        uint64_t,
        typename meta::conditional<
            meta::is_signed<T>::value && (sizeof(T) <= sizeof(int32_t)),
            int64_t,
            T>::type>::type;

    CASTLE_CONST calc_type shifted = static_cast<calc_type>(value) - static_cast<calc_type>(low);
    CASTLE_CONST calc_type period = static_cast<calc_type>(high) - static_cast<calc_type>(low);
    CASTLE_CONST calc_type wrapped = static_cast<calc_type>(positive_mod(shifted, period));
    return static_cast<T>(static_cast<calc_type>(low) + wrapped);
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_MOD_HPP
