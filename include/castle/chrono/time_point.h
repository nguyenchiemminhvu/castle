#ifndef CASTLE_CHRONO_TIME_POINT_H
#define CASTLE_CHRONO_TIME_POINT_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/chrono/duration.h"

namespace castle
{
namespace chrono
{

// ============================================================================
// time_point
// ============================================================================
// An instant associated with a Clock. The object stores only the duration from
// the Clock's epoch, so its footprint is exactly the size of Duration in normal
// ABI layouts.
// ============================================================================
template <typename Clock, typename Duration = typename Clock::duration>
class time_point
{
public:
    using clock = Clock;
    using duration = Duration;
    using rep = typename duration::rep;
    using period = typename duration::period;

    CASTLE_CONSTEXPR time_point() CASTLE_NOEXCEPT
        : value_()
    {
    }

    explicit CASTLE_CONSTEXPR time_point(CASTLE_CONST duration& value) CASTLE_NOEXCEPT
        : value_(value)
    {
    }

    template <typename Duration2>
    explicit CASTLE_CONSTEXPR time_point(
        CASTLE_CONST time_point<Clock, Duration2>& other) CASTLE_NOEXCEPT
        : value_(duration_cast<duration>(other.time_since_epoch()))
    {
    }

    time_point(CASTLE_CONST time_point&) CASTLE_DEFAULT;
    time_point& operator=(CASTLE_CONST time_point&) CASTLE_DEFAULT;

    CASTLE_NODISCARD CASTLE_CONSTEXPR duration time_since_epoch() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return value_;
    }

    CASTLE_NODISCARD static CASTLE_CONSTEXPR time_point min() CASTLE_NOEXCEPT
    {
        return time_point(duration::min());
    }

    CASTLE_NODISCARD static CASTLE_CONSTEXPR time_point max() CASTLE_NOEXCEPT
    {
        return time_point(duration::max());
    }

    CASTLE_CONSTEXPR time_point& operator+=(CASTLE_CONST duration& delta) CASTLE_NOEXCEPT
    {
        value_ += delta;
        return *this;
    }

    CASTLE_CONSTEXPR time_point& operator-=(CASTLE_CONST duration& delta) CASTLE_NOEXCEPT
    {
        value_ -= delta;
        return *this;
    }

    CASTLE_CONSTEXPR time_point& operator++() CASTLE_NOEXCEPT
    {
        ++value_;
        return *this;
    }

    CASTLE_CONSTEXPR time_point operator++(int) CASTLE_NOEXCEPT
    {
        time_point temp(*this);
        ++value_;
        return temp;
    }

    CASTLE_CONSTEXPR time_point& operator--() CASTLE_NOEXCEPT
    {
        --value_;
        return *this;
    }

    CASTLE_CONSTEXPR time_point operator--(int) CASTLE_NOEXCEPT
    {
        time_point temp(*this);
        --value_;
        return temp;
    }

private:
    duration value_;
};

// ============================================================================
// time_point_cast
// ============================================================================
template <typename ToDuration, typename Clock, typename Duration>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<Clock, ToDuration>
time_point_cast(CASTLE_CONST time_point<Clock, Duration>& value) CASTLE_NOEXCEPT
{
    return time_point<Clock, ToDuration>(
        duration_cast<ToDuration>(value.time_since_epoch())
    );
}

template <typename ToDuration, typename Clock, typename Duration>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<Clock, ToDuration>
floor(CASTLE_CONST time_point<Clock, Duration>& value) CASTLE_NOEXCEPT
{
    return time_point<Clock, ToDuration>(
        chrono::floor<ToDuration>(value.time_since_epoch())
    );
}

template <typename ToDuration, typename Clock, typename Duration>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<Clock, ToDuration>
ceil(CASTLE_CONST time_point<Clock, Duration>& value) CASTLE_NOEXCEPT
{
    return time_point<Clock, ToDuration>(
        chrono::ceil<ToDuration>(value.time_since_epoch())
    );
}

template <typename ToDuration, typename Clock, typename Duration>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<Clock, ToDuration>
round(CASTLE_CONST time_point<Clock, Duration>& value) CASTLE_NOEXCEPT
{
    return time_point<Clock, ToDuration>(
        chrono::round<ToDuration>(value.time_since_epoch())
    );
}

// ============================================================================
// time_point + / - duration
// ============================================================================
template <typename Clock, typename Duration1, typename Rep2, typename Period2>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<
    Clock,
    castle::common_type_t<Duration1, duration<Rep2, Period2>>>
operator+(
    CASTLE_CONST time_point<Clock, Duration1>& value,
    CASTLE_CONST duration<Rep2, Period2>& delta) CASTLE_NOEXCEPT
{
    using delta_type = duration<Rep2, Period2>;
    using result_duration = castle::common_type_t<Duration1, delta_type>;
    using result = time_point<Clock, result_duration>;

    return result(duration_cast<result_duration>(value.time_since_epoch()) +
                  duration_cast<result_duration>(delta));
}

template <typename Rep1, typename Period1, typename Clock, typename Duration2>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<
    Clock,
    castle::common_type_t<duration<Rep1, Period1>, Duration2>>
operator+(
    CASTLE_CONST duration<Rep1, Period1>& delta,
    CASTLE_CONST time_point<Clock, Duration2>& value) CASTLE_NOEXCEPT
{
    return value + delta;
}

template <typename Clock, typename Duration1, typename Rep2, typename Period2>
CASTLE_NODISCARD CASTLE_CONSTEXPR time_point<
    Clock,
    castle::common_type_t<Duration1, duration<Rep2, Period2>>>
operator-(
    CASTLE_CONST time_point<Clock, Duration1>& value,
    CASTLE_CONST duration<Rep2, Period2>& delta) CASTLE_NOEXCEPT
{
    using delta_type = duration<Rep2, Period2>;
    using result_duration = castle::common_type_t<Duration1, delta_type>;
    using result = time_point<Clock, result_duration>;

    return result(duration_cast<result_duration>(value.time_since_epoch()) -
                  duration_cast<result_duration>(delta));
}

// ============================================================================
// Difference between time points
// ============================================================================
template <typename Clock, typename Duration1, typename Duration2>
CASTLE_NODISCARD CASTLE_CONSTEXPR castle::common_type_t<Duration1, Duration2>
operator-(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    using result = castle::common_type_t<Duration1, Duration2>;

    return duration_cast<result>(lhs.time_since_epoch()) -
           duration_cast<result>(rhs.time_since_epoch());
}

// ============================================================================
// time_point comparisons
// ============================================================================
template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator==(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    using common_duration = castle::common_type_t<Duration1, Duration2>;

    return duration_cast<common_duration>(lhs.time_since_epoch()).count() ==
           duration_cast<common_duration>(rhs.time_since_epoch()).count();
}

template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator!=(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator<(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    using common_duration = castle::common_type_t<Duration1, Duration2>;

    return duration_cast<common_duration>(lhs.time_since_epoch()).count() <
           duration_cast<common_duration>(rhs.time_since_epoch()).count();
}

template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator<=(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    return !(rhs < lhs);
}

template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator>(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    return rhs < lhs;
}

template <typename Clock, typename Duration1, typename Duration2>
CASTLE_CONSTEXPR bool
operator>=(
    CASTLE_CONST time_point<Clock, Duration1>& lhs,
    CASTLE_CONST time_point<Clock, Duration2>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs < rhs);
}

} // namespace chrono
} // namespace castle

#endif // CASTLE_CHRONO_TIME_POINT_H
