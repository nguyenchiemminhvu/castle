// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file force.hpp
 * @brief Force units: the newton with SI multiples plus the pound-force.
 *
 * Example:
 * @code
 * #include "castle/units/force.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::newtons<double> thrust = 1.2_kN + 300.0_N;  // 1500 N
 * castle::units::millinewtons<int64_t> fine = 2_N;          // implicit, exact: 2000 mN
 * @endcode
 *
 * Key constraints:
 * - `dim::force` is L M T^-2; the product of a mass and an acceleration in
 *   coherent units is exactly `newtons<Rep>`.
 * - The pound-force is exact: 1 lbf = 0.45359237 kg * 9.80665 m/s^2 = 4.4482216152605 N.
 */
#ifndef CASTLE_UNITS_FORCE_HPP
#define CASTLE_UNITS_FORCE_HPP

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
/// @brief Force, L M T^-2.
using force = dimension<1, 1, -2, 0, 0, 0, 0, 0>;
} // namespace dim

/// @brief Newton, the coherent force unit (kg*m/s^2).
using newton_unit = unit<dim::force>;
/// @brief Millinewton.
using millinewton_unit = scaled_unit_t<newton_unit, milli>;
/// @brief Kilonewton.
using kilonewton_unit = scaled_unit_t<newton_unit, kilo>;
/// @brief Pound-force, exactly 4.4482216152605 N.
using pound_force_unit = unit<dim::force, math::ratio<44482216152605, 10000000000000>>;

/// @brief Force in newtons.
template <typename Rep>
using newtons = quantity<newton_unit, Rep>;
/// @brief Force in millinewtons.
template <typename Rep>
using millinewtons = quantity<millinewton_unit, Rep>;
/// @brief Force in kilonewtons.
template <typename Rep>
using kilonewtons = quantity<kilonewton_unit, Rep>;
/// @brief Force in pounds force.
template <typename Rep>
using pounds_force = quantity<pound_force_unit, Rep>;

namespace literals
{

/// @brief Floating newtons literal, e.g. `1.5_N`.
CASTLE_CONSTEXPR newtons<double> operator""_N(long double value) CASTLE_NOEXCEPT
{
    return newtons<double>(static_cast<double>(value));
}

/// @brief Integer newtons literal, e.g. `15_N`.
CASTLE_CONSTEXPR newtons<int64_t> operator""_N(unsigned long long value) CASTLE_NOEXCEPT
{
    return newtons<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating millinewtons literal, e.g. `1.5_mN`.
CASTLE_CONSTEXPR millinewtons<double> operator""_mN(long double value) CASTLE_NOEXCEPT
{
    return millinewtons<double>(static_cast<double>(value));
}

/// @brief Integer millinewtons literal, e.g. `15_mN`.
CASTLE_CONSTEXPR millinewtons<int64_t> operator""_mN(unsigned long long value) CASTLE_NOEXCEPT
{
    return millinewtons<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating kilonewtons literal, e.g. `1.5_kN`.
CASTLE_CONSTEXPR kilonewtons<double> operator""_kN(long double value) CASTLE_NOEXCEPT
{
    return kilonewtons<double>(static_cast<double>(value));
}

/// @brief Integer kilonewtons literal, e.g. `15_kN`.
CASTLE_CONSTEXPR kilonewtons<int64_t> operator""_kN(unsigned long long value) CASTLE_NOEXCEPT
{
    return kilonewtons<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating pounds force literal, e.g. `1.5_lbf`.
CASTLE_CONSTEXPR pounds_force<double> operator""_lbf(long double value) CASTLE_NOEXCEPT
{
    return pounds_force<double>(static_cast<double>(value));
}

/// @brief Integer pounds force literal, e.g. `15_lbf`.
CASTLE_CONSTEXPR pounds_force<int64_t> operator""_lbf(unsigned long long value) CASTLE_NOEXCEPT
{
    return pounds_force<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_FORCE_HPP
