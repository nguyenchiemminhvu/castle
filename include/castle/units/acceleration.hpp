// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file acceleration.hpp
 * @brief Acceleration units: meters per second squared, standard gravity and feet per second squared.
 *
 * Example:
 * @code
 * #include "castle/units/acceleration.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::meters_per_second_squared<double> a = 2.0_gn;         // 19.6133 m/s^2
 * castle::units::milli_standard_gravities<int32_t> raw = 1000_mgn;     // 1 g
 * @endcode
 *
 * Key constraints:
 * - Standard gravity is exactly 9.80665 m/s^2 (ISO 80000-3).
 * - Dividing a velocity by a duration yields `meters_per_second_squared` in the coherent scale.
 */
#ifndef CASTLE_UNITS_ACCELERATION_HPP
#define CASTLE_UNITS_ACCELERATION_HPP

#include "castle/core/compiler.hpp"
#include "castle/units/quantity.hpp"

#include <stdint.h>

namespace castle
{
namespace units
{
namespace dim
{
using acceleration = dimension<1, 0, -2, 0, 0, 0, 0, 0>; ///< L / T^2.
} // namespace dim

/// @brief Meter per second squared, the coherent acceleration unit.
using meter_per_second_squared_unit = unit<dim::acceleration>;
/// @brief Standard gravity, 9.80665 m/s^2.
using standard_gravity_unit = unit<dim::acceleration, math::ratio<980665, 100000>>;
/// @brief Milli standard gravity, 0.00980665 m/s^2.
using milli_standard_gravity_unit = unit<dim::acceleration, math::ratio<980665, 100000000>>;
/// @brief International foot per second squared, 0.3048 m/s^2.
using foot_per_second_squared_unit = unit<dim::acceleration, math::ratio<3048, 10000>>;

/// @brief Acceleration in meters per second squared.
template <typename Rep>
using meters_per_second_squared = quantity<meter_per_second_squared_unit, Rep>;
/// @brief Acceleration in standard gravities.
template <typename Rep>
using standard_gravities = quantity<standard_gravity_unit, Rep>;
/// @brief Acceleration in milli standard gravities.
template <typename Rep>
using milli_standard_gravities = quantity<milli_standard_gravity_unit, Rep>;
/// @brief Acceleration in feet per second squared.
template <typename Rep>
using feet_per_second_squared = quantity<foot_per_second_squared_unit, Rep>;

namespace literals
{

/// @brief Floating meters per second squared literal, e.g. `1.5_mps2`.
CASTLE_CONSTEXPR meters_per_second_squared<double> operator""_mps2(long double value) CASTLE_NOEXCEPT
{
    return meters_per_second_squared<double>(static_cast<double>(value));
}

/// @brief Integer meters per second squared literal, e.g. `15_mps2`.
CASTLE_CONSTEXPR meters_per_second_squared<int64_t> operator""_mps2(unsigned long long value) CASTLE_NOEXCEPT
{
    return meters_per_second_squared<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating standard gravities literal, e.g. `1.5_gn`.
CASTLE_CONSTEXPR standard_gravities<double> operator""_gn(long double value) CASTLE_NOEXCEPT
{
    return standard_gravities<double>(static_cast<double>(value));
}

/// @brief Integer standard gravities literal, e.g. `15_gn`.
CASTLE_CONSTEXPR standard_gravities<int64_t> operator""_gn(unsigned long long value) CASTLE_NOEXCEPT
{
    return standard_gravities<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating milli standard gravities literal, e.g. `1.5_mgn`.
CASTLE_CONSTEXPR milli_standard_gravities<double> operator""_mgn(long double value) CASTLE_NOEXCEPT
{
    return milli_standard_gravities<double>(static_cast<double>(value));
}

/// @brief Integer milli standard gravities literal, e.g. `15_mgn`.
CASTLE_CONSTEXPR milli_standard_gravities<int64_t> operator""_mgn(unsigned long long value) CASTLE_NOEXCEPT
{
    return milli_standard_gravities<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_ACCELERATION_HPP
