// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file charge.hpp
 * @brief Electric charge units: the coulomb, the millicoulomb and battery-style ampere-hours.
 *
 * Example:
 * @code
 * #include "castle/units/charge.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::milliampere_hours<double> drawn = 0.5_Ah + 250.0_mAh;  // 750 mAh
 * castle::units::coulombs<double> q = 1.0_mAh;                         // implicit, lossless: 3.6 C
 * @endcode
 *
 * Key constraints:
 * - Ampere-hours use their exact definitions (1 Ah = 3600 C, 1 mAh = 3.6 C).
 * - Current times a duration gives coulombs (in the combined scale); charge
 *   divided by a duration gives a current quantity.
 */
#ifndef CASTLE_UNITS_CHARGE_HPP
#define CASTLE_UNITS_CHARGE_HPP

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

/// @brief Electric charge, T I.
using charge = dimension<0, 0, 1, 1, 0, 0, 0, 0>;

} // namespace dim

/// @brief Coulomb, the coherent charge unit (1 A s).
using coulomb_unit = unit<dim::charge>;
/// @brief Millicoulomb.
using millicoulomb_unit = scaled_unit_t<coulomb_unit, milli>;
/// @brief Ampere-hour, 3600 C.
using ampere_hour_unit = unit<dim::charge, math::ratio<3600>>;
/// @brief Milliampere-hour, 3.6 C.
using milliampere_hour_unit = unit<dim::charge, math::ratio<18, 5>>;

/// @brief Charge in coulombs.
template <typename Rep>
using coulombs = quantity<coulomb_unit, Rep>;
/// @brief Charge in millicoulombs.
template <typename Rep>
using millicoulombs = quantity<millicoulomb_unit, Rep>;
/// @brief Charge in ampere-hours.
template <typename Rep>
using ampere_hours = quantity<ampere_hour_unit, Rep>;
/// @brief Charge in milliampere-hours.
template <typename Rep>
using milliampere_hours = quantity<milliampere_hour_unit, Rep>;

namespace literals
{

/// @brief Floating coulombs literal, e.g. `1.5_C`.
CASTLE_CONSTEXPR coulombs<double> operator""_C(long double value) CASTLE_NOEXCEPT
{
    return coulombs<double>(static_cast<double>(value));
}

/// @brief Integer coulombs literal, e.g. `15_C`.
CASTLE_CONSTEXPR coulombs<int64_t> operator""_C(unsigned long long value) CASTLE_NOEXCEPT
{
    return coulombs<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating millicoulombs literal, e.g. `1.5_mC`.
CASTLE_CONSTEXPR millicoulombs<double> operator""_mC(long double value) CASTLE_NOEXCEPT
{
    return millicoulombs<double>(static_cast<double>(value));
}

/// @brief Integer millicoulombs literal, e.g. `15_mC`.
CASTLE_CONSTEXPR millicoulombs<int64_t> operator""_mC(unsigned long long value) CASTLE_NOEXCEPT
{
    return millicoulombs<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating ampere-hours literal, e.g. `1.5_Ah`.
CASTLE_CONSTEXPR ampere_hours<double> operator""_Ah(long double value) CASTLE_NOEXCEPT
{
    return ampere_hours<double>(static_cast<double>(value));
}

/// @brief Integer ampere-hours literal, e.g. `15_Ah`.
CASTLE_CONSTEXPR ampere_hours<int64_t> operator""_Ah(unsigned long long value) CASTLE_NOEXCEPT
{
    return ampere_hours<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating milliampere-hours literal, e.g. `1.5_mAh`.
CASTLE_CONSTEXPR milliampere_hours<double> operator""_mAh(long double value) CASTLE_NOEXCEPT
{
    return milliampere_hours<double>(static_cast<double>(value));
}

/// @brief Integer milliampere-hours literal, e.g. `15_mAh`.
CASTLE_CONSTEXPR milliampere_hours<int64_t> operator""_mAh(unsigned long long value) CASTLE_NOEXCEPT
{
    return milliampere_hours<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_CHARGE_HPP
