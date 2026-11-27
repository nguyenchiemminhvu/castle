// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file time_point.hpp
 * @brief Clock-relative instants for Castle's deterministic chrono layer.
 *
 * Include this header when code needs an instant on a specific clock timeline,
 * such as scheduler deadlines, timeout expirations, or timestamps returned by
 * Castle clocks. A time point stores only a duration since its clock epoch.
 *
 * Key constraints:
 * - Tick resolution is defined by the @c Duration template argument.
 * - Wall-clock or monotonic semantics come from the associated @c Clock type, not from @c time_point itself.
 * - Arithmetic and conversions use duration operations and therefore inherit their truncation and overflow behavior.
 *
 * Example:
 * @code
 * #include "castle/chrono/time_point.hpp"
 *
 * struct test_clock
 * {
 *     using duration = castle::chrono::milliseconds;
 * };
 *
 * int main()
 * {
 *     using point = castle::chrono::time_point<test_clock, castle::chrono::milliseconds>;
 *     point start(castle::chrono::milliseconds(100));
 *     point deadline = start + castle::chrono::milliseconds(25);
 *     (void)deadline;
 *     return 0;
 * }
 * @endcode
 */

#ifndef CASTLE_CHRONO_TIME_POINT_HPP
#define CASTLE_CHRONO_TIME_POINT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/chrono/duration.hpp"

namespace castle
{
namespace chrono
{

/**
 * @brief Represents an instant on a specific clock timeline.
 * @tparam Clock Clock type defining the epoch and semantics.
 * @tparam Duration Duration type used to store ticks since the clock epoch.
 * @warning Arithmetic is unchecked and may overflow the underlying duration representation.
 */
template <typename Clock, typename Duration = typename Clock::duration>
class time_point
{
public:
    using clock = Clock;
    using duration = Duration;
    using rep = typename duration::rep;
    using period = typename duration::period;

    /**
     * @brief Constructs a time point at the clock epoch.
     * @return A time point whose duration since epoch is zero.
     */
    CASTLE_CONSTEXPR time_point() CASTLE_NOEXCEPT
        : value_()
    {
    }

    /**
     * @brief Constructs a time point from a duration since the clock epoch.
     * @param value Duration since the associated clock epoch.
     */
    explicit CASTLE_CONSTEXPR time_point(CASTLE_CONST duration& value) CASTLE_NOEXCEPT
        : value_(value)
    {
    }

    /**
     * @brief Converts a time point of the same clock to a different duration precision.
     * @tparam Duration2 Source duration type.
     * @param other Time point to convert.
     * @note Conversion uses @c duration_cast and therefore truncates toward zero when the destination duration uses an integral representation.
     * @warning Narrowing to a smaller representation may lose data or overflow.
     */
    template <typename Duration2>
    explicit CASTLE_CONSTEXPR time_point(
        CASTLE_CONST time_point<Clock, Duration2>& other) CASTLE_NOEXCEPT
        : value_(duration_cast<duration>(other.time_since_epoch()))
    {
    }

    /**
     * @brief Copies a time point.
     * @param other Time point to copy.
     */
    time_point(CASTLE_CONST time_point&) CASTLE_DEFAULT;

    /**
     * @brief Assigns from another time point of the same type.
     * @param other Time point to copy.
     * @return Reference to this time point.
     */
    time_point& operator=(CASTLE_CONST time_point&) CASTLE_DEFAULT;

    /**
     * @brief Returns the duration since the clock epoch.
     * @return Stored duration measured in this time point's @c period.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR duration time_since_epoch() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return value_;
    }

    /**
     * @brief Returns the lowest representable time point.
     * @return Time point whose stored duration equals @c duration::min().
     */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR time_point min() CASTLE_NOEXCEPT
    {
        return time_point(duration::min());
    }

    /**
     * @brief Returns the highest representable time point.
     * @return Time point whose stored duration equals @c duration::max().
     */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR time_point max() CASTLE_NOEXCEPT
    {
        return time_point(duration::max());
    }

