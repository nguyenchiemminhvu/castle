#ifndef CASTLE_CHRONO_DURATION_H
#define CASTLE_CHRONO_DURATION_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/type_ranges.h"
#include "castle/math/gcd.h"
#include "castle/math/lcm.h"
#include "castle/math/ratio.h"

#include <float.h>
#include <limits.h>
#include <stdint.h>

namespace castle
{
namespace chrono
{

template <typename Ratio>
struct is_valid_period
    : castle::bool_constant<(Ratio::num > 0) && (Ratio::den > 0)>
{
};

// ============================================================================
// duration_values
// ============================================================================
// Supplies zero/min/max without depending on <limits> or another STL header.
// Custom representations may specialize this template when min()/max() are
// needed by user code.
// ============================================================================

template <typename Rep>
struct duration_values
{
    static CASTLE_CONSTEXPR Rep zero() CASTLE_NOEXCEPT
    {
        return Rep(0);
    }

    static CASTLE_CONSTEXPR Rep min() CASTLE_NOEXCEPT
    {
        return castle::numeric_limits<Rep>::lowest();
    }

    static CASTLE_CONSTEXPR Rep max() CASTLE_NOEXCEPT
    {
        return castle::numeric_limits<Rep>::max();
    }
};

// ============================================================================
// duration
//
// Fixed-ratio quantity of time.
// ============================================================================
template <typename Rep, typename Period = castle::math::ratio<1, 1>>
class duration;

template <typename ToDuration, typename Rep, typename Period>
CASTLE_CONSTEXPR ToDuration duration_cast(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT;

template <typename Rep, typename Period>
class duration
{
public:
    using rep = Rep;
    using period = Period;

    static_assert(is_valid_period<Period>::value,
                  "castle::chrono::duration Period must have positive num/den");

    CASTLE_CONSTEXPR duration() CASTLE_NOEXCEPT
        : value_(duration_values<rep>::zero())
    {
    }

    CASTLE_CONSTEXPR explicit duration(rep value) CASTLE_NOEXCEPT
        : value_(value)
    {
    }

    template <typename Rep2,
              typename meta::enable_if<
                  !(castle::is_integral<rep>::value &&
                    castle::is_floating_point<Rep2>::value), int>::type = 0>
    CASTLE_CONSTEXPR explicit duration(
        CASTLE_CONST duration<Rep2, Period>& other) CASTLE_NOEXCEPT
        : value_(static_cast<rep>(other.count()))
    {
    }

    template <typename Rep2, typename Period2,
              typename meta::enable_if<
                  !(castle::is_integral<rep>::value &&
                    castle::is_floating_point<Rep2>::value), int>::type = 0>
    CASTLE_CONSTEXPR explicit duration(
        CASTLE_CONST duration<Rep2, Period2>& other) CASTLE_NOEXCEPT
        : value_(duration_cast<duration>(other).count())
    {
    }

    duration(CASTLE_CONST duration&) CASTLE_DEFAULT;
    duration& operator=(CASTLE_CONST duration&) CASTLE_DEFAULT;

    CASTLE_NODISCARD CASTLE_CONSTEXPR rep count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return value_;
    }

    CASTLE_NODISCARD static CASTLE_CONSTEXPR duration zero() CASTLE_NOEXCEPT
    {
        return duration(duration_values<rep>::zero());
    }

    CASTLE_NODISCARD static CASTLE_CONSTEXPR duration min() CASTLE_NOEXCEPT
    {
        return duration(duration_values<rep>::min());
    }

    CASTLE_NODISCARD static CASTLE_CONSTEXPR duration max() CASTLE_NOEXCEPT
    {
        return duration(duration_values<rep>::max());
    }

    CASTLE_CONSTEXPR duration& operator++() CASTLE_NOEXCEPT
    {
        ++value_;
        return *this;
    }

    CASTLE_CONSTEXPR duration operator++(int) CASTLE_NOEXCEPT
    {
        duration temp(*this);
        ++value_;
        return temp;
    }

    CASTLE_CONSTEXPR duration& operator--() CASTLE_NOEXCEPT
    {
        --value_;
        return *this;
    }

    CASTLE_CONSTEXPR duration operator--(int) CASTLE_NOEXCEPT
    {
        duration temp(*this);
        --value_;
        return temp;
    }

    CASTLE_CONSTEXPR duration& operator+=(CASTLE_CONST duration& other) CASTLE_NOEXCEPT
    {
        value_ += other.value_;
        return *this;
    }

    CASTLE_CONSTEXPR duration& operator-=(CASTLE_CONST duration& other) CASTLE_NOEXCEPT
    {
        value_ -= other.value_;
        return *this;
    }

    CASTLE_CONSTEXPR duration& operator*=(CASTLE_CONST rep& scalar) CASTLE_NOEXCEPT
    {
        value_ *= scalar;
        return *this;
    }

    CASTLE_CONSTEXPR duration& operator/=(CASTLE_CONST rep& scalar) CASTLE_NOEXCEPT
    {
        value_ /= scalar;
        return *this;
    }

    CASTLE_CONSTEXPR duration& operator%=(CASTLE_CONST rep& scalar) CASTLE_NOEXCEPT
    {
        value_ %= scalar;
        return *this;
    }

    CASTLE_CONSTEXPR duration& operator%=(CASTLE_CONST duration& other) CASTLE_NOEXCEPT
    {
        value_ %= other.value_;
        return *this;
    }

private:
    rep value_;
};

// ============================================================================
// Standard duration aliases
// ============================================================================
// int64_t is intentional here. It gives long-running embedded schedulers a
// stable range and keeps the aliases independent from the target ABI's `long`.
// Applications with tighter constraints can use duration<uint32_t, ...> or
// another representation explicitly.
// ============================================================================
using nanoseconds  = duration<int64_t, castle::nano>;
using microseconds = duration<int64_t, castle::micro>;
using milliseconds = duration<int64_t, castle::milli>;
using seconds      = duration<int64_t, castle::seconds>;
using minutes      = duration<int64_t, castle::minutes>;
using hours        = duration<int64_t, castle::hours>;
using days         = duration<int64_t, castle::days>;
using weeks        = duration<int64_t, castle::weeks>;

// ============================================================================
// duration_cast
// ============================================================================
// Converts a duration between periods using integer/floating arithmetic of
// the common representation type. Integer conversions truncate toward zero,
// matching the behavior expected from duration_cast-style conversions.
// ============================================================================
template <typename ToDuration, typename Rep, typename Period>
CASTLE_CONSTEXPR ToDuration
duration_cast(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    using to_rep = typename ToDuration::rep;
    using to_period = typename ToDuration::period;
    using common_rep = castle::common_type_t<Rep, to_rep, intmax_t>;

    return ToDuration(static_cast<to_rep>(
        (static_cast<common_rep>(value.count()) *
         static_cast<common_rep>(Period::num) *
         static_cast<common_rep>(to_period::den)) /
        (static_cast<common_rep>(Period::den) *
         static_cast<common_rep>(to_period::num))));
}

// ============================================================================
// Common duration type
// ============================================================================
// The common period is the greatest period that represents both operands
// exactly. The common representation is Castle's normal common_type result.
// ============================================================================
namespace detail
{

template <typename Period1, typename Period2>
struct common_duration_period
{
    static_assert(is_valid_period<Period1>::value && is_valid_period<Period2>::value,
                  "castle::chrono::common_type requires valid periods");

