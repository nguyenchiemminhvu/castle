// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file literals.hpp
 * @brief User-defined literals for Castle chrono duration aliases.
 *
 * Include this header when readable integer time constants are preferable to
 * explicit duration constructors. Each literal stores its result in the matching
 * Castle duration alias with an @c int64_t representation.
 *
 * Key constraints:
 * - Literal units map directly to Castle duration aliases: ns, us, ms, s, min, h, d, and w.
 * - Literals are compile-time conveniences only; they do not change clock monotonicity or resolution.
 * - Arithmetic on the resulting durations is still subject to normal duration overflow limits.
 *
 * Example:
 * @code
 * #include "castle/chrono/literals.hpp"
 *
 * using namespace castle::chrono::literals::chrono_literals;
 *
 * int main()
 * {
 *     const auto period = 10_ms;
 *     const auto timeout = 2_s;
 *     (void)period;
 *     (void)timeout;
 *     return 0;
 * }
 * @endcode
 */

#ifndef CASTLE_CHRONO_LITERALS_HPP
#define CASTLE_CHRONO_LITERALS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/chrono/duration.hpp"

namespace castle
{
namespace chrono
{
namespace literals
{
namespace chrono_literals
{

/**
 * @brief Constructs a nanosecond duration from an integer literal.
 * @param value Nanosecond count.
 * @return A @c castle::chrono::nanoseconds storing @p value.
 * @note The stored representation type is @c int64_t.
 * @warning Values outside the representable @c int64_t range are subject to integer conversion limits.
 */
CASTLE_CONSTEXPR nanoseconds operator""_ns(unsigned long long value) CASTLE_NOEXCEPT
{
    return nanoseconds(static_cast<int64_t>(value));
}

/**
 * @brief Constructs a microsecond duration from an integer literal.
 * @param value Microsecond count.
 * @return A @c castle::chrono::microseconds storing @p value.
 * @note The stored representation type is @c int64_t.
 * @warning Values outside the representable @c int64_t range are subject to integer conversion limits.
 */
CASTLE_CONSTEXPR microseconds operator""_us(unsigned long long value) CASTLE_NOEXCEPT
{
    return microseconds(static_cast<int64_t>(value));
}

/**
 * @brief Constructs a millisecond duration from an integer literal.
 * @param value Millisecond count.
 * @return A @c castle::chrono::milliseconds storing @p value.
 * @note The stored representation type is @c int64_t.
 * @warning Values outside the representable @c int64_t range are subject to integer conversion limits.
 */
CASTLE_CONSTEXPR milliseconds operator""_ms(unsigned long long value) CASTLE_NOEXCEPT
{
    return milliseconds(static_cast<int64_t>(value));
}

/**
 * @brief Constructs a second duration from an integer literal.
 * @param value Second count.
 * @return A @c castle::chrono::seconds storing @p value.
 * @note The stored representation type is @c int64_t.
 * @warning Values outside the representable @c int64_t range are subject to integer conversion limits.
 */
CASTLE_CONSTEXPR seconds operator""_s(unsigned long long value) CASTLE_NOEXCEPT
{
    return seconds(static_cast<int64_t>(value));
}

/**
 * @brief Constructs a minute duration from an integer literal.
 * @param value Minute count.
 * @return A @c castle::chrono::minutes storing @p value.
 * @note The stored representation type is @c int64_t.
 * @warning Values outside the representable @c int64_t range are subject to integer conversion limits.
 */
CASTLE_CONSTEXPR minutes operator""_min(unsigned long long value) CASTLE_NOEXCEPT
{
    return minutes(static_cast<int64_t>(value));
}

/**
 * @brief Constructs an hour duration from an integer literal.
 * @param value Hour count.
 * @return A @c castle::chrono::hours storing @p value.
 * @note The stored representation type is @c int64_t.
 * @warning Values outside the representable @c int64_t range are subject to integer conversion limits.
 */
CASTLE_CONSTEXPR hours operator""_h(unsigned long long value) CASTLE_NOEXCEPT
{
    return hours(static_cast<int64_t>(value));
}

/**
 * @brief Constructs a day duration from an integer literal.
 * @param value Day count.
 * @return A @c castle::chrono::days storing @p value.
 * @note The stored representation type is @c int64_t.
 * @warning Values outside the representable @c int64_t range are subject to integer conversion limits.
 */
CASTLE_CONSTEXPR days operator""_d(unsigned long long value) CASTLE_NOEXCEPT
{
    return days(static_cast<int64_t>(value));
}

/**
 * @brief Constructs a week duration from an integer literal.
 * @param value Week count.
 * @return A @c castle::chrono::weeks storing @p value.
 * @note The stored representation type is @c int64_t.
 * @warning Values outside the representable @c int64_t range are subject to integer conversion limits.
 */
CASTLE_CONSTEXPR weeks operator""_w(unsigned long long value) CASTLE_NOEXCEPT
{
    return weeks(static_cast<int64_t>(value));
}

}
}
}
}

#endif
