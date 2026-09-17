// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file saturating.hpp
 * @brief Saturating add/subtract helpers for integral types.
 *
 * This header performs deterministic integer addition and subtraction while
 * clamping out-of-range results to the destination type's representable limits
 * instead of allowing wraparound. Use it for physical commands, counters, and
 * accumulators where overflow must remain bounded.
 *
 * Example:
 * @code
 * const uint8_t hi = castle::math::saturating_add<uint8_t>(250U, 20U);
 * const int8_t lo = castle::math::saturating_sub<int8_t>(-120, 20);
 * (void)hi;
 * (void)lo;
 * @endcode
 */
#ifndef CASTLE_MATH_SATURATING_HPP
#define CASTLE_MATH_SATURATING_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Adds two unsigned integers with saturation at the type maximum.
 * @tparam T Unsigned integral type.
 * @param a Left operand.
 * @param b Right operand.
 * @return `a + b`, or `numeric_limits<T>::max()` when the exact sum would
 *         overflow.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value && meta::is_unsigned<T>::value, T>::type
saturating_add(T a, T b) CASTLE_NOEXCEPT
{
    CASTLE_CONST T maximum = castle::numeric_limits<T>::max();
    return (b > static_cast<T>(maximum - a)) ? maximum : static_cast<T>(a + b);
}

/**
 * @brief Subtracts two unsigned integers with saturation at zero.
 * @tparam T Unsigned integral type.
 * @param a Left operand.
 * @param b Right operand.
 * @return `a - b`, or `0` when the exact result would underflow.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value && meta::is_unsigned<T>::value, T>::type
saturating_sub(T a, T b) CASTLE_NOEXCEPT
{
    return (b > a) ? T{0} : static_cast<T>(a - b);
}

/**
 * @brief Adds two signed integers with saturation at the type limits.
 * @tparam T Signed integral type.
 * @param a Left operand.
 * @param b Right operand.
 * @return Exact sum when representable; otherwise the signed minimum or
 *         maximum of `T`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value && meta::is_signed<T>::value, T>::type
saturating_add(T a, T b) CASTLE_NOEXCEPT
{
    CASTLE_CONST T maximum = castle::numeric_limits<T>::max();
    CASTLE_CONST T minimum = castle::numeric_limits<T>::min();

    if (b > T{0} && a > static_cast<T>(maximum - b))
    {
        return maximum;
    }
    if (b < T{0} && a < static_cast<T>(minimum - b))
    {
        return minimum;
    }
    return static_cast<T>(a + b);
}

/**
 * @brief Subtracts two signed integers with saturation at the type limits.
 * @tparam T Signed integral type.
 * @param a Left operand.
 * @param b Right operand.
 * @return Exact difference when representable; otherwise the signed minimum or
 *         maximum of `T`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value && meta::is_signed<T>::value, T>::type
saturating_sub(T a, T b) CASTLE_NOEXCEPT
{
    CASTLE_CONST T maximum = castle::numeric_limits<T>::max();
    CASTLE_CONST T minimum = castle::numeric_limits<T>::min();

    if (b > T{0} && a < static_cast<T>(minimum + b))
    {
        return minimum;
    }
    if (b < T{0} && a > static_cast<T>(maximum + b))
    {
        return maximum;
    }
    return static_cast<T>(a - b);
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_SATURATING_HPP
