// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file pressure.hpp
 * @brief Pressure units: SI multiples of the pascal plus bar, standard atmosphere and psi.
 *
 * Example:
 * @code
 * #include "castle/units/pressure.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::hectopascals<double> qnh = 1.01325_bar;    // 1013.25 hPa, implicit
 * castle::units::pascals<int64_t> tyre = 2_bar + 150_hPa;   // 215000 Pa
 * @endcode
 *
 * Key constraints:
 * - Non-SI units use exact definitions (1 bar = 100000 Pa, 1 atm = 101325 Pa,
 *   1 psi = 0.45359237 kg * 9.80665 m/s^2 / (0.0254 m)^2 = 44482216152605 / 6451600000 Pa).
 */
#ifndef CASTLE_UNITS_PRESSURE_HPP
#define CASTLE_UNITS_PRESSURE_HPP

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
using pressure = dimension<-1, 1, -2, 0, 0, 0, 0, 0>; ///< Pressure, M L^-1 T^-2.
} // namespace dim

/// @brief Pascal, the coherent pressure unit.
using pascal_unit = unit<dim::pressure>;
/// @brief Hectopascal, numerically equal to the millibar.
using hectopascal_unit = scaled_unit_t<pascal_unit, hecto>;
/// @brief Kilopascal.
using kilopascal_unit = scaled_unit_t<pascal_unit, kilo>;
/// @brief Megapascal.
using megapascal_unit = scaled_unit_t<pascal_unit, mega>;
/// @brief Bar, 100000 Pa.
using bar_unit = unit<dim::pressure, math::ratio<100000>>;
/// @brief Millibar, 100 Pa.
using millibar_unit = unit<dim::pressure, math::ratio<100>>;
/// @brief Standard atmosphere, 101325 Pa.
using atmosphere_unit = unit<dim::pressure, math::ratio<101325>>;
/// @brief Pound-force per square inch, 44482216152605 / 6451600000 Pa (about 6894.757 Pa).
using pound_per_square_inch_unit = unit<dim::pressure, math::ratio<44482216152605, 6451600000>>;

/// @brief Pressure in pascals.
template <typename Rep>
using pascals = quantity<pascal_unit, Rep>;
/// @brief Pressure in hectopascals.
template <typename Rep>
using hectopascals = quantity<hectopascal_unit, Rep>;
/// @brief Pressure in kilopascals.
template <typename Rep>
using kilopascals = quantity<kilopascal_unit, Rep>;
/// @brief Pressure in megapascals.
template <typename Rep>
using megapascals = quantity<megapascal_unit, Rep>;
/// @brief Pressure in bars.
template <typename Rep>
using bars = quantity<bar_unit, Rep>;
/// @brief Pressure in millibars.
template <typename Rep>
using millibars = quantity<millibar_unit, Rep>;
/// @brief Pressure in standard atmospheres.
template <typename Rep>
using atmospheres = quantity<atmosphere_unit, Rep>;
/// @brief Pressure in pounds per square inch.
template <typename Rep>
using pounds_per_square_inch = quantity<pound_per_square_inch_unit, Rep>;

namespace literals
{

/// @brief Floating pascals literal, e.g. `1.5_Pa`.
CASTLE_CONSTEXPR pascals<double> operator""_Pa(long double value) CASTLE_NOEXCEPT
{
    return pascals<double>(static_cast<double>(value));
}

/// @brief Integer pascals literal, e.g. `15_Pa`.
CASTLE_CONSTEXPR pascals<int64_t> operator""_Pa(unsigned long long value) CASTLE_NOEXCEPT
{
    return pascals<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating hectopascals literal, e.g. `1.5_hPa`.
CASTLE_CONSTEXPR hectopascals<double> operator""_hPa(long double value) CASTLE_NOEXCEPT
{
    return hectopascals<double>(static_cast<double>(value));
}

/// @brief Integer hectopascals literal, e.g. `15_hPa`.
CASTLE_CONSTEXPR hectopascals<int64_t> operator""_hPa(unsigned long long value) CASTLE_NOEXCEPT
{
    return hectopascals<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating kilopascals literal, e.g. `1.5_kPa`.
CASTLE_CONSTEXPR kilopascals<double> operator""_kPa(long double value) CASTLE_NOEXCEPT
{
    return kilopascals<double>(static_cast<double>(value));
}

/// @brief Integer kilopascals literal, e.g. `15_kPa`.
CASTLE_CONSTEXPR kilopascals<int64_t> operator""_kPa(unsigned long long value) CASTLE_NOEXCEPT
{
    return kilopascals<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating megapascals literal, e.g. `1.5_MPa`.
CASTLE_CONSTEXPR megapascals<double> operator""_MPa(long double value) CASTLE_NOEXCEPT
{
    return megapascals<double>(static_cast<double>(value));
}

/// @brief Integer megapascals literal, e.g. `15_MPa`.
CASTLE_CONSTEXPR megapascals<int64_t> operator""_MPa(unsigned long long value) CASTLE_NOEXCEPT
{
    return megapascals<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating bars literal, e.g. `1.5_bar`.
CASTLE_CONSTEXPR bars<double> operator""_bar(long double value) CASTLE_NOEXCEPT
{
    return bars<double>(static_cast<double>(value));
}

/// @brief Integer bars literal, e.g. `15_bar`.
CASTLE_CONSTEXPR bars<int64_t> operator""_bar(unsigned long long value) CASTLE_NOEXCEPT
{
    return bars<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating millibars literal, e.g. `1.5_mbar`.
CASTLE_CONSTEXPR millibars<double> operator""_mbar(long double value) CASTLE_NOEXCEPT
{
    return millibars<double>(static_cast<double>(value));
}

/// @brief Integer millibars literal, e.g. `15_mbar`.
CASTLE_CONSTEXPR millibars<int64_t> operator""_mbar(unsigned long long value) CASTLE_NOEXCEPT
{
    return millibars<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating standard atmospheres literal, e.g. `1.5_atm`.
CASTLE_CONSTEXPR atmospheres<double> operator""_atm(long double value) CASTLE_NOEXCEPT
{
    return atmospheres<double>(static_cast<double>(value));
}

/// @brief Integer standard atmospheres literal, e.g. `15_atm`.
CASTLE_CONSTEXPR atmospheres<int64_t> operator""_atm(unsigned long long value) CASTLE_NOEXCEPT
{
    return atmospheres<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating pounds per square inch literal, e.g. `1.5_psi`.
CASTLE_CONSTEXPR pounds_per_square_inch<double> operator""_psi(long double value) CASTLE_NOEXCEPT
{
    return pounds_per_square_inch<double>(static_cast<double>(value));
}

/// @brief Integer pounds per square inch literal, e.g. `15_psi`.
CASTLE_CONSTEXPR pounds_per_square_inch<int64_t> operator""_psi(unsigned long long value) CASTLE_NOEXCEPT
{
    return pounds_per_square_inch<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_PRESSURE_HPP
