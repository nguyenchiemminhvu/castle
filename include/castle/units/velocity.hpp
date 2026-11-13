// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file velocity.hpp
 * @brief Speed units: meters per second plus kilometers per hour, miles per hour, knots and feet per second.
 *
 * Example:
 * @code
 * #include "castle/units/velocity.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::meters_per_second<double> limit = 90.0_kmph;   // 25 m/s
 * castle::units::kilometers_per_hour<double> ground = 10.0_kn;  // 18.52 km/h
 * @endcode
 *
 * Key constraints:
 * - Miles per hour, knots and feet per second use exact international
 *   definitions (1 mi = 1609.344 m, 1 NM = 1852 m, 1 ft = 0.3048 m).
 * - Dividing a length by a duration yields `meters_per_second` in the coherent scale.
 */
#ifndef CASTLE_UNITS_VELOCITY_HPP
#define CASTLE_UNITS_VELOCITY_HPP

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
using velocity = dimension<1, 0, -1, 0, 0, 0, 0, 0>; ///< L / T.
} // namespace dim

/// @brief Meter per second, the coherent velocity unit.
using meter_per_second_unit = unit<dim::velocity>;
/// @brief Kilometer per hour, 5/18 m/s.
using kilometer_per_hour_unit = unit<dim::velocity, math::ratio<5, 18>>;
/// @brief International mile per hour, 0.44704 m/s.
using mile_per_hour_unit = unit<dim::velocity, math::ratio<1609344, 3600000>>;
/// @brief Knot, one nautical mile per hour.
using knot_unit = unit<dim::velocity, math::ratio<1852, 3600>>;
/// @brief International foot per second, 0.3048 m/s.
using foot_per_second_unit = unit<dim::velocity, math::ratio<3048, 10000>>;
/// @brief Centimeter per second.
using centimeter_per_second_unit = scaled_unit_t<meter_per_second_unit, centi>;
/// @brief Millimeter per second.
using millimeter_per_second_unit = scaled_unit_t<meter_per_second_unit, milli>;

/// @brief Velocity in meters per second.
template <typename Rep>
using meters_per_second = quantity<meter_per_second_unit, Rep>;
/// @brief Velocity in kilometers per hour.
template <typename Rep>
using kilometers_per_hour = quantity<kilometer_per_hour_unit, Rep>;
/// @brief Velocity in miles per hour.
template <typename Rep>
using miles_per_hour = quantity<mile_per_hour_unit, Rep>;
/// @brief Velocity in knots.
template <typename Rep>
using knots = quantity<knot_unit, Rep>;
/// @brief Velocity in feet per second.
template <typename Rep>
using feet_per_second = quantity<foot_per_second_unit, Rep>;
/// @brief Velocity in centimeters per second.
template <typename Rep>
using centimeters_per_second = quantity<centimeter_per_second_unit, Rep>;
/// @brief Velocity in millimeters per second.
template <typename Rep>
using millimeters_per_second = quantity<millimeter_per_second_unit, Rep>;

namespace literals
{

/// @brief Floating meters per second literal, e.g. `1.5_mps`.
CASTLE_CONSTEXPR meters_per_second<double> operator""_mps(long double value) CASTLE_NOEXCEPT
{
    return meters_per_second<double>(static_cast<double>(value));
}

/// @brief Integer meters per second literal, e.g. `15_mps`.
CASTLE_CONSTEXPR meters_per_second<int64_t> operator""_mps(unsigned long long value) CASTLE_NOEXCEPT
{
    return meters_per_second<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating kilometers per hour literal, e.g. `1.5_kmph`.
CASTLE_CONSTEXPR kilometers_per_hour<double> operator""_kmph(long double value) CASTLE_NOEXCEPT
{
    return kilometers_per_hour<double>(static_cast<double>(value));
}

/// @brief Integer kilometers per hour literal, e.g. `15_kmph`.
CASTLE_CONSTEXPR kilometers_per_hour<int64_t> operator""_kmph(unsigned long long value) CASTLE_NOEXCEPT
{
    return kilometers_per_hour<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating miles per hour literal, e.g. `1.5_mph`.
CASTLE_CONSTEXPR miles_per_hour<double> operator""_mph(long double value) CASTLE_NOEXCEPT
{
    return miles_per_hour<double>(static_cast<double>(value));
}

/// @brief Integer miles per hour literal, e.g. `15_mph`.
CASTLE_CONSTEXPR miles_per_hour<int64_t> operator""_mph(unsigned long long value) CASTLE_NOEXCEPT
{
    return miles_per_hour<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating knots literal, e.g. `1.5_kn`.
CASTLE_CONSTEXPR knots<double> operator""_kn(long double value) CASTLE_NOEXCEPT
{
    return knots<double>(static_cast<double>(value));
}

/// @brief Integer knots literal, e.g. `15_kn`.
CASTLE_CONSTEXPR knots<int64_t> operator""_kn(unsigned long long value) CASTLE_NOEXCEPT
{
    return knots<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating centimeters per second literal, e.g. `1.5_cmps`.
CASTLE_CONSTEXPR centimeters_per_second<double> operator""_cmps(long double value) CASTLE_NOEXCEPT
{
    return centimeters_per_second<double>(static_cast<double>(value));
}

/// @brief Integer centimeters per second literal, e.g. `15_cmps`.
CASTLE_CONSTEXPR centimeters_per_second<int64_t> operator""_cmps(unsigned long long value) CASTLE_NOEXCEPT
{
    return centimeters_per_second<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating millimeters per second literal, e.g. `1.5_mmps`.
CASTLE_CONSTEXPR millimeters_per_second<double> operator""_mmps(long double value) CASTLE_NOEXCEPT
{
    return millimeters_per_second<double>(static_cast<double>(value));
}

/// @brief Integer millimeters per second literal, e.g. `15_mmps`.
CASTLE_CONSTEXPR millimeters_per_second<int64_t> operator""_mmps(unsigned long long value) CASTLE_NOEXCEPT
{
    return millimeters_per_second<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_VELOCITY_HPP
