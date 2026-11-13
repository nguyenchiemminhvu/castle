// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file current.hpp
 * @brief Electric current units: SI multiples of the ampere.
 *
 * Example:
 * @code
 * #include "castle/units/current.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::milliamperes<int32_t> load = 1_A + 250_mA;      // 1250 mA
 * castle::units::amperes<double> peak = 1500.0_mA;               // implicit, lossless
 * @endcode
 *
 * Key constraints:
 * - Current is the SI base dimension `dim::current` declared by quantity.hpp;
 *   this header only adds units, aliases and literals.
 */
#ifndef CASTLE_UNITS_CURRENT_HPP
#define CASTLE_UNITS_CURRENT_HPP

#include "castle/core/compiler.hpp"
#include "castle/units/quantity.hpp"
#include "castle/units/scale.hpp"

#include <stdint.h>

namespace castle
{
namespace units
{

/// @brief Ampere, the coherent current unit.
using ampere_unit = unit<dim::current>;
/// @brief Kiloampere.
using kiloampere_unit = scaled_unit_t<ampere_unit, kilo>;
/// @brief Milliampere.
using milliampere_unit = scaled_unit_t<ampere_unit, milli>;
/// @brief Microampere.
using microampere_unit = scaled_unit_t<ampere_unit, micro>;

/// @brief Current in amperes.
template <typename Rep>
using amperes = quantity<ampere_unit, Rep>;
/// @brief Current in kiloamperes.
template <typename Rep>
using kiloamperes = quantity<kiloampere_unit, Rep>;
/// @brief Current in milliamperes.
template <typename Rep>
using milliamperes = quantity<milliampere_unit, Rep>;
/// @brief Current in microamperes.
template <typename Rep>
using microamperes = quantity<microampere_unit, Rep>;

namespace literals
{

/// @brief Floating amperes literal, e.g. `1.5_A`.
CASTLE_CONSTEXPR amperes<double> operator""_A(long double value) CASTLE_NOEXCEPT
{
    return amperes<double>(static_cast<double>(value));
}

/// @brief Integer amperes literal, e.g. `15_A`.
CASTLE_CONSTEXPR amperes<int64_t> operator""_A(unsigned long long value) CASTLE_NOEXCEPT
{
    return amperes<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating kiloamperes literal, e.g. `1.5_kA`.
CASTLE_CONSTEXPR kiloamperes<double> operator""_kA(long double value) CASTLE_NOEXCEPT
{
    return kiloamperes<double>(static_cast<double>(value));
}

/// @brief Integer kiloamperes literal, e.g. `15_kA`.
CASTLE_CONSTEXPR kiloamperes<int64_t> operator""_kA(unsigned long long value) CASTLE_NOEXCEPT
{
    return kiloamperes<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating milliamperes literal, e.g. `1.5_mA`.
CASTLE_CONSTEXPR milliamperes<double> operator""_mA(long double value) CASTLE_NOEXCEPT
{
    return milliamperes<double>(static_cast<double>(value));
}

/// @brief Integer milliamperes literal, e.g. `15_mA`.
CASTLE_CONSTEXPR milliamperes<int64_t> operator""_mA(unsigned long long value) CASTLE_NOEXCEPT
{
    return milliamperes<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating microamperes literal, e.g. `1.5_uA`.
CASTLE_CONSTEXPR microamperes<double> operator""_uA(long double value) CASTLE_NOEXCEPT
{
    return microamperes<double>(static_cast<double>(value));
}

/// @brief Integer microamperes literal, e.g. `15_uA`.
CASTLE_CONSTEXPR microamperes<int64_t> operator""_uA(unsigned long long value) CASTLE_NOEXCEPT
{
    return microamperes<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_CURRENT_HPP
