#ifndef CASTLE_CHRONO_LITERALS_H
#define CASTLE_CHRONO_LITERALS_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/chrono/duration.h"

namespace castle
{
namespace chrono
{
namespace literals
{
namespace chrono_literals
{

CASTLE_CONSTEXPR nanoseconds operator""_ns(unsigned long long value) CASTLE_NOEXCEPT
{
    return nanoseconds(static_cast<int64_t>(value));
}

CASTLE_CONSTEXPR microseconds operator""_us(unsigned long long value) CASTLE_NOEXCEPT
{
    return microseconds(static_cast<int64_t>(value));
}

CASTLE_CONSTEXPR milliseconds operator""_ms(unsigned long long value) CASTLE_NOEXCEPT
{
    return milliseconds(static_cast<int64_t>(value));
}

CASTLE_CONSTEXPR seconds operator""_s(unsigned long long value) CASTLE_NOEXCEPT
{
    return seconds(static_cast<int64_t>(value));
}

CASTLE_CONSTEXPR minutes operator""_min(unsigned long long value) CASTLE_NOEXCEPT
{
    return minutes(static_cast<int64_t>(value));
}

CASTLE_CONSTEXPR hours operator""_h(unsigned long long value) CASTLE_NOEXCEPT
{
    return hours(static_cast<int64_t>(value));
}

CASTLE_CONSTEXPR days operator""_d(unsigned long long value) CASTLE_NOEXCEPT
{
    return days(static_cast<int64_t>(value));
}

CASTLE_CONSTEXPR weeks operator""_w(unsigned long long value) CASTLE_NOEXCEPT
{
    return weeks(static_cast<int64_t>(value));
}

} // namespace chrono_literals
} // namespace literals
} // namespace chrono
} // namespace castle

#endif // CASTLE_CHRONO_LITERALS_H
