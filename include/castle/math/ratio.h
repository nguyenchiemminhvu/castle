#ifndef CASTLE_MATH_RATIO_H
#define CASTLE_MATH_RATIO_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/math/gcd.h"

#include <stdint.h>

namespace castle
{
namespace math
{

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
    static CASTLE_CONSTEXPR intmax_t num = sign * (Numerator / divisor);
    static CASTLE_CONSTEXPR intmax_t den = abs_den / divisor;

    using type = ratio<num, den>;
};

template <typename TRatio1, typename TRatio2>
struct ratio_add
{
    using type = ratio<
        (TRatio1::num * TRatio2::den) +
        (TRatio2::num * TRatio1::den),
        TRatio1::den * TRatio2::den>;
};

template <typename TRatio1, typename TRatio2>
using ratio_add_t = typename ratio_add<TRatio1, TRatio2>::type;

template <typename TRatio1, typename TRatio2>
struct ratio_subtract
{
    using type = ratio<
        (TRatio1::num * TRatio2::den) -
        (TRatio2::num * TRatio1::den),
        TRatio1::den * TRatio2::den>;
};

template <typename TRatio1, typename TRatio2>
using ratio_subtract_t = typename ratio_subtract<TRatio1, TRatio2>::type;

template <typename TRatio1, typename TRatio2>
struct ratio_multiply
{
    using type = ratio<
        TRatio1::num * TRatio2::num,
        TRatio1::den * TRatio2::den>;
};

template <typename TRatio1, typename TRatio2>
using ratio_multiply_t = typename ratio_multiply<TRatio1, TRatio2>::type;

template <typename TRatio1, typename TRatio2>
struct ratio_divide
{
    using type = ratio<
        TRatio1::num * TRatio2::den,
        TRatio1::den * TRatio2::num>;
};

template <typename TRatio1, typename TRatio2>
using ratio_divide_t = typename ratio_divide<TRatio1, TRatio2>::type;

template <typename TRatio1, typename TRatio2>
struct ratio_equal
{
    static CASTLE_CONSTEXPR bool value =
        (TRatio1::num == TRatio2::num) &&
        (TRatio1::den == TRatio2::den);
};

template <typename TRatio1, typename TRatio2>
struct ratio_not_equal
{
    static CASTLE_CONSTEXPR bool value = !ratio_equal<TRatio1, TRatio2>::value;
};

template <typename TRatio1, typename TRatio2>
struct ratio_less
{
    static CASTLE_CONSTEXPR bool value =
        (TRatio1::num * TRatio2::den) <
        (TRatio2::num * TRatio1::den);
};

template <typename TRatio1, typename TRatio2>
struct ratio_less_equal
{
    static CASTLE_CONSTEXPR bool value = !ratio_less<TRatio2, TRatio1>::value;
};

template <typename TRatio1, typename TRatio2>
struct ratio_greater
{
    static CASTLE_CONSTEXPR bool value = ratio_less<TRatio2, TRatio1>::value;
};

template <typename TRatio1, typename TRatio2>
struct ratio_greater_equal
{
    static CASTLE_CONSTEXPR bool value = !ratio_less<TRatio1, TRatio2>::value;
};

} // namespace math

// ----------------------------------------------------------------------------
// Chrono convenience ratios
// ----------------------------------------------------------------------------
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

#endif // CASTLE_MATH_RATIO_H
