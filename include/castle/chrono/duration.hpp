// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file duration.hpp
 * @brief Fixed-ratio time quantities for Castle's deterministic chrono layer.
 *
 * Include this header when code needs compile-time period-aware time values
 * without depending on the C++ standard library. Durations are small value types
 * that store a representation plus a compile-time ratio and support arithmetic,
 * comparisons, casting, and rounding.
 *
 * Key constraints:
 * - Tick resolution is defined by the @c Period ratio, such as nanoseconds or milliseconds.
 * - Durations carry no wall-clock or monotonic semantics by themselves; clocks add those semantics.
 * - Cross-period conversion uses integer or floating arithmetic in a common representation.
 * - Arithmetic and conversions do not check overflow; callers must keep results representable.
 *
 * Example:
 * @code
 * #include "castle/chrono/duration.hpp"
 *
 * int main()
 * {
 *     castle::chrono::milliseconds period(10);
 *     castle::chrono::microseconds fine =
 *         castle::chrono::duration_cast<castle::chrono::microseconds>(period);
 *     auto total = period + castle::chrono::milliseconds(5);
 *     (void)fine;
 *     (void)total;
 *     return 0;
 * }
 * @endcode
 */

#ifndef CASTLE_CHRONO_DURATION_HPP
#define CASTLE_CHRONO_DURATION_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/math/gcd.hpp"
#include "castle/math/lcm.hpp"
#include "castle/math/ratio.hpp"

#include <float.h>
#include <limits.h>
#include <stdint.h>

namespace castle
{
namespace chrono
{

/**
 * @brief Reports whether a ratio can be used as a duration period.
 * @tparam Ratio Ratio type with @c num and @c den members.
 * @note A valid period must have strictly positive numerator and denominator.
 */
template <typename Ratio>
struct is_valid_period
    : meta::bool_constant<(Ratio::num > 0) && (Ratio::den > 0)>
{
};

/**
 * @brief Supplies zero, minimum, and maximum values for a duration representation.
 * @tparam Rep Underlying representation type.
 * @note Specialize this template when a custom representation needs custom limits.
 */
template <typename Rep>
struct duration_values
{
    /**
     * @brief Returns the zero value for the representation type.
     * @return A zero-initialized @p Rep.
     */
    static CASTLE_CONSTEXPR Rep zero() CASTLE_NOEXCEPT
    {
        return Rep(0);
    }

    /**
     * @brief Returns the lowest representable value for the representation type.
     * @return The lowest value reported by @c castle::numeric_limits<Rep>.
     */
    static CASTLE_CONSTEXPR Rep min() CASTLE_NOEXCEPT
    {
        return castle::numeric_limits<Rep>::lowest();
    }