    /**
     * @brief Advances the time point by a duration of the same type.
     * @param delta Duration to add.
     * @return Reference to this time point after addition.
     * @warning Addition is unchecked and may overflow the underlying duration representation.
     */
    CASTLE_CONSTEXPR time_point& operator+=(CASTLE_CONST duration& delta) CASTLE_NOEXCEPT
    {
        value_ += delta;
        return *this;
    }

    /**
     * @brief Moves the time point backward by a duration of the same type.
     * @param delta Duration to subtract.
     * @return Reference to this time point after subtraction.
     * @warning Subtraction is unchecked and may overflow the underlying duration representation.
     */
    CASTLE_CONSTEXPR time_point& operator-=(CASTLE_CONST duration& delta) CASTLE_NOEXCEPT
    {
        value_ -= delta;
        return *this;
    }

    /**
     * @brief Increments the stored duration by one tick.
     * @return Reference to this time point after incrementing.
     * @note One tick means one unit of this time point's @c period.
     */
    CASTLE_CONSTEXPR time_point& operator++() CASTLE_NOEXCEPT
    {
        ++value_;
        return *this;
    }

    /**
     * @brief Returns the value before incrementing by one tick.
     * @return Copy of the time point before incrementing.
     * @note One tick means one unit of this time point's @c period.
     */
    CASTLE_CONSTEXPR time_point operator++(int) CASTLE_NOEXCEPT
    {
        time_point temp(*this);
        ++value_;
        return temp;
    }

    /**
     * @brief Decrements the stored duration by one tick.
     * @return Reference to this time point after decrementing.
     * @note One tick means one unit of this time point's @c period.
     */
    CASTLE_CONSTEXPR time_point& operator--() CASTLE_NOEXCEPT
    {
        --value_;
        return *this;
    }

