// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file temperature.hpp
 * @brief Thermodynamic temperature units: kelvin plus the affine Celsius and Fahrenheit scales.
 *
 * Example:
 * @code
 * #include "castle/units/temperature.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::kelvin<double> k = 25.0_degC;                       // implicit: 298.15 K
 * castle::units::fahrenheit<int32_t> f(castle::units::celsius<int32_t>(100)); // explicit: 212 degF
 * bool same = (-40_degC == -40_degF);                                // true
 * castle::units::celsius<double> c = castle::units::deci_celsius<int16_t>(253); // 25.3 degC
 * @endcode
 *
 * Key constraints (affine rules):
 * - Celsius, Fahrenheit and deci-Celsius are offset units: conversions apply the
 *   origin offset, so they are only implicit into a floating representation or
 *   when scale and offset are whole numbers; integral results truncate toward zero.
 * - Comparisons work across all temperature units.
 * - `+` and `-` work between two values of the same unit, or between units
 *   without offset (kelvin, millikelvin). Mixing two different offset units
 *   (for example Celsius and Fahrenheit) is rejected at compile time.
 * - Offset units cannot be multiplied, divided or inverted; convert to kelvin first.
 * - Express temperature differences in kelvin: a Celsius value always denotes a
 *   point on the Celsius scale, not an interval.
 */
#ifndef CASTLE_UNITS_TEMPERATURE_HPP
#define CASTLE_UNITS_TEMPERATURE_HPP

#include "castle/core/compiler.hpp"
#include "castle/units/quantity.hpp"
#include "castle/units/scale.hpp"

#include <stdint.h>

namespace castle
{
namespace units
{

/// @brief Kelvin, the coherent temperature unit.
using kelvin_unit = unit<dim::temperature>;
/// @brief Millikelvin.
using millikelvin_unit = scaled_unit_t<kelvin_unit, milli>;
/// @brief Degree Celsius: 1 K steps, zero at 273.15 K.
using celsius_unit = unit<dim::temperature, math::ratio<1>, 0, math::ratio<27315, 100>>;
/// @brief Degree Fahrenheit: 5/9 K steps, zero at 459.67 / 1.8 K.
using fahrenheit_unit = unit<dim::temperature, math::ratio<5, 9>, 0, math::ratio<45967, 180>>;
/// @brief Tenth of a degree Celsius, the raw format of many int16 temperature sensors.
using deci_celsius_unit = scaled_unit_t<celsius_unit, deci>;

/// @brief Temperature in kelvin.
template <typename Rep>
using kelvin = quantity<kelvin_unit, Rep>;
/// @brief Temperature in millikelvin.
template <typename Rep>
using millikelvin = quantity<millikelvin_unit, Rep>;
/// @brief Temperature in degrees Celsius.
template <typename Rep>
using celsius = quantity<celsius_unit, Rep>;
/// @brief Temperature in degrees Fahrenheit.
template <typename Rep>
using fahrenheit = quantity<fahrenheit_unit, Rep>;
/// @brief Temperature in tenths of a degree Celsius.
template <typename Rep>
using deci_celsius = quantity<deci_celsius_unit, Rep>;

namespace literals
{

/// @brief Floating kelvin literal, e.g. `1.5_K`.
CASTLE_CONSTEXPR kelvin<double> operator""_K(long double value) CASTLE_NOEXCEPT
{
    return kelvin<double>(static_cast<double>(value));
}

/// @brief Integer kelvin literal, e.g. `15_K`.
CASTLE_CONSTEXPR kelvin<int64_t> operator""_K(unsigned long long value) CASTLE_NOEXCEPT
{
    return kelvin<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating millikelvin literal, e.g. `1.5_mK`.
CASTLE_CONSTEXPR millikelvin<double> operator""_mK(long double value) CASTLE_NOEXCEPT
{
    return millikelvin<double>(static_cast<double>(value));
}

/// @brief Integer millikelvin literal, e.g. `15_mK`.
CASTLE_CONSTEXPR millikelvin<int64_t> operator""_mK(unsigned long long value) CASTLE_NOEXCEPT
{
    return millikelvin<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating degrees Celsius literal, e.g. `1.5_degC`.
CASTLE_CONSTEXPR celsius<double> operator""_degC(long double value) CASTLE_NOEXCEPT
{
    return celsius<double>(static_cast<double>(value));
}

/// @brief Integer degrees Celsius literal, e.g. `15_degC`.
CASTLE_CONSTEXPR celsius<int64_t> operator""_degC(unsigned long long value) CASTLE_NOEXCEPT
{
    return celsius<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating degrees Fahrenheit literal, e.g. `1.5_degF`.
CASTLE_CONSTEXPR fahrenheit<double> operator""_degF(long double value) CASTLE_NOEXCEPT
{
    return fahrenheit<double>(static_cast<double>(value));
}

/// @brief Integer degrees Fahrenheit literal, e.g. `15_degF`.
CASTLE_CONSTEXPR fahrenheit<int64_t> operator""_degF(unsigned long long value) CASTLE_NOEXCEPT
{
    return fahrenheit<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_TEMPERATURE_HPP
