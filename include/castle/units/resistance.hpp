// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file resistance.hpp
 * @brief Electric resistance units: SI multiples of the ohm.
 *
 * Example:
 * @code
 * #include "castle/units/resistance.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::ohms<int64_t> divider = 4_kOhm + 700_Ohm;  // 4700 Ohm
 * castle::units::kiloohms<double> pullup = 10000.0_Ohm;     // implicit, lossless
 * @endcode
 *
 * Key constraints:
 * - `dim::resistance` is M L^2 T^-3 I^-2, so Ohm's law (`V = R * I`) works with
 *   voltage.hpp and current.hpp without any helper function.
 */
#ifndef CASTLE_UNITS_RESISTANCE_HPP
#define CASTLE_UNITS_RESISTANCE_HPP

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
using resistance = dimension<2, 1, -3, -2, 0, 0, 0, 0>; ///< Electric resistance, M L^2 T^-3 I^-2.
} // namespace dim

/// @brief Ohm, the coherent resistance unit.
using ohm_unit = unit<dim::resistance>;
/// @brief Milliohm.
using milliohm_unit = scaled_unit_t<ohm_unit, milli>;
/// @brief Kiloohm.
using kiloohm_unit = scaled_unit_t<ohm_unit, kilo>;
/// @brief Megaohm.
using megaohm_unit = scaled_unit_t<ohm_unit, mega>;

/// @brief Resistance in ohms.
template <typename Rep>
using ohms = quantity<ohm_unit, Rep>;
/// @brief Resistance in milliohms.
template <typename Rep>
using milliohms = quantity<milliohm_unit, Rep>;
/// @brief Resistance in kiloohms.
template <typename Rep>
using kiloohms = quantity<kiloohm_unit, Rep>;
/// @brief Resistance in megaohms.
template <typename Rep>
using megaohms = quantity<megaohm_unit, Rep>;

namespace literals
{

/// @brief Floating ohms literal, e.g. `1.5_Ohm`.
CASTLE_CONSTEXPR ohms<double> operator""_Ohm(long double value) CASTLE_NOEXCEPT
{
    return ohms<double>(static_cast<double>(value));
}

/// @brief Integer ohms literal, e.g. `15_Ohm`.
CASTLE_CONSTEXPR ohms<int64_t> operator""_Ohm(unsigned long long value) CASTLE_NOEXCEPT
{
    return ohms<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating milliohms literal, e.g. `1.5_mOhm`.
CASTLE_CONSTEXPR milliohms<double> operator""_mOhm(long double value) CASTLE_NOEXCEPT
{
    return milliohms<double>(static_cast<double>(value));
}

/// @brief Integer milliohms literal, e.g. `15_mOhm`.
CASTLE_CONSTEXPR milliohms<int64_t> operator""_mOhm(unsigned long long value) CASTLE_NOEXCEPT
{
    return milliohms<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating kiloohms literal, e.g. `1.5_kOhm`.
CASTLE_CONSTEXPR kiloohms<double> operator""_kOhm(long double value) CASTLE_NOEXCEPT
{
    return kiloohms<double>(static_cast<double>(value));
}

/// @brief Integer kiloohms literal, e.g. `15_kOhm`.
CASTLE_CONSTEXPR kiloohms<int64_t> operator""_kOhm(unsigned long long value) CASTLE_NOEXCEPT
{
    return kiloohms<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating megaohms literal, e.g. `1.5_MOhm`.
CASTLE_CONSTEXPR megaohms<double> operator""_MOhm(long double value) CASTLE_NOEXCEPT
{
    return megaohms<double>(static_cast<double>(value));
}

/// @brief Integer megaohms literal, e.g. `15_MOhm`.
CASTLE_CONSTEXPR megaohms<int64_t> operator""_MOhm(unsigned long long value) CASTLE_NOEXCEPT
{
    return megaohms<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_RESISTANCE_HPP