    /**
     * @brief Returns the value before decrementing by one tick.
     * @return Copy of the time point before decrementing.
     * @note One tick means one unit of this time point's @c period.
     */
    CASTLE_CONSTEXPR time_point operator--(int) CASTLE_NOEXCEPT
    {
        time_point temp(*this);
        --value_;
        return temp;
    }

private:
    duration value_;
};

/**
 * @brief Converts a time point to a different duration precision while preserving the clock type.
 * @tparam ToDuration Destination duration type.
 * @tparam Clock Clock type.
 * @tparam Duration Source duration type.
 * @param value Time point to convert.
 * @return Time point whose @c time_since_epoch() is expressed as @p ToDuration.
 * @note Conversion uses @c duration_cast and therefore truncates toward zero when the destination duration uses an integral representation.
 */
template <typename ToDuration, typename Clock, typename Duration>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<Clock, ToDuration>
time_point_cast(CASTLE_CONST time_point<Clock, Duration>& value) CASTLE_NOEXCEPT
{
    return time_point<Clock, ToDuration>(
        duration_cast<ToDuration>(value.time_since_epoch())
    );
}

/**
 * @brief Rounds a time point toward negative infinity in a destination duration unit.
 * @tparam ToDuration Destination duration type.
 * @tparam Clock Clock type.
 * @tparam Duration Source duration type.
 * @param value Time point to round.
 * @return Largest destination-precision time point not greater than @p value.
 */
template <typename ToDuration, typename Clock, typename Duration>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<Clock, ToDuration>
floor(CASTLE_CONST time_point<Clock, Duration>& value) CASTLE_NOEXCEPT
{
    return time_point<Clock, ToDuration>(
        chrono::floor<ToDuration>(value.time_since_epoch())
    );
}

/**
 * @brief Rounds a time point toward positive infinity in a destination duration unit.
 * @tparam ToDuration Destination duration type.
 * @tparam Clock Clock type.
 * @tparam Duration Source duration type.
 * @param value Time point to round.
 * @return Smallest destination-precision time point not less than @p value.
 */
template <typename ToDuration, typename Clock, typename Duration>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<Clock, ToDuration>
ceil(CASTLE_CONST time_point<Clock, Duration>& value) CASTLE_NOEXCEPT
{
    return time_point<Clock, ToDuration>(
        chrono::ceil<ToDuration>(value.time_since_epoch())
    );
}

/**
 * @brief Rounds a time point to the nearest destination duration unit using ties-to-even.
 * @tparam ToDuration Destination duration type.
 * @tparam Clock Clock type.
 * @tparam Duration Source duration type.
 * @param value Time point to round.
 * @return Nearest destination-precision time point to @p value.
 * @note Midpoint ties round to the even destination tick count.
 */
template <typename ToDuration, typename Clock, typename Duration>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<Clock, ToDuration>
round(CASTLE_CONST time_point<Clock, Duration>& value) CASTLE_NOEXCEPT
{
    return time_point<Clock, ToDuration>(
        chrono::round<ToDuration>(value.time_since_epoch())
    );
}

/**
 * @brief Adds a duration to a time point.
 * @tparam Clock Clock type.
 * @tparam Duration1 Time-point duration type.
 * @tparam Rep2 Delta representation type.
 * @tparam Period2 Delta period ratio.
 * @param value Base time point.
 * @param delta Duration to add.
 * @return Time point expressed in the common duration type of @p value and @p delta.
 * @warning Addition is unchecked and may overflow the common duration representation.
 */
template <typename Clock, typename Duration1, typename Rep2, typename Period2>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<
    Clock,
    meta::common_type_t<Duration1, duration<Rep2, Period2>>>
operator+(
    CASTLE_CONST time_point<Clock, Duration1>& value,
    CASTLE_CONST duration<Rep2, Period2>& delta) CASTLE_NOEXCEPT
{
    using delta_type = duration<Rep2, Period2>;
    using result_duration = meta::common_type_t<Duration1, delta_type>;
    using result = time_point<Clock, result_duration>;

    return result(duration_cast<result_duration>(value.time_since_epoch()) +
                  duration_cast<result_duration>(delta));
}

/**
 * @brief Adds a duration to a time point with operands reversed.
 * @tparam Rep1 Delta representation type.
 * @tparam Period1 Delta period ratio.
 * @tparam Clock Clock type.
 * @tparam Duration2 Time-point duration type.
 * @param delta Duration to add.
 * @param value Base time point.
 * @return Time point expressed in the common duration type of @p delta and @p value.
 * @warning Addition is unchecked and may overflow the common duration representation.
 */
template <typename Rep1, typename Period1, typename Clock, typename Duration2>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<
    Clock,
    meta::common_type_t<duration<Rep1, Period1>, Duration2>>
operator+(
    CASTLE_CONST duration<Rep1, Period1>& delta,
    CASTLE_CONST time_point<Clock, Duration2>& value) CASTLE_NOEXCEPT
{
    return value + delta;
}

/**
 * @brief Subtracts a duration from a time point.
 * @tparam Clock Clock type.
 * @tparam Duration1 Time-point duration type.
 * @tparam Rep2 Delta representation type.
 * @tparam Period2 Delta period ratio.
 * @param value Base time point.
 * @param delta Duration to subtract.
 * @return Time point expressed in the common duration type of @p value and @p delta.
 * @warning Subtraction is unchecked and may overflow the common duration representation.
 */
template <typename Clock, typename Duration1, typename Rep2, typename Period2>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<
    Clock,
    meta::common_type_t<Duration1, duration<Rep2, Period2>>>
operator-(
    CASTLE_CONST time_point<Clock, Duration1>& value,
    CASTLE_CONST duration<Rep2, Period2>& delta) CASTLE_NOEXCEPT
{
    using delta_type = duration<Rep2, Period2>;
    using result_duration = meta::common_type_t<Duration1, delta_type>;
    using result = time_point<Clock, result_duration>;

    return result(duration_cast<result_duration>(value.time_since_epoch()) -
                  duration_cast<result_duration>(delta));
}

/**
 * @brief Computes the elapsed duration between two time points on the same clock.
 * @tparam Clock Clock type shared by both time points.
 * @tparam Duration1 Left-hand duration type.
 * @tparam Duration2 Right-hand duration type.
 * @param lhs Left-hand time point.
 * @param rhs Right-hand time point.
 * @return Duration expressed in the common duration type of @p lhs and @p rhs.
 * @warning Subtraction is unchecked and may overflow the common duration representation.
 */
template <typename Clock, typename Duration1, typename Duration2>
CASTLE_NODISCARD CASTLE_CONSTEXPR meta::common_type_t<Duration1, Duration2>
operator-(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    using result = meta::common_type_t<Duration1, Duration2>;

    return duration_cast<result>(lhs.time_since_epoch()) -
           duration_cast<result>(rhs.time_since_epoch());
}

/**
 * @brief Compares two time points on the same clock for equality.
 * @tparam Clock Clock type shared by both time points.
 * @tparam Duration1 Left-hand duration type.
 * @tparam Duration2 Right-hand duration type.
 * @param lhs Left-hand time point.
 * @param rhs Right-hand time point.
 * @return @c true when both time points represent the same instant, otherwise @c false.
 */
template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator==(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    using common_duration = meta::common_type_t<Duration1, Duration2>;

    return duration_cast<common_duration>(lhs.time_since_epoch()).count() ==
           duration_cast<common_duration>(rhs.time_since_epoch()).count();
}

/**
 * @brief Compares two time points on the same clock for inequality.
 * @tparam Clock Clock type shared by both time points.
 * @tparam Duration1 Left-hand duration type.
 * @tparam Duration2 Right-hand duration type.
 * @param lhs Left-hand time point.
 * @param rhs Right-hand time point.
 * @return @c true when both time points represent different instants, otherwise @c false.
 */
template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator!=(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/**
 * @brief Orders two time points on the same clock.
 * @tparam Clock Clock type shared by both time points.
 * @tparam Duration1 Left-hand duration type.
 * @tparam Duration2 Right-hand duration type.
 * @param lhs Left-hand time point.
 * @param rhs Right-hand time point.
 * @return @c true when @p lhs precedes @p rhs, otherwise @c false.
 */
template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator<(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    using common_duration = meta::common_type_t<Duration1, Duration2>;

    return duration_cast<common_duration>(lhs.time_since_epoch()).count() <
           duration_cast<common_duration>(rhs.time_since_epoch()).count();
}

/**
 * @brief Orders two time points on the same clock.
 * @tparam Clock Clock type shared by both time points.
 * @tparam Duration1 Left-hand duration type.
 * @tparam Duration2 Right-hand duration type.
 * @param lhs Left-hand time point.
 * @param rhs Right-hand time point.
 * @return @c true when @p lhs precedes or equals @p rhs, otherwise @c false.
 */
template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator<=(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    return !(rhs < lhs);
}

/**
 * @brief Orders two time points on the same clock.
 * @tparam Clock Clock type shared by both time points.
 * @tparam Duration1 Left-hand duration type.
 * @tparam Duration2 Right-hand duration type.
 * @param lhs Left-hand time point.
 * @param rhs Right-hand time point.
 * @return @c true when @p lhs follows @p rhs, otherwise @c false.
 */
template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator>(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    return rhs < lhs;
}

/**
 * @brief Orders two time points on the same clock.
 * @tparam Clock Clock type shared by both time points.
 * @tparam Duration1 Left-hand duration type.
 * @tparam Duration2 Right-hand duration type.
 * @param lhs Left-hand time point.
 * @param rhs Right-hand time point.
 * @return @c true when @p lhs follows or equals @p rhs, otherwise @c false.
 */
template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator>=(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs < rhs);
}

}
}

#endif