    using type = castle::math::ratio<
        castle::math::gcd<Period1::num, Period2::num>::value,
        castle::math::lcm<Period1::den, Period2::den>::value>;
};

} // namespace detail

} // namespace chrono

namespace meta
{
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
struct common_type<castle::chrono::duration<Rep1, Period1>,
                   castle::chrono::duration<Rep2, Period2> >
{
    using rep = common_type_t<Rep1, Rep2>;
    using period = typename castle::chrono::detail::common_duration_period<Period1, Period2>::type;
    using type = castle::chrono::duration<rep, period>;
};
} // namespace meta

namespace chrono
{

// ============================================================================
// Unary duration operators
// ============================================================================
template <typename Rep, typename Period>
CASTLE_CONSTEXPR duration<Rep, Period>
operator+(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return value;
}

template <typename Rep, typename Period>
CASTLE_CONSTEXPR duration<Rep, Period>
operator-(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return duration<Rep, Period>(-value.count());
}

// ============================================================================
// Binary duration arithmetic
// ============================================================================
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR castle::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator+(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using result = castle::common_type_t<
        duration<Rep1, Period1>, duration<Rep2, Period2> >;

    return result(duration_cast<result>(lhs).count() +
                  duration_cast<result>(rhs).count());
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR castle::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator-(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using result = castle::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;

    return result(duration_cast<result>(lhs).count() -
                  duration_cast<result>(rhs).count());
}

template <typename Rep, typename Period, typename Scalar>
CASTLE_CONSTEXPR duration<castle::common_type_t<Rep, Scalar>, Period>
operator*(
    CASTLE_CONST duration<Rep, Period>& value,
    CASTLE_CONST Scalar& scalar) CASTLE_NOEXCEPT
{
    using result_rep = castle::common_type_t<Rep, Scalar>;
    using result = duration<result_rep, Period>;
    return result(static_cast<result_rep>(value.count()) *
                  static_cast<result_rep>(scalar));
}

template <typename Scalar, typename Rep, typename Period>
CASTLE_CONSTEXPR duration<castle::common_type_t<Rep, Scalar>, Period>
operator*(
    CASTLE_CONST Scalar& scalar,
    CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return value * scalar;
}

template <typename Rep, typename Period, typename Scalar>
CASTLE_CONSTEXPR duration<castle::common_type_t<Rep, Scalar>, Period>
operator/(
    CASTLE_CONST duration<Rep, Period>& value,
    CASTLE_CONST Scalar& scalar) CASTLE_NOEXCEPT
{
    using result_rep = castle::common_type_t<Rep, Scalar>;
    using result = duration<result_rep, Period>;
    return result(static_cast<result_rep>(value.count()) /
                  static_cast<result_rep>(scalar));
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR castle::common_type_t<Rep1, Rep2>
operator/(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using common_duration = castle::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
    using result_rep = castle::common_type_t<Rep1, Rep2>;

    return static_cast<result_rep>(duration_cast<common_duration>(lhs).count()) /
           static_cast<result_rep>(duration_cast<common_duration>(rhs).count());
}

template <typename Rep, typename Period, typename Scalar>
CASTLE_CONSTEXPR duration<Rep, Period>
operator%(
    CASTLE_CONST duration<Rep, Period>& value,
    CASTLE_CONST Scalar& scalar) CASTLE_NOEXCEPT
{
    return duration<Rep, Period>(value.count() % scalar);
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR castle::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator%(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using result = castle::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;

    return result(duration_cast<result>(lhs).count() %
                  duration_cast<result>(rhs).count());
}

// ============================================================================
// Duration comparisons
// ============================================================================
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator==(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using common_duration = castle::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;

    return duration_cast<common_duration>(lhs).count() ==
           duration_cast<common_duration>(rhs).count();
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator!=(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator<(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using common_duration = castle::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;

    return duration_cast<common_duration>(lhs).count() <
           duration_cast<common_duration>(rhs).count();
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator<=(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    return !(rhs < lhs);
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator>(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    return rhs < lhs;
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator>=(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs < rhs);
}

// ============================================================================
// Rounding helpers (C++17)
// ============================================================================
template <typename ToDuration, typename Rep, typename Period>
CASTLE_CONSTEXPR ToDuration
floor(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return (duration_cast<ToDuration>(value) > value)
           ? (duration_cast<ToDuration>(value) - ToDuration(1))
           : duration_cast<ToDuration>(value);
}

template <typename ToDuration, typename Rep, typename Period>
CASTLE_CONSTEXPR ToDuration
ceil(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return (duration_cast<ToDuration>(value) < value)
           ? (duration_cast<ToDuration>(value) + ToDuration(1))
           : duration_cast<ToDuration>(value);
}

template <typename ToDuration, typename Rep, typename Period>
CASTLE_CONSTEXPR ToDuration
round(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    CASTLE_CONST ToDuration lower = floor<ToDuration>(value);
    CASTLE_CONST ToDuration upper = lower + ToDuration(1);
    CASTLE_CONST auto lower_distance = value - lower;
    CASTLE_CONST auto upper_distance = upper - value;

    if (lower_distance < upper_distance)
    {
        return lower;
    }

    if (upper_distance < lower_distance)
    {
        return upper;
    }

    return (lower.count() % 2 == 0) ? lower : upper;
}

template <typename Rep, typename Period>
CASTLE_CONSTEXPR duration<Rep, Period>
abs(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return (value < duration<Rep, Period>::zero()) ? -value : value;
}

} // namespace chrono
} // namespace castle

#endif // CASTLE_CHRONO_DURATION_H
