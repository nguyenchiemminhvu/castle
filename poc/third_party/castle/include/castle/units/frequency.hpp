// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file frequency.hpp
 * @brief Frequency units: the hertz and its SI multiples.
 *
 * Example:
 * @code
 * #include "castle/units/frequency.hpp"
 * #include "castle/units/time.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::hertz<double> rate = castle::units::inverse(20.0_ms); // 50 Hz
 * castle::units::seconds<double> period = 1.0 / 2.0_kHz;               // 0.0005 s
 * double cycles = 50.0_Hz * 2.0_s;                                      // 100 cycles
 * @endcode
 *
 * Key constraints:
 * - Frequency is cycles per time (T^-1) with no angle exponent, so it never mixes
 *   with angular velocity (`units/angular_velocity.hpp`).
 * - The reciprocal of a frequency is a `castle::chrono::duration` in seconds, and
 *   frequency times a duration is a plain cycle count.
 */
#ifndef CASTLE_UNITS_FREQUENCY_HPP
#define CASTLE_UNITS_FREQUENCY_HPP

#include "castle/core/compiler.hpp"
#include "castle/units/quantity.hpp"
#include "castle/units/scale.hpp"

#include <stdint.h>

namespace castle
{
namespace units
{

namespace dim
{
using frequency = dimension<0, 0, -1, 0, 0, 0, 0, 0>; ///< T^-1, cycles per time.
} // namespace dim

/// @brief Hertz, the coherent frequency unit.
using hertz_unit = unit<dim::frequency>;
/// @brief Millihertz.
using millihertz_unit = scaled_unit_t<hertz_unit, milli>;
/// @brief Kilohertz.
using kilohertz_unit = scaled_unit_t<hertz_unit, kilo>;
/// @brief Megahertz.
using megahertz_unit = scaled_unit_t<hertz_unit, mega>;
/// @brief Gigahertz.
using gigahertz_unit = scaled_unit_t<hertz_unit, giga>;

/// @brief Frequency in hertz.
template <typename Rep>
using hertz = quantity<hertz_unit, Rep>;
/// @brief Frequency in millihertz.
template <typename Rep>
using millihertz = quantity<millihertz_unit, Rep>;
/// @brief Frequency in kilohertz.
template <typename Rep>
using kilohertz = quantity<kilohertz_unit, Rep>;
/// @brief Frequency in megahertz.
template <typename Rep>
using megahertz = quantity<megahertz_unit, Rep>;
/// @brief Frequency in gigahertz.
template <typename Rep>
using gigahertz = quantity<gigahertz_unit, Rep>;

namespace literals
{

/// @brief Floating hertz literal, e.g. `1.5_Hz`.
CASTLE_CONSTEXPR hertz<double> operator""_Hz(long double value) CASTLE_NOEXCEPT
{
    return hertz<double>(static_cast<double>(value));
}

/// @brief Integer hertz literal, e.g. `15_Hz`.
CASTLE_CONSTEXPR hertz<int64_t> operator""_Hz(unsigned long long value) CASTLE_NOEXCEPT
{
    return hertz<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating millihertz literal, e.g. `1.5_mHz`.
CASTLE_CONSTEXPR millihertz<double> operator""_mHz(long double value) CASTLE_NOEXCEPT
{
    return millihertz<double>(static_cast<double>(value));
}

/// @brief Integer millihertz literal, e.g. `15_mHz`.
CASTLE_CONSTEXPR millihertz<int64_t> operator""_mHz(unsigned long long value) CASTLE_NOEXCEPT
{
    return millihertz<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating kilohertz literal, e.g. `1.5_kHz`.
CASTLE_CONSTEXPR kilohertz<double> operator""_kHz(long double value) CASTLE_NOEXCEPT
{
    return kilohertz<double>(static_cast<double>(value));
}

/// @brief Integer kilohertz literal, e.g. `15_kHz`.
CASTLE_CONSTEXPR kilohertz<int64_t> operator""_kHz(unsigned long long value) CASTLE_NOEXCEPT
{
    return kilohertz<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating megahertz literal, e.g. `1.5_MHz`.
CASTLE_CONSTEXPR megahertz<double> operator""_MHz(long double value) CASTLE_NOEXCEPT
{
    return megahertz<double>(static_cast<double>(value));
}

/// @brief Integer megahertz literal, e.g. `15_MHz`.
CASTLE_CONSTEXPR megahertz<int64_t> operator""_MHz(unsigned long long value) CASTLE_NOEXCEPT
{
    return megahertz<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating gigahertz literal, e.g. `1.5_GHz`.
CASTLE_CONSTEXPR gigahertz<double> operator""_GHz(long double value) CASTLE_NOEXCEPT
{
    return gigahertz<double>(static_cast<double>(value));
}

/// @brief Integer gigahertz literal, e.g. `15_GHz`.
CASTLE_CONSTEXPR gigahertz<int64_t> operator""_GHz(unsigned long long value) CASTLE_NOEXCEPT
{
    return gigahertz<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_FREQUENCY_HPP
