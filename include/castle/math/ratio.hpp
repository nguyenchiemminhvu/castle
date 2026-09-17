// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ratio.hpp
 * @brief Compile-time rational arithmetic and convenience period aliases.
 *
 * This header represents rational numbers as reduced `intmax_t` numerator and
 * denominator pairs known entirely at compile time. Use it for unit scaling,
 * chrono-style periods, and template metaprogramming where runtime arithmetic
 * would be unnecessary.
 *
 * Example:
 * @code
 * using half = castle::math::ratio<2, 4>;
 * using third = castle::math::ratio<1, 3>;
 * using sum = castle::math::ratio_add_t<half, third>;
 * static_assert(sum::num == 5 && sum::den == 6, "ratio add");
 * @endcode
 *
 * @warning Intermediate compile-time products are evaluated in `intmax_t` and
 *          can overflow for sufficiently large operands.
 */
#ifndef CASTLE_MATH_RATIO_HPP
#define CASTLE_MATH_RATIO_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/gcd.hpp"

#include <stdint.h>

namespace castle
{
namespace math
{

/**
 * @brief Represents a reduced rational number at compile time.
 * @tparam Numerator Numerator of the rational value.
 * @tparam Denominator Denominator of the rational value. Defaults to `1`.
 * @note The sign is normalized so `den` is always non-negative.
 * @warning `Denominator` must not be zero.
 */
template <intmax_t Numerator, intmax_t Denominator = 1>
struct ratio
{
    static_assert(Denominator != 0,
                  "ratio denominator cannot be zero");

private:
    static CASTLE_CONSTEXPR intmax_t abs_num = (Numerator < 0) ? -Numerator : Numerator;
    static CASTLE_CONSTEXPR intmax_t abs_den = (Denominator < 0) ? -Denominator : Denominator;
    static CASTLE_CONSTEXPR intmax_t divisor = gcd<abs_num, abs_den>::value;
    static CASTLE_CONSTEXPR intmax_t sign = (Denominator < 0) ? -1LL : 1LL;

public:
    /**
     * @brief Reduced numerator.
     */
    static CASTLE_CONSTEXPR intmax_t num = sign * (Numerator / divisor);

    /**
     * @brief Reduced denominator.
     */
    static CASTLE_CONSTEXPR intmax_t den = abs_den / divisor;

