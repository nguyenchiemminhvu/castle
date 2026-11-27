// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file energy.hpp
 * @brief Energy units: the joule with SI multiples plus watt-hour based units.
 *
 * Example:
 * @code
 * #include "castle/units/energy.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::joules<double> e = 1.5_kJ + 500.0_J;           // 2000 J
 * castle::units::milliwatt_hours<int64_t> pack = 3_Wh;          // implicit, exact: 3000 mWh
 * @endcode
 *
 * Key constraints:
 * - `dim::energy` is L^2 M T^-2; force times length in coherent units is exactly `joules<Rep>`.
 * - Watt-hour units are exact: 1 mWh = 3.6 J, 1 Wh = 3600 J, 1 kWh = 3.6 MJ.
 */
#ifndef CASTLE_UNITS_ENERGY_HPP
#define CASTLE_UNITS_ENERGY_HPP

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
/// @brief Energy, L^2 M T^-2.
using energy = dimension<2, 1, -2, 0, 0, 0, 0, 0>;
} // namespace dim

/// @brief Joule, the coherent energy unit (N*m).
using joule_unit = unit<dim::energy>;
/// @brief Millijoule.
using millijoule_unit = scaled_unit_t<joule_unit, milli>;
/// @brief Kilojoule.
using kilojoule_unit = scaled_unit_t<joule_unit, kilo>;
/// @brief Megajoule.
using megajoule_unit = scaled_unit_t<joule_unit, mega>;
/// @brief Milliwatt-hour, 3.6 J.
using milliwatt_hour_unit = unit<dim::energy, math::ratio<18, 5>>;
/// @brief Watt-hour, 3600 J.
using watt_hour_unit = unit<dim::energy, math::ratio<3600>>;
/// @brief Kilowatt-hour, 3.6 MJ.
using kilowatt_hour_unit = unit<dim::energy, math::ratio<3600000>>;

/// @brief Energy in joules.
template <typename Rep>
using joules = quantity<joule_unit, Rep>;
/// @brief Energy in millijoules.
template <typename Rep>
using millijoules = quantity<millijoule_unit, Rep>;
/// @brief Energy in kilojoules.
template <typename Rep>
using kilojoules = quantity<kilojoule_unit, Rep>;
/// @brief Energy in megajoules.
template <typename Rep>
using megajoules = quantity<megajoule_unit, Rep>;
/// @brief Energy in milliwatt hours.
template <typename Rep>
using milliwatt_hours = quantity<milliwatt_hour_unit, Rep>;
/// @brief Energy in watt hours.
template <typename Rep>
using watt_hours = quantity<watt_hour_unit, Rep>;
/// @brief Energy in kilowatt hours.
template <typename Rep>
using kilowatt_hours = quantity<kilowatt_hour_unit, Rep>;

namespace literals
{

/// @brief Floating joules literal, e.g. `1.5_J`.
CASTLE_CONSTEXPR joules<double> operator""_J(long double value) CASTLE_NOEXCEPT
{
    return joules<double>(static_cast<double>(value));
}

/// @brief Integer joules literal, e.g. `15_J`.
CASTLE_CONSTEXPR joules<int64_t> operator""_J(unsigned long long value) CASTLE_NOEXCEPT
{
    return joules<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating millijoules literal, e.g. `1.5_mJ`.
CASTLE_CONSTEXPR millijoules<double> operator""_mJ(long double value) CASTLE_NOEXCEPT
{
    return millijoules<double>(static_cast<double>(value));
}

/// @brief Integer millijoules literal, e.g. `15_mJ`.
CASTLE_CONSTEXPR millijoules<int64_t> operator""_mJ(unsigned long long value) CASTLE_NOEXCEPT
{
    return millijoules<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating kilojoules literal, e.g. `1.5_kJ`.
CASTLE_CONSTEXPR kilojoules<double> operator""_kJ(long double value) CASTLE_NOEXCEPT
{
    return kilojoules<double>(static_cast<double>(value));
}

/// @brief Integer kilojoules literal, e.g. `15_kJ`.
CASTLE_CONSTEXPR kilojoules<int64_t> operator""_kJ(unsigned long long value) CASTLE_NOEXCEPT
{
    return kilojoules<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating megajoules literal, e.g. `1.5_MJ`.
CASTLE_CONSTEXPR megajoules<double> operator""_MJ(long double value) CASTLE_NOEXCEPT
{
    return megajoules<double>(static_cast<double>(value));
}

/// @brief Integer megajoules literal, e.g. `15_MJ`.
CASTLE_CONSTEXPR megajoules<int64_t> operator""_MJ(unsigned long long value) CASTLE_NOEXCEPT
{
    return megajoules<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating milliwatt hours literal, e.g. `1.5_mWh`.
CASTLE_CONSTEXPR milliwatt_hours<double> operator""_mWh(long double value) CASTLE_NOEXCEPT
{
    return milliwatt_hours<double>(static_cast<double>(value));
}

/// @brief Integer milliwatt hours literal, e.g. `15_mWh`.
CASTLE_CONSTEXPR milliwatt_hours<int64_t> operator""_mWh(unsigned long long value) CASTLE_NOEXCEPT
{
    return milliwatt_hours<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating watt hours literal, e.g. `1.5_Wh`.
CASTLE_CONSTEXPR watt_hours<double> operator""_Wh(long double value) CASTLE_NOEXCEPT
{
    return watt_hours<double>(static_cast<double>(value));
}

/// @brief Integer watt hours literal, e.g. `15_Wh`.
CASTLE_CONSTEXPR watt_hours<int64_t> operator""_Wh(unsigned long long value) CASTLE_NOEXCEPT
{
    return watt_hours<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating kilowatt hours literal, e.g. `1.5_kWh`.
CASTLE_CONSTEXPR kilowatt_hours<double> operator""_kWh(long double value) CASTLE_NOEXCEPT
{
    return kilowatt_hours<double>(static_cast<double>(value));
}

/// @brief Integer kilowatt hours literal, e.g. `15_kWh`.
CASTLE_CONSTEXPR kilowatt_hours<int64_t> operator""_kWh(unsigned long long value) CASTLE_NOEXCEPT
{
    return kilowatt_hours<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_ENERGY_HPP
