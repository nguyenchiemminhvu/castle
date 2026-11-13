// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file angular_velocity.hpp
 * @brief Angular velocity units: radians, degrees and revolutions per unit time.
 *
 * Example:
 * @code
 * #include "castle/units/angular_velocity.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::degrees_per_second<double> rate = 60.0_rpm;    // implicit: 360 deg/s
 * castle::units::radians_per_second<double> omega = 180.0_degps; // implicit: pi rad/s
 * @endcode
 *
 * Key constraints:
 * - Angular velocity is angle per time (T^-1 * angle), deliberately distinct from
 *   frequency (T^-1), so `rpm` never silently becomes `Hz`.
 * - Degree and revolution based units carry an exact pi factor, like the angle
 *   units in `units/angle.hpp`; conversions to radians are evaluated once in
 *   floating point, while degree/revolution conversions stay exact.
 */
#ifndef CASTLE_UNITS_ANGULAR_VELOCITY_HPP
#define CASTLE_UNITS_ANGULAR_VELOCITY_HPP

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
using angular_velocity = dimension<0, 0, -1, 0, 0, 0, 0, 1>; ///< Angle / T.
} // namespace dim

/// @brief Radian per second, the coherent angular velocity unit.
using radian_per_second_unit = unit<dim::angular_velocity>;
/// @brief Milliradian per second.
using milliradian_per_second_unit = scaled_unit_t<radian_per_second_unit, milli>;
/// @brief Degree per second, pi / 180 rad/s.
using degree_per_second_unit = unit<dim::angular_velocity, math::ratio<1, 180>, 1>;
/// @brief Revolution per second, 2 pi rad/s.
using revolution_per_second_unit = unit<dim::angular_velocity, math::ratio<2>, 1>;
/// @brief Revolution per minute, pi / 30 rad/s.
using revolution_per_minute_unit = unit<dim::angular_velocity, math::ratio<1, 30>, 1>;

/// @brief Angular velocity in radians per second.
template <typename Rep>
using radians_per_second = quantity<radian_per_second_unit, Rep>;
/// @brief Angular velocity in milliradians per second.
template <typename Rep>
using milliradians_per_second = quantity<milliradian_per_second_unit, Rep>;
/// @brief Angular velocity in degrees per second.
template <typename Rep>
using degrees_per_second = quantity<degree_per_second_unit, Rep>;
/// @brief Angular velocity in revolutions per second.
template <typename Rep>
using revolutions_per_second = quantity<revolution_per_second_unit, Rep>;
/// @brief Angular velocity in revolutions per minute.
template <typename Rep>
using revolutions_per_minute = quantity<revolution_per_minute_unit, Rep>;

namespace literals
{

/// @brief Floating radians per second literal, e.g. `1.5_radps`.
CASTLE_CONSTEXPR radians_per_second<double> operator""_radps(long double value) CASTLE_NOEXCEPT
{
    return radians_per_second<double>(static_cast<double>(value));
}

/// @brief Integer radians per second literal, e.g. `15_radps`.
CASTLE_CONSTEXPR radians_per_second<int64_t> operator""_radps(unsigned long long value) CASTLE_NOEXCEPT
{
    return radians_per_second<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating milliradians per second literal, e.g. `1.5_mradps`.
CASTLE_CONSTEXPR milliradians_per_second<double> operator""_mradps(long double value) CASTLE_NOEXCEPT
{
    return milliradians_per_second<double>(static_cast<double>(value));
}

/// @brief Integer milliradians per second literal, e.g. `15_mradps`.
CASTLE_CONSTEXPR milliradians_per_second<int64_t> operator""_mradps(unsigned long long value) CASTLE_NOEXCEPT
{
    return milliradians_per_second<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating degrees per second literal, e.g. `1.5_degps`.
CASTLE_CONSTEXPR degrees_per_second<double> operator""_degps(long double value) CASTLE_NOEXCEPT
{
    return degrees_per_second<double>(static_cast<double>(value));
}

/// @brief Integer degrees per second literal, e.g. `15_degps`.
CASTLE_CONSTEXPR degrees_per_second<int64_t> operator""_degps(unsigned long long value) CASTLE_NOEXCEPT
{
    return degrees_per_second<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating revolutions per second literal, e.g. `1.5_rps`.
CASTLE_CONSTEXPR revolutions_per_second<double> operator""_rps(long double value) CASTLE_NOEXCEPT
{
    return revolutions_per_second<double>(static_cast<double>(value));
}

/// @brief Integer revolutions per second literal, e.g. `15_rps`.
CASTLE_CONSTEXPR revolutions_per_second<int64_t> operator""_rps(unsigned long long value) CASTLE_NOEXCEPT
{
    return revolutions_per_second<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating revolutions per minute literal, e.g. `1.5_rpm`.
CASTLE_CONSTEXPR revolutions_per_minute<double> operator""_rpm(long double value) CASTLE_NOEXCEPT
{
    return revolutions_per_minute<double>(static_cast<double>(value));
}

/// @brief Integer revolutions per minute literal, e.g. `15_rpm`.
CASTLE_CONSTEXPR revolutions_per_minute<int64_t> operator""_rpm(unsigned long long value) CASTLE_NOEXCEPT
{
    return revolutions_per_minute<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_ANGULAR_VELOCITY_HPP