    /**
     * @brief Canonical reduced ratio type.
     */
    using type = ratio<num, den>;
};

/**
 * @brief Adds two compile-time ratios.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 * @note The result is provided as the nested `type`.
 */
template <typename TRatio1, typename TRatio2>
struct ratio_add
{
    using type = ratio<
        (TRatio1::num * TRatio2::den) +
        (TRatio2::num * TRatio1::den),
        TRatio1::den * TRatio2::den>;
};

/**
 * @brief Alias for the reduced result of `ratio_add`.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 */
template <typename TRatio1, typename TRatio2>
using ratio_add_t = typename ratio_add<TRatio1, TRatio2>::type;

/**
 * @brief Subtracts one compile-time ratio from another.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 * @note The result is provided as the nested `type`.
 */
template <typename TRatio1, typename TRatio2>
struct ratio_subtract
{
    using type = ratio<
        (TRatio1::num * TRatio2::den) -
        (TRatio2::num * TRatio1::den),
        TRatio1::den * TRatio2::den>;
};

/**
 * @brief Alias for the reduced result of `ratio_subtract`.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 */
template <typename TRatio1, typename TRatio2>
using ratio_subtract_t = typename ratio_subtract<TRatio1, TRatio2>::type;

/**
 * @brief Multiplies two compile-time ratios.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 * @note The result is provided as the nested `type`.
 */
template <typename TRatio1, typename TRatio2>
struct ratio_multiply
{
    using type = ratio<
        TRatio1::num * TRatio2::num,
        TRatio1::den * TRatio2::den>;
};

/**
 * @brief Alias for the reduced result of `ratio_multiply`.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 */
template <typename TRatio1, typename TRatio2>
using ratio_multiply_t = typename ratio_multiply<TRatio1, TRatio2>::type;

/**
 * @brief Divides one compile-time ratio by another.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 * @note The result is provided as the nested `type`.
 * @warning If `TRatio2::num` is zero, the resulting ratio denominator becomes
 *          zero and triggers the `ratio` static assertion.
 */
template <typename TRatio1, typename TRatio2>
struct ratio_divide
{
    using type = ratio<
        TRatio1::num * TRatio2::den,
        TRatio1::den * TRatio2::num>;
};

/**
 * @brief Alias for the reduced result of `ratio_divide`.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 */
template <typename TRatio1, typename TRatio2>
using ratio_divide_t = typename ratio_divide<TRatio1, TRatio2>::type;

/**
 * @brief Tests two compile-time ratios for equality.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 * @note Comparison is performed on the reduced numerator/denominator pairs.
 */
template <typename TRatio1, typename TRatio2>
struct ratio_equal
{
    static CASTLE_CONSTEXPR bool value =
        (TRatio1::num == TRatio2::num) &&
        (TRatio1::den == TRatio2::den);
};

/**
 * @brief Tests two compile-time ratios for inequality.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 */
template <typename TRatio1, typename TRatio2>
struct ratio_not_equal
{
    static CASTLE_CONSTEXPR bool value = !ratio_equal<TRatio1, TRatio2>::value;
};

/**
 * @brief Tests whether one compile-time ratio is less than another.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 * @warning The cross-products are evaluated in `intmax_t`.
 */
template <typename TRatio1, typename TRatio2>
struct ratio_less
{
    static CASTLE_CONSTEXPR bool value =
        (TRatio1::num * TRatio2::den) <
        (TRatio2::num * TRatio1::den);
};

/**
 * @brief Tests whether one compile-time ratio is less than or equal to another.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 */
template <typename TRatio1, typename TRatio2>
struct ratio_less_equal
{
    static CASTLE_CONSTEXPR bool value = !ratio_less<TRatio2, TRatio1>::value;
};

/**
 * @brief Tests whether one compile-time ratio is greater than another.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 */
template <typename TRatio1, typename TRatio2>
struct ratio_greater
{
    static CASTLE_CONSTEXPR bool value = ratio_less<TRatio2, TRatio1>::value;
};

/**
 * @brief Tests whether one compile-time ratio is greater than or equal to another.
 * @tparam TRatio1 Left-hand ratio type.
 * @tparam TRatio2 Right-hand ratio type.
 */
template <typename TRatio1, typename TRatio2>
struct ratio_greater_equal
{
    static CASTLE_CONSTEXPR bool value = !ratio_less<TRatio1, TRatio2>::value;
};

} // namespace math

/**
 * @brief Common SI-scale and time-period ratio aliases.
 * @note These aliases live directly in namespace `castle` for chrono-style use.
 */
using atto  = math::ratio<1LL, 1000000000000000000LL>;
using femto = math::ratio<1LL, 1000000000000000LL>;
using pico  = math::ratio<1LL, 1000000000000LL>;
using nano  = math::ratio<1LL, 1000000000LL>;
using micro = math::ratio<1LL, 1000000LL>;
using milli = math::ratio<1LL, 1000LL>;
using centi = math::ratio<1LL, 100LL>;
using deci  = math::ratio<1LL, 10LL>;

using deca  = math::ratio<10LL, 1LL>;
using hecto = math::ratio<100LL, 1LL>;
using kilo  = math::ratio<1000LL, 1LL>;
using mega  = math::ratio<1000000LL, 1LL>;
using giga  = math::ratio<1000000000LL, 1LL>;

using seconds = math::ratio<1LL, 1LL>;
using minutes = math::ratio<60LL, 1LL>;
using hours   = math::ratio<3600LL, 1LL>;
using days    = math::ratio<86400LL, 1LL>;
using weeks   = math::ratio<604800LL, 1LL>;

} // namespace castle

#endif // CASTLE_MATH_RATIO_HPP
