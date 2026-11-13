// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file distance.hpp
 * @brief Length units: SI multiples of the meter plus common imperial and nautical units.
 *
 * Example:
 * @code
 * #include "castle/units/distance.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::meters<double> total = 1.2_km + 350.0_m;  // 1550 m
 * castle::units::feet<double> altitude = 100.0_m;          // implicit, lossless
 * @endcode
 *
 * Key constraints:
 * - Imperial and nautical units use their exact international definitions
 *   (1 ft = 0.3048 m, 1 in = 0.0254 m, 1 mi = 1609.344 m, 1 NM = 1852 m).
 */
#ifndef CASTLE_UNITS_DISTANCE_HPP
#define CASTLE_UNITS_DISTANCE_HPP

#include "castle/core/compiler.hpp"
#include "castle/units/quantity.hpp"
#include "castle/units/scale.hpp"

#include <stdint.h>

namespace castle
{
namespace units
{

/// @brief Meter, the coherent length unit.
using meter_unit = unit<dim::length>;
/// @brief Kilometer.
using kilometer_unit = scaled_unit_t<meter_unit, kilo>;
/// @brief Centimeter.
using centimeter_unit = scaled_unit_t<meter_unit, centi>;
/// @brief Millimeter.
using millimeter_unit = scaled_unit_t<meter_unit, milli>;
/// @brief Micrometer.
using micrometer_unit = scaled_unit_t<meter_unit, micro>;
/// @brief Nanometer.
using nanometer_unit = scaled_unit_t<meter_unit, nano>;
/// @brief International inch, 0.0254 m.
using inch_unit = unit<dim::length, math::ratio<254, 10000>>;
/// @brief International foot, 0.3048 m.
using foot_unit = unit<dim::length, math::ratio<3048, 10000>>;
/// @brief International yard, 0.9144 m.
using yard_unit = unit<dim::length, math::ratio<9144, 10000>>;
/// @brief International statute mile, 1609.344 m.
using mile_unit = unit<dim::length, math::ratio<1609344, 1000>>;
/// @brief International nautical mile, 1852 m.
using nautical_mile_unit = unit<dim::length, math::ratio<1852>>;

/// @brief Length in meters.
template <typename Rep>
using meters = quantity<meter_unit, Rep>;
/// @brief Length in kilometers.
template <typename Rep>
using kilometers = quantity<kilometer_unit, Rep>;
/// @brief Length in centimeters.
template <typename Rep>
using centimeters = quantity<centimeter_unit, Rep>;
/// @brief Length in millimeters.
template <typename Rep>
using millimeters = quantity<millimeter_unit, Rep>;
/// @brief Length in micrometers.
template <typename Rep>
using micrometers = quantity<micrometer_unit, Rep>;
/// @brief Length in nanometers.
template <typename Rep>
using nanometers = quantity<nanometer_unit, Rep>;
/// @brief Length in inches.
template <typename Rep>
using inches = quantity<inch_unit, Rep>;
/// @brief Length in feet.
template <typename Rep>
using feet = quantity<foot_unit, Rep>;
/// @brief Length in yards.
template <typename Rep>
using yards = quantity<yard_unit, Rep>;
/// @brief Length in statute miles.
template <typename Rep>
using miles = quantity<mile_unit, Rep>;
/// @brief Length in nautical miles.
template <typename Rep>
using nautical_miles = quantity<nautical_mile_unit, Rep>;

namespace literals
{

/// @brief Floating meters literal, e.g. `1.5_m`.
CASTLE_CONSTEXPR meters<double> operator""_m(long double value) CASTLE_NOEXCEPT
{
    return meters<double>(static_cast<double>(value));
}

/// @brief Integer meters literal, e.g. `15_m`.
CASTLE_CONSTEXPR meters<int64_t> operator""_m(unsigned long long value) CASTLE_NOEXCEPT
{
    return meters<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating kilometers literal, e.g. `1.5_km`.
CASTLE_CONSTEXPR kilometers<double> operator""_km(long double value) CASTLE_NOEXCEPT
{
    return kilometers<double>(static_cast<double>(value));
}

/// @brief Integer kilometers literal, e.g. `15_km`.
CASTLE_CONSTEXPR kilometers<int64_t> operator""_km(unsigned long long value) CASTLE_NOEXCEPT
{
    return kilometers<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating centimeters literal, e.g. `1.5_cm`.
CASTLE_CONSTEXPR centimeters<double> operator""_cm(long double value) CASTLE_NOEXCEPT
{
    return centimeters<double>(static_cast<double>(value));
}

/// @brief Integer centimeters literal, e.g. `15_cm`.
CASTLE_CONSTEXPR centimeters<int64_t> operator""_cm(unsigned long long value) CASTLE_NOEXCEPT
{
    return centimeters<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating millimeters literal, e.g. `1.5_mm`.
CASTLE_CONSTEXPR millimeters<double> operator""_mm(long double value) CASTLE_NOEXCEPT
{
    return millimeters<double>(static_cast<double>(value));
}

/// @brief Integer millimeters literal, e.g. `15_mm`.
CASTLE_CONSTEXPR millimeters<int64_t> operator""_mm(unsigned long long value) CASTLE_NOEXCEPT
{
    return millimeters<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating micrometers literal, e.g. `1.5_um`.
CASTLE_CONSTEXPR micrometers<double> operator""_um(long double value) CASTLE_NOEXCEPT
{
    return micrometers<double>(static_cast<double>(value));
}

/// @brief Integer micrometers literal, e.g. `15_um`.
CASTLE_CONSTEXPR micrometers<int64_t> operator""_um(unsigned long long value) CASTLE_NOEXCEPT
{
    return micrometers<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating nanometers literal, e.g. `1.5_nm`.
CASTLE_CONSTEXPR nanometers<double> operator""_nm(long double value) CASTLE_NOEXCEPT
{
    return nanometers<double>(static_cast<double>(value));
}

/// @brief Integer nanometers literal, e.g. `15_nm`.
CASTLE_CONSTEXPR nanometers<int64_t> operator""_nm(unsigned long long value) CASTLE_NOEXCEPT
{
    return nanometers<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating inches literal, e.g. `1.5_in`.
CASTLE_CONSTEXPR inches<double> operator""_in(long double value) CASTLE_NOEXCEPT
{
    return inches<double>(static_cast<double>(value));
}

/// @brief Integer inches literal, e.g. `15_in`.
CASTLE_CONSTEXPR inches<int64_t> operator""_in(unsigned long long value) CASTLE_NOEXCEPT
{
    return inches<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating feet literal, e.g. `1.5_ft`.
CASTLE_CONSTEXPR feet<double> operator""_ft(long double value) CASTLE_NOEXCEPT
{
    return feet<double>(static_cast<double>(value));
}

/// @brief Integer feet literal, e.g. `15_ft`.
CASTLE_CONSTEXPR feet<int64_t> operator""_ft(unsigned long long value) CASTLE_NOEXCEPT
{
    return feet<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating yards literal, e.g. `1.5_yd`.
CASTLE_CONSTEXPR yards<double> operator""_yd(long double value) CASTLE_NOEXCEPT
{
    return yards<double>(static_cast<double>(value));
}

/// @brief Integer yards literal, e.g. `15_yd`.
CASTLE_CONSTEXPR yards<int64_t> operator""_yd(unsigned long long value) CASTLE_NOEXCEPT
{
    return yards<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating statute miles literal, e.g. `1.5_mi`.
CASTLE_CONSTEXPR miles<double> operator""_mi(long double value) CASTLE_NOEXCEPT
{
    return miles<double>(static_cast<double>(value));
}

/// @brief Integer statute miles literal, e.g. `15_mi`.
CASTLE_CONSTEXPR miles<int64_t> operator""_mi(unsigned long long value) CASTLE_NOEXCEPT
{
    return miles<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating nautical miles literal, e.g. `1.5_nmi`.
CASTLE_CONSTEXPR nautical_miles<double> operator""_nmi(long double value) CASTLE_NOEXCEPT
{
    return nautical_miles<double>(static_cast<double>(value));
}

/// @brief Integer nautical miles literal, e.g. `15_nmi`.
CASTLE_CONSTEXPR nautical_miles<int64_t> operator""_nmi(unsigned long long value) CASTLE_NOEXCEPT
{
    return nautical_miles<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_DISTANCE_HPP
