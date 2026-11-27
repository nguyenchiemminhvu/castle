// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file voltage.hpp
 * @brief Electric potential units: SI multiples of the volt.
 *
 * Example:
 * @code
 * #include "castle/units/voltage.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::millivolts<int32_t> rail = 3_V + 300_mV;   // 3300 mV
 * castle::units::volts<double> reading = 1650.0_mV;         // implicit, lossless
 * @endcode
 *
 * Key constraints:
 * - `dim::voltage` is M L^2 T^-3 I^-1, so `volts / amperes` gives ohms and
 *   `volts * amperes` gives watts when the matching headers are included.
 */
#ifndef CASTLE_UNITS_VOLTAGE_HPP
#define CASTLE_UNITS_VOLTAGE_HPP

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
using voltage = dimension<2, 1, -3, -1, 0, 0, 0, 0>; ///< Electric potential, M L^2 T^-3 I^-1.
} // namespace dim

/// @brief Volt, the coherent voltage unit.
using volt_unit = unit<dim::voltage>;
/// @brief Kilovolt.
using kilovolt_unit = scaled_unit_t<volt_unit, kilo>;
/// @brief Millivolt.
using millivolt_unit = scaled_unit_t<volt_unit, milli>;
/// @brief Microvolt.
using microvolt_unit = scaled_unit_t<volt_unit, micro>;

/// @brief Voltage in volts.
template <typename Rep>
using volts = quantity<volt_unit, Rep>;
/// @brief Voltage in kilovolts.
template <typename Rep>
using kilovolts = quantity<kilovolt_unit, Rep>;
/// @brief Voltage in millivolts.
template <typename Rep>
using millivolts = quantity<millivolt_unit, Rep>;
/// @brief Voltage in microvolts.
template <typename Rep>
using microvolts = quantity<microvolt_unit, Rep>;

namespace literals
{

/// @brief Floating volts literal, e.g. `1.5_V`.
CASTLE_CONSTEXPR volts<double> operator""_V(long double value) CASTLE_NOEXCEPT
{
    return volts<double>(static_cast<double>(value));
}

/// @brief Integer volts literal, e.g. `15_V`.
CASTLE_CONSTEXPR volts<int64_t> operator""_V(unsigned long long value) CASTLE_NOEXCEPT
{
    return volts<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating kilovolts literal, e.g. `1.5_kV`.
CASTLE_CONSTEXPR kilovolts<double> operator""_kV(long double value) CASTLE_NOEXCEPT
{
    return kilovolts<double>(static_cast<double>(value));
}

/// @brief Integer kilovolts literal, e.g. `15_kV`.
CASTLE_CONSTEXPR kilovolts<int64_t> operator""_kV(unsigned long long value) CASTLE_NOEXCEPT
{
    return kilovolts<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating millivolts literal, e.g. `1.5_mV`.
CASTLE_CONSTEXPR millivolts<double> operator""_mV(long double value) CASTLE_NOEXCEPT
{
    return millivolts<double>(static_cast<double>(value));
}

/// @brief Integer millivolts literal, e.g. `15_mV`.
CASTLE_CONSTEXPR millivolts<int64_t> operator""_mV(unsigned long long value) CASTLE_NOEXCEPT
{
    return millivolts<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating microvolts literal, e.g. `1.5_uV`.
CASTLE_CONSTEXPR microvolts<double> operator""_uV(long double value) CASTLE_NOEXCEPT
{
    return microvolts<double>(static_cast<double>(value));
}

/// @brief Integer microvolts literal, e.g. `15_uV`.
CASTLE_CONSTEXPR microvolts<int64_t> operator""_uV(unsigned long long value) CASTLE_NOEXCEPT
{
    return microvolts<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_VOLTAGE_HPP
