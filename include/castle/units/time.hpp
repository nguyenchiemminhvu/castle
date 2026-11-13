// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file time.hpp
 * @brief Time units for Castle units: aliases of `castle::chrono::duration` plus time literals.
 *
 * Castle has a single time type: `castle::chrono::duration`. This header names
 * it in the units vocabulary (`castle::units::seconds<double>`, ...) and owns
 * the time literals. Units arithmetic accepts durations directly, so
 * `meters / seconds` yields `meters_per_second` and `meters / meters_per_second`
 * yields a duration.
 *
 * Example:
 * @code
 * #include "castle/units/time.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::chrono::milliseconds timeout = 250_ms;  // int64_t ticks
 * castle::units::seconds<double> dt = 0.02_s;     // floating ticks
 * @endcode
 *
 * Key constraints:
 * - Integer literals produce the `int64_t` chrono aliases (`chrono::milliseconds`, ...).
 * - Floating literals produce `double` durations.
 * - Duration conversions keep chrono semantics: changing period needs an explicit
 *   constructor or `chrono::duration_cast` / `units::quantity_cast`.
 */
#ifndef CASTLE_UNITS_TIME_HPP
#define CASTLE_UNITS_TIME_HPP

#include "castle/core/compiler.hpp"
#include "castle/chrono/duration.hpp"
#include "castle/units/quantity.hpp"
#include "castle/units/scale.hpp"

#include <stdint.h>

namespace castle
{
namespace units
{

/// @brief Duration in nanoseconds with representation `Rep`.
template <typename Rep>
using nanoseconds = chrono::duration<Rep, nano>;

/// @brief Duration in microseconds with representation `Rep`.
template <typename Rep>
using microseconds = chrono::duration<Rep, micro>;

/// @brief Duration in milliseconds with representation `Rep`.
template <typename Rep>
using milliseconds = chrono::duration<Rep, milli>;

/// @brief Duration in seconds with representation `Rep`.
template <typename Rep>
using seconds = chrono::duration<Rep, second>;

/// @brief Duration in minutes with representation `Rep`.
template <typename Rep>
using minutes = chrono::duration<Rep, minute>;

/// @brief Duration in hours with representation `Rep`.
template <typename Rep>
using hours = chrono::duration<Rep, hour>;

/// @brief Duration in days with representation `Rep`.
template <typename Rep>
using days = chrono::duration<Rep, day>;

/// @brief Duration in weeks with representation `Rep`.
template <typename Rep>
using weeks = chrono::duration<Rep, week>;

namespace literals
{

/// @brief Integer nanoseconds literal, e.g. `100_ns`.
CASTLE_CONSTEXPR chrono::nanoseconds operator""_ns(unsigned long long value) CASTLE_NOEXCEPT
{
    return chrono::nanoseconds(static_cast<int64_t>(value));
}

/// @brief Floating nanoseconds literal, e.g. `2.5_ns`.
CASTLE_CONSTEXPR nanoseconds<double> operator""_ns(long double value) CASTLE_NOEXCEPT
{
    return nanoseconds<double>(static_cast<double>(value));
}

/// @brief Integer microseconds literal, e.g. `100_us`.
CASTLE_CONSTEXPR chrono::microseconds operator""_us(unsigned long long value) CASTLE_NOEXCEPT
{
    return chrono::microseconds(static_cast<int64_t>(value));
}

/// @brief Floating microseconds literal, e.g. `2.5_us`.
CASTLE_CONSTEXPR microseconds<double> operator""_us(long double value) CASTLE_NOEXCEPT
{
    return microseconds<double>(static_cast<double>(value));
}

/// @brief Integer milliseconds literal, e.g. `250_ms`.
CASTLE_CONSTEXPR chrono::milliseconds operator""_ms(unsigned long long value) CASTLE_NOEXCEPT
{
    return chrono::milliseconds(static_cast<int64_t>(value));
}

/// @brief Floating milliseconds literal, e.g. `2.5_ms`.
CASTLE_CONSTEXPR milliseconds<double> operator""_ms(long double value) CASTLE_NOEXCEPT
{
    return milliseconds<double>(static_cast<double>(value));
}

/// @brief Integer seconds literal, e.g. `2_s`.
CASTLE_CONSTEXPR chrono::seconds operator""_s(unsigned long long value) CASTLE_NOEXCEPT
{
    return chrono::seconds(static_cast<int64_t>(value));
}

/// @brief Floating seconds literal, e.g. `0.02_s`.
CASTLE_CONSTEXPR seconds<double> operator""_s(long double value) CASTLE_NOEXCEPT
{
    return seconds<double>(static_cast<double>(value));
}

/// @brief Integer minutes literal, e.g. `5_min`.
CASTLE_CONSTEXPR chrono::minutes operator""_min(unsigned long long value) CASTLE_NOEXCEPT
{
    return chrono::minutes(static_cast<int64_t>(value));
}

/// @brief Floating minutes literal, e.g. `1.5_min`.
CASTLE_CONSTEXPR minutes<double> operator""_min(long double value) CASTLE_NOEXCEPT
{
    return minutes<double>(static_cast<double>(value));
}

/// @brief Integer hours literal, e.g. `6_h`.
CASTLE_CONSTEXPR chrono::hours operator""_h(unsigned long long value) CASTLE_NOEXCEPT
{
    return chrono::hours(static_cast<int64_t>(value));
}

/// @brief Floating hours literal, e.g. `1.5_h`.
CASTLE_CONSTEXPR hours<double> operator""_h(long double value) CASTLE_NOEXCEPT
{
    return hours<double>(static_cast<double>(value));
}

/// @brief Integer days literal, e.g. `7_d`.
CASTLE_CONSTEXPR chrono::days operator""_d(unsigned long long value) CASTLE_NOEXCEPT
{
    return chrono::days(static_cast<int64_t>(value));
}

/// @brief Floating days literal, e.g. `0.5_d`.
CASTLE_CONSTEXPR days<double> operator""_d(long double value) CASTLE_NOEXCEPT
{
    return days<double>(static_cast<double>(value));
}

/// @brief Integer weeks literal, e.g. `2_w`.
CASTLE_CONSTEXPR chrono::weeks operator""_w(unsigned long long value) CASTLE_NOEXCEPT
{
    return chrono::weeks(static_cast<int64_t>(value));
}

/// @brief Floating weeks literal, e.g. `0.5_w`.
CASTLE_CONSTEXPR weeks<double> operator""_w(long double value) CASTLE_NOEXCEPT
{
    return weeks<double>(static_cast<double>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_TIME_HPP