    /**
     * @brief Returns the highest representable value for the representation type.
     * @return The maximum value reported by @c castle::numeric_limits<Rep>.
     */
    static CASTLE_CONSTEXPR Rep max() CASTLE_NOEXCEPT
    {
        return castle::numeric_limits<Rep>::max();
    }
};

/**
 * @brief Represents a quantity of time as a tick count plus a compile-time period ratio.
 * @tparam Rep Underlying representation type used to store tick counts.
 * @tparam Period Positive ratio describing one tick in seconds.
 * @warning Arithmetic on the stored representation is unchecked and may overflow.
 */
template <typename Rep, typename Period = castle::math::ratio<1, 1>>
class duration;

/**
 * @brief Converts a duration to a different period and representation type.
 * @tparam ToDuration Destination duration type.
 * @tparam Rep Source representation type.
 * @tparam Period Source period ratio.
 * @param value Duration to convert.
 * @return @p value expressed as @p ToDuration.
 * @note Integer conversions truncate toward zero.
 * @warning The intermediate arithmetic is unchecked and may overflow for extreme values.
 */
template <typename ToDuration, typename Rep, typename Period>
CASTLE_CONSTEXPR ToDuration duration_cast(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT;

/**
 * @brief Represents a quantity of time using a representation and a compile-time tick period.
 * @tparam Rep Underlying representation type used to store tick counts.
 * @tparam Period Positive ratio describing one tick in seconds.
 */
template <typename Rep, typename Period>
class duration
{
public:
    using rep = Rep;
    using period = Period;

    static_assert(is_valid_period<Period>::value,
                  "castle::chrono::duration Period must have positive num/den");

    /**
     * @brief Constructs a zero-valued duration.
     * @return A duration whose stored tick count is zero.
     */
    CASTLE_CONSTEXPR duration() CASTLE_NOEXCEPT
        : value_(duration_values<rep>::zero())
    {
    }

    /**
     * @brief Constructs a duration from a raw tick count.
     * @param value Tick count expressed in this duration's @c period.
     * @note No scaling is performed because the input is already in the target unit.
     */
    CASTLE_CONSTEXPR explicit duration(rep value) CASTLE_NOEXCEPT
        : value_(value)
    {
    }

    /**
     * @brief Converts a duration with the same period to this representation type.
     * @tparam Rep2 Source representation type.
     * @param other Duration to convert.
     * @note This overload is disabled when converting a floating-point representation to an integral representation.
     * @warning Narrowing to a smaller representation may lose data or overflow.
     */
    template <typename Rep2,
              typename meta::enable_if<
                  !(meta::is_integral<rep>::value &&
                    meta::is_floating_point<Rep2>::value), int>::type = 0>
    CASTLE_CONSTEXPR explicit duration(
        CASTLE_CONST duration<Rep2, Period>& other) CASTLE_NOEXCEPT
        : value_(static_cast<rep>(other.count()))
    {
    }

    /**
     * @brief Converts a duration with a different period to this duration type.
     * @tparam Rep2 Source representation type.
     * @tparam Period2 Source period ratio.
     * @param other Duration to convert.
     * @note Conversion is performed with @c duration_cast and truncates toward zero when the target representation is integral.
     * @warning Narrowing to a smaller representation may lose data or overflow.
     */
    template <typename Rep2, typename Period2,
              typename meta::enable_if<
                  !(meta::is_integral<rep>::value &&
                  meta::is_floating_point<Rep2>::value), int>::type = 0>
    CASTLE_CONSTEXPR explicit duration(
        CASTLE_CONST duration<Rep2, Period2>& other) CASTLE_NOEXCEPT
        : value_(duration_cast<duration>(other).count())
    {
    }

    /**
     * @brief Copies a duration.
     * @param other Duration to copy.
     */
    duration(CASTLE_CONST duration&) CASTLE_DEFAULT;

    /**
     * @brief Assigns from another duration of the same type.
     * @param other Duration to copy.
     * @return Reference to this duration.
     */
    duration& operator=(CASTLE_CONST duration&) CASTLE_DEFAULT;

    /**
     * @brief Returns the stored tick count.
     * @return Tick count expressed in this duration's @c period.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR rep count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return value_;
    }

    /**
     * @brief Returns a zero-valued duration.
     * @return A duration with a tick count of zero.
     */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR duration zero() CASTLE_NOEXCEPT
    {
        return duration(duration_values<rep>::zero());
    }

    /**
     * @brief Returns the lowest representable duration.
     * @return A duration whose stored tick count is the lowest representable @c rep value.
     */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR duration min() CASTLE_NOEXCEPT
    {
        return duration(duration_values<rep>::min());
    }

    /**
     * @brief Returns the highest representable duration.
     * @return A duration whose stored tick count is the highest representable @c rep value.
     */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR duration max() CASTLE_NOEXCEPT
    {
        return duration(duration_values<rep>::max());
    }

    /**
     * @brief Increments the stored tick count by one tick.
     * @return Reference to this duration after incrementing.
     * @note One tick means one unit of this duration's @c period.
     */
    CASTLE_CONSTEXPR duration& operator++() CASTLE_NOEXCEPT
    {
        ++value_;
        return *this;
    }

    /**
     * @brief Returns the value before incrementing by one tick.
     * @return Copy of the duration before incrementing.
     * @note One tick means one unit of this duration's @c period.
     */
    CASTLE_CONSTEXPR duration operator++(int) CASTLE_NOEXCEPT
    {
        duration temp(*this);
        ++value_;
        return temp;
    }

    /**
     * @brief Decrements the stored tick count by one tick.
     * @return Reference to this duration after decrementing.
     * @note One tick means one unit of this duration's @c period.
     */
    CASTLE_CONSTEXPR duration& operator--() CASTLE_NOEXCEPT
    {
        --value_;
        return *this;
    }

    /**
     * @brief Returns the value before decrementing by one tick.
     * @return Copy of the duration before decrementing.
     * @note One tick means one unit of this duration's @c period.
     */
    CASTLE_CONSTEXPR duration operator--(int) CASTLE_NOEXCEPT
    {
        duration temp(*this);
        --value_;
        return temp;
    }

    /**
     * @brief Adds another duration of the same type.
     * @param other Duration to add.
     * @return Reference to this duration after addition.
     * @warning Addition is unchecked and may overflow the representation type.
     */
    CASTLE_CONSTEXPR duration& operator+=(CASTLE_CONST duration& other) CASTLE_NOEXCEPT
    {
        value_ += other.value_;
        return *this;
    }

    /**
     * @brief Subtracts another duration of the same type.
     * @param other Duration to subtract.
     * @return Reference to this duration after subtraction.
     * @warning Subtraction is unchecked and may overflow the representation type.
     */
    CASTLE_CONSTEXPR duration& operator-=(CASTLE_CONST duration& other) CASTLE_NOEXCEPT
    {
        value_ -= other.value_;
        return *this;
    }

    /**
     * @brief Multiplies the stored tick count by a scalar.
     * @param scalar Scalar multiplier in the same representation type as @c rep.
     * @return Reference to this duration after multiplication.
     * @warning Multiplication is unchecked and may overflow the representation type.
     */
    CASTLE_CONSTEXPR duration& operator*=(CASTLE_CONST rep& scalar) CASTLE_NOEXCEPT
    {
        value_ *= scalar;
        return *this;
    }

    /**
     * @brief Divides the stored tick count by a scalar.
     * @param scalar Scalar divisor in the same representation type as @c rep.
     * @return Reference to this duration after division.
     * @note Integral representations use integral division semantics.
     * @warning Division by zero is undefined and is not checked.
     */
    CASTLE_CONSTEXPR duration& operator/=(CASTLE_CONST rep& scalar) CASTLE_NOEXCEPT
    {
        value_ /= scalar;
        return *this;
    }

    /**
     * @brief Replaces the stored tick count with its remainder modulo a scalar.
     * @param scalar Scalar modulus in the same representation type as @c rep.
     * @return Reference to this duration after the remainder operation.
     * @note This operator requires that @c rep supports the @c % operator.
     * @warning Modulo by zero is undefined and is not checked.
     */
    CASTLE_CONSTEXPR duration& operator%=(CASTLE_CONST rep& scalar) CASTLE_NOEXCEPT
    {
        value_ %= scalar;
        return *this;
    }

    /**
     * @brief Replaces the stored tick count with its remainder modulo another duration.
     * @param other Duration providing the modulus tick count.
     * @return Reference to this duration after the remainder operation.
     * @note This operator requires that @c rep supports the @c % operator.
     * @warning Modulo by a zero tick count is undefined and is not checked.
     */
    CASTLE_CONSTEXPR duration& operator%=(CASTLE_CONST duration& other) CASTLE_NOEXCEPT
    {
        value_ %= other.value_;
        return *this;
    }

private:
    rep value_;
};

/**
 * @brief Nanosecond duration alias using an @c int64_t tick count.
 */
using nanoseconds  = duration<int64_t, castle::nano>;

/**
 * @brief Microsecond duration alias using an @c int64_t tick count.
 */
using microseconds = duration<int64_t, castle::micro>;

/**
 * @brief Millisecond duration alias using an @c int64_t tick count.
 */
using milliseconds = duration<int64_t, castle::milli>;

/**
 * @brief Second duration alias using an @c int64_t tick count.
 */
using seconds      = duration<int64_t, castle::seconds>;

/**
 * @brief Minute duration alias using an @c int64_t tick count.
 */
using minutes      = duration<int64_t, castle::minutes>;

/**
 * @brief Hour duration alias using an @c int64_t tick count.
 */
using hours        = duration<int64_t, castle::hours>;

/**
 * @brief Day duration alias using an @c int64_t tick count.
 */
using days         = duration<int64_t, castle::days>;

/**
 * @brief Week duration alias using an @c int64_t tick count.
 */
using weeks        = duration<int64_t, castle::weeks>;

/**
 * @brief Casts a duration to another duration type, potentially changing the tick count and period.
 *
 * @tparam ToDuration Target duration type to cast to.
 * @tparam Rep Representation type of the source duration.
 * @tparam Period Period ratio of the source duration.
 * @param value Source duration to cast.
 * @return The casted duration of type @c ToDuration.
 * @note The cast may involve truncation if the target period is coarser than the source period.
 */
template <typename ToDuration, typename Rep, typename Period>
CASTLE_CONSTEXPR ToDuration
duration_cast(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    using to_rep = typename ToDuration::rep;
    using to_period = typename ToDuration::period;
    using common_rep = meta::common_type_t<Rep, to_rep, intmax_t>;

    return ToDuration(static_cast<to_rep>(
        // Scale the source tick count into the destination period before truncating to the target representation.
        (static_cast<common_rep>(value.count()) *
         static_cast<common_rep>(Period::num) *
         static_cast<common_rep>(to_period::den)) /
        (static_cast<common_rep>(Period::den) *
         static_cast<common_rep>(to_period::num))));
}

namespace detail
{

/**
 * @brief Computes the common period type for two duration periods.
 *
 * @tparam Period1 First duration period type.
 * @tparam Period2 Second duration period type.
 * @return The common period type as a ratio.
 * @note The common period is computed using the greatest common divisor of the numerators and the least common multiple of the denominators.
 */
template <typename Period1, typename Period2>
struct common_duration_period
{
    static_assert(is_valid_period<Period1>::value && is_valid_period<Period2>::value,
                  "castle::chrono::common_type requires valid periods");

    using type = castle::math::ratio<
        castle::math::gcd<Period1::num, Period2::num>::value,
        castle::math::lcm<Period1::den, Period2::den>::value>;
};

}

}

namespace meta
{

/**
 * @brief Computes the common type for two duration types, including both the representation and period.
 *
 * @tparam Rep1 Representation type of the first duration.
 * @tparam Period1 Period ratio of the first duration.
 * @tparam Rep2 Representation type of the second duration.
 * @tparam Period2 Period ratio of the second duration.
 * @return The common duration type for the two specified duration types.
 * @note The common type is determined by computing the common representation type and the common period type for the two durations.
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
struct common_type<castle::chrono::duration<Rep1, Period1>,
                   castle::chrono::duration<Rep2, Period2> >
{
    using rep = common_type_t<Rep1, Rep2>;
    using period = typename castle::chrono::detail::common_duration_period<Period1, Period2>::type;
    using type = castle::chrono::duration<rep, period>;
};

}

namespace chrono
{

/**
 * @brief Returns a duration unchanged.
 * @tparam Rep Duration representation type.
 * @tparam Period Duration period ratio.
 * @param value Duration to return.
 * @return Copy of @p value.
 */
template <typename Rep, typename Period>
CASTLE_CONSTEXPR duration<Rep, Period>
operator+(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return value;
}

/**
 * @brief Negates a duration's tick count.
 * @tparam Rep Duration representation type.
 * @tparam Period Duration period ratio.
 * @param value Duration to negate.
 * @return Duration whose tick count is the arithmetic negation of @p value.
 * @warning Negating the minimum signed representation value may overflow.
 */
template <typename Rep, typename Period>
CASTLE_CONSTEXPR duration<Rep, Period>
operator-(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return duration<Rep, Period>(-value.count());
}

/**
 * @brief Adds two durations after converting both to a common period.
 * @tparam Rep1 Left-hand representation type.
 * @tparam Period1 Left-hand period ratio.
 * @tparam Rep2 Right-hand representation type.
 * @tparam Period2 Right-hand period ratio.
 * @param lhs Left-hand duration.
 * @param rhs Right-hand duration.
 * @return Sum expressed in the common duration type of @p lhs and @p rhs.
 * @warning Addition is unchecked and may overflow the common representation type.
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR meta::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator+(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using result = meta::common_type_t<
        duration<Rep1, Period1>, duration<Rep2, Period2> >;

    return result(duration_cast<result>(lhs).count() +
                  duration_cast<result>(rhs).count());
}

/**
 * @brief Subtracts one duration from another after converting both to a common period.
 * @tparam Rep1 Left-hand representation type.
 * @tparam Period1 Left-hand period ratio.
 * @tparam Rep2 Right-hand representation type.
 * @tparam Period2 Right-hand period ratio.
 * @param lhs Left-hand duration.
 * @param rhs Right-hand duration.
 * @return Difference expressed in the common duration type of @p lhs and @p rhs.
 * @warning Subtraction is unchecked and may overflow the common representation type.
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR meta::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator-(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using result = meta::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;

    return result(duration_cast<result>(lhs).count() -
                  duration_cast<result>(rhs).count());
}

/**
 * @brief Multiplies a duration by a scalar.
 * @tparam Rep Duration representation type.
 * @tparam Period Duration period ratio.
 * @tparam Scalar Scalar type.
 * @param value Duration to scale.
 * @param scalar Multiplier.
 * @return Duration in the same period with a common representation type.
 * @warning Multiplication is unchecked and may overflow the result representation type.
 */
template <typename Rep, typename Period, typename Scalar>
CASTLE_CONSTEXPR duration<meta::common_type_t<Rep, Scalar>, Period>
operator*(
    CASTLE_CONST duration<Rep, Period>& value,
    CASTLE_CONST Scalar& scalar) CASTLE_NOEXCEPT
{
    using result_rep = meta::common_type_t<Rep, Scalar>;
    using result = duration<result_rep, Period>;
    return result(static_cast<result_rep>(value.count()) *
                  static_cast<result_rep>(scalar));
}

/**
 * @brief Multiplies a scalar by a duration.
 * @tparam Scalar Scalar type.
 * @tparam Rep Duration representation type.
 * @tparam Period Duration period ratio.
 * @param scalar Multiplier.
 * @param value Duration to scale.
 * @return Duration in the same period with a common representation type.
 * @warning Multiplication is unchecked and may overflow the result representation type.
 */
template <typename Scalar, typename Rep, typename Period>
CASTLE_CONSTEXPR duration<meta::common_type_t<Rep, Scalar>, Period>
operator*(
    CASTLE_CONST Scalar& scalar,
    CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return value * scalar;
}

/**
 * @brief Divides a duration by a scalar.
 * @tparam Rep Duration representation type.
 * @tparam Period Duration period ratio.
 * @tparam Scalar Scalar type.
 * @param value Duration to divide.
 * @param scalar Divisor.
 * @return Duration in the same period with a common representation type.
 * @note Integral representations use integral division semantics.
 * @warning Division by zero is undefined and is not checked.
 */
template <typename Rep, typename Period, typename Scalar>
CASTLE_CONSTEXPR duration<meta::common_type_t<Rep, Scalar>, Period>
operator/(
    CASTLE_CONST duration<Rep, Period>& value,
    CASTLE_CONST Scalar& scalar) CASTLE_NOEXCEPT
{
    using result_rep = meta::common_type_t<Rep, Scalar>;
    using result = duration<result_rep, Period>;
    return result(static_cast<result_rep>(value.count()) /
                  static_cast<result_rep>(scalar));
}

/**
 * @brief Divides one duration by another after converting both to a common period.
 * @tparam Rep1 Left-hand representation type.
 * @tparam Period1 Left-hand period ratio.
 * @tparam Rep2 Right-hand representation type.
 * @tparam Period2 Right-hand period ratio.
 * @param lhs Numerator duration.
 * @param rhs Denominator duration.
 * @return Scalar quotient in the common representation type of @p lhs and @p rhs.
 * @note Integral representations use integral division semantics.
 * @warning Division by a zero-duration tick count is undefined and is not checked.
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR meta::common_type_t<Rep1, Rep2>
operator/(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using common_duration = meta::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
    using result_rep = meta::common_type_t<Rep1, Rep2>;

    return static_cast<result_rep>(duration_cast<common_duration>(lhs).count()) /
           static_cast<result_rep>(duration_cast<common_duration>(rhs).count());
}

/**
 * @brief Computes the remainder of a duration divided by a scalar.
 * @tparam Rep Duration representation type.
 * @tparam Period Duration period ratio.
 * @tparam Scalar Scalar type.
 * @param value Duration to reduce.
 * @param scalar Scalar modulus.
 * @return Duration with the same period containing the remainder tick count.
 * @note This operator requires that the underlying representation supports @c %.
 * @warning Modulo by zero is undefined and is not checked.
 */
template <typename Rep, typename Period, typename Scalar>
CASTLE_CONSTEXPR duration<Rep, Period>
operator%(
    CASTLE_CONST duration<Rep, Period>& value,
    CASTLE_CONST Scalar& scalar) CASTLE_NOEXCEPT
{
    return duration<Rep, Period>(value.count() % scalar);
}

/**
 * @brief Computes the remainder of one duration divided by another after conversion to a common period.
 * @tparam Rep1 Left-hand representation type.
 * @tparam Period1 Left-hand period ratio.
 * @tparam Rep2 Right-hand representation type.
 * @tparam Period2 Right-hand period ratio.
 * @param lhs Numerator duration.
 * @param rhs Denominator duration.
 * @return Remainder expressed in the common duration type of @p lhs and @p rhs.
 * @note This operator requires that the common representation supports @c %.
 * @warning Modulo by a zero-duration tick count is undefined and is not checked.
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR meta::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator%(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using result = meta::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;

    return result(duration_cast<result>(lhs).count() %
                  duration_cast<result>(rhs).count());
}

/**
 * @brief Compares two durations for equality after conversion to a common period.
 * @tparam Rep1 Left-hand representation type.
 * @tparam Period1 Left-hand period ratio.
 * @tparam Rep2 Right-hand representation type.
 * @tparam Period2 Right-hand period ratio.
 * @param lhs Left-hand duration.
 * @param rhs Right-hand duration.
 * @return @c true when both durations represent the same elapsed time, otherwise @c false.
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator==(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using common_duration = meta::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;

    return duration_cast<common_duration>(lhs).count() ==
           duration_cast<common_duration>(rhs).count();
}

/**
 * @brief Compares two durations for inequality after conversion to a common period.
 * @tparam Rep1 Left-hand representation type.
 * @tparam Period1 Left-hand period ratio.
 * @tparam Rep2 Right-hand representation type.
 * @tparam Period2 Right-hand period ratio.
 * @param lhs Left-hand duration.
 * @param rhs Right-hand duration.
 * @return @c true when the durations represent different elapsed times, otherwise @c false.
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator!=(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/**
 * @brief Orders two durations after conversion to a common period.
 * @tparam Rep1 Left-hand representation type.
 * @tparam Period1 Left-hand period ratio.
 * @tparam Rep2 Right-hand representation type.
 * @tparam Period2 Right-hand period ratio.
 * @param lhs Left-hand duration.
 * @param rhs Right-hand duration.
 * @return @c true when @p lhs is shorter than @p rhs, otherwise @c false.
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator<(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    using common_duration = meta::common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;

    return duration_cast<common_duration>(lhs).count() <
           duration_cast<common_duration>(rhs).count();
}

/**
 * @brief Orders two durations after conversion to a common period.
 * @tparam Rep1 Left-hand representation type.
 * @tparam Period1 Left-hand period ratio.
 * @tparam Rep2 Right-hand representation type.
 * @tparam Period2 Right-hand period ratio.
 * @param lhs Left-hand duration.
 * @param rhs Right-hand duration.
 * @return @c true when @p lhs is shorter than or equal to @p rhs, otherwise @c false.
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator<=(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    return !(rhs < lhs);
}

/**
 * @brief Orders two durations after conversion to a common period.
 * @tparam Rep1 Left-hand representation type.
 * @tparam Period1 Left-hand period ratio.
 * @tparam Rep2 Right-hand representation type.
 * @tparam Period2 Right-hand period ratio.
 * @param lhs Left-hand duration.
 * @param rhs Right-hand duration.
 * @return @c true when @p lhs is longer than @p rhs, otherwise @c false.
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator>(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    return rhs < lhs;
}

/**
 * @brief Orders two durations after conversion to a common period.
 * @tparam Rep1 Left-hand representation type.
 * @tparam Period1 Left-hand period ratio.
 * @tparam Rep2 Right-hand representation type.
 * @tparam Period2 Right-hand period ratio.
 * @param lhs Left-hand duration.
 * @param rhs Right-hand duration.
 * @return @c true when @p lhs is longer than or equal to @p rhs, otherwise @c false.
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
CASTLE_CONSTEXPR bool
operator>=(
    CASTLE_CONST duration<Rep1, Period1>& lhs,
    CASTLE_CONST duration<Rep2, Period2>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs < rhs);
}

/**
 * @brief Rounds a duration toward negative infinity in a destination unit.
 * @tparam ToDuration Destination duration type.
 * @tparam Rep Source representation type.
 * @tparam Period Source period ratio.
 * @param value Duration to round.
 * @return Largest destination duration not greater than @p value.
 * @note Integer conversion semantics come from @c duration_cast.
 */
template <typename ToDuration, typename Rep, typename Period>
CASTLE_CONSTEXPR ToDuration
floor(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return (duration_cast<ToDuration>(value) > value)
           ? (duration_cast<ToDuration>(value) - ToDuration(1))
           : duration_cast<ToDuration>(value);
}

/**
 * @brief Rounds a duration toward positive infinity in a destination unit.
 * @tparam ToDuration Destination duration type.
 * @tparam Rep Source representation type.
 * @tparam Period Source period ratio.
 * @param value Duration to round.
 * @return Smallest destination duration not less than @p value.
 * @note Integer conversion semantics come from @c duration_cast.
 */
template <typename ToDuration, typename Rep, typename Period>
CASTLE_CONSTEXPR ToDuration
ceil(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return (duration_cast<ToDuration>(value) < value)
           ? (duration_cast<ToDuration>(value) + ToDuration(1))
           : duration_cast<ToDuration>(value);
}

/**
 * @brief Rounds a duration to the nearest destination unit using ties-to-even.
 * @tparam ToDuration Destination duration type.
 * @tparam Rep Source representation type.
 * @tparam Period Source period ratio.
 * @param value Duration to round.
 * @return Nearest destination duration to @p value.
 * @note Midpoint ties round to the even destination tick count.
 */
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

/**
 * @brief Returns the absolute value of a duration.
 * @tparam Rep Duration representation type.
 * @tparam Period Duration period ratio.
 * @param value Duration to examine.
 * @return Non-negative duration with the same magnitude as @p value.
 * @warning Negating the minimum signed representation value may overflow.
 */
template <typename Rep, typename Period>
CASTLE_CONSTEXPR duration<Rep, Period>
abs(CASTLE_CONST duration<Rep, Period>& value) CASTLE_NOEXCEPT
{
    return (value < duration<Rep, Period>::zero()) ? -value : value;
}

}
}

#endif
