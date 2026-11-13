// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mass.hpp
 * @brief Mass units: the kilogram (coherent SI unit) with its metric multiples plus the avoirdupois pound and ounce.
 *
 * Example:
 * @code
 * #include "castle/units/mass.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::kilograms<double> payload = 1.5_kg + 250.0_g;  // 1.75 kg
 * castle::units::grams<int64_t> sample = 2_kg;                 // implicit, exact: 2000 g
 * @endcode
 *
 * Key constraints:
 * - The kilogram, not the gram, is the coherent unit: `kilogram_unit` has ratio 1
 *   and grams are `scaled_unit_t<kilogram_unit, milli>`.
 * - Pound and ounce use their exact international definitions
 *   (1 lb = 0.45359237 kg, 1 oz = 1/16 lb).
 */
#ifndef CASTLE_UNITS_MASS_HPP
#define CASTLE_UNITS_MASS_HPP

#include "castle/core/compiler.hpp"
#include "castle/units/quantity.hpp"
#include "castle/units/scale.hpp"

#include <stdint.h>

namespace castle
{
namespace units
{

/// @brief Kilogram, the coherent mass unit.
using kilogram_unit = unit<dim::mass>;
/// @brief Gram, 10^-3 kg.
using gram_unit = scaled_unit_t<kilogram_unit, milli>;
/// @brief Milligram, 10^-6 kg.
using milligram_unit = scaled_unit_t<kilogram_unit, micro>;
/// @brief Metric tonne, 1000 kg.
using tonne_unit = scaled_unit_t<kilogram_unit, kilo>;
/// @brief International avoirdupois pound, 0.45359237 kg.
using pound_unit = unit<dim::mass, math::ratio<45359237, 100000000>>;
/// @brief International avoirdupois ounce, 1/16 lb = 0.028349523125 kg.
using ounce_unit = unit<dim::mass, math::ratio<45359237, 1600000000>>;

/// @brief Mass in kilograms.
template <typename Rep>
using kilograms = quantity<kilogram_unit, Rep>;
/// @brief Mass in grams.
template <typename Rep>
using grams = quantity<gram_unit, Rep>;
/// @brief Mass in milligrams.
template <typename Rep>
using milligrams = quantity<milligram_unit, Rep>;
/// @brief Mass in tonnes.
template <typename Rep>
using tonnes = quantity<tonne_unit, Rep>;
/// @brief Mass in pounds.
template <typename Rep>
using pounds = quantity<pound_unit, Rep>;
/// @brief Mass in ounces.
template <typename Rep>
using ounces = quantity<ounce_unit, Rep>;

namespace literals
{

/// @brief Floating kilograms literal, e.g. `1.5_kg`.
CASTLE_CONSTEXPR kilograms<double> operator""_kg(long double value) CASTLE_NOEXCEPT
{
    return kilograms<double>(static_cast<double>(value));
}

/// @brief Integer kilograms literal, e.g. `15_kg`.
CASTLE_CONSTEXPR kilograms<int64_t> operator""_kg(unsigned long long value) CASTLE_NOEXCEPT
{
    return kilograms<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating grams literal, e.g. `1.5_g`.
CASTLE_CONSTEXPR grams<double> operator""_g(long double value) CASTLE_NOEXCEPT
{
    return grams<double>(static_cast<double>(value));
}

/// @brief Integer grams literal, e.g. `15_g`.
CASTLE_CONSTEXPR grams<int64_t> operator""_g(unsigned long long value) CASTLE_NOEXCEPT
{
    return grams<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating milligrams literal, e.g. `1.5_mg`.
CASTLE_CONSTEXPR milligrams<double> operator""_mg(long double value) CASTLE_NOEXCEPT
{
    return milligrams<double>(static_cast<double>(value));
}

/// @brief Integer milligrams literal, e.g. `15_mg`.
CASTLE_CONSTEXPR milligrams<int64_t> operator""_mg(unsigned long long value) CASTLE_NOEXCEPT
{
    return milligrams<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating tonnes literal, e.g. `1.5_t`.
CASTLE_CONSTEXPR tonnes<double> operator""_t(long double value) CASTLE_NOEXCEPT
{
    return tonnes<double>(static_cast<double>(value));
}

/// @brief Integer tonnes literal, e.g. `15_t`.
CASTLE_CONSTEXPR tonnes<int64_t> operator""_t(unsigned long long value) CASTLE_NOEXCEPT
{
    return tonnes<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating pounds literal, e.g. `1.5_lb`.
CASTLE_CONSTEXPR pounds<double> operator""_lb(long double value) CASTLE_NOEXCEPT
{
    return pounds<double>(static_cast<double>(value));
}

/// @brief Integer pounds literal, e.g. `15_lb`.
CASTLE_CONSTEXPR pounds<int64_t> operator""_lb(unsigned long long value) CASTLE_NOEXCEPT
{
    return pounds<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating ounces literal, e.g. `1.5_oz`.
CASTLE_CONSTEXPR ounces<double> operator""_oz(long double value) CASTLE_NOEXCEPT
{
    return ounces<double>(static_cast<double>(value));
}

/// @brief Integer ounces literal, e.g. `15_oz`.
CASTLE_CONSTEXPR ounces<int64_t> operator""_oz(unsigned long long value) CASTLE_NOEXCEPT
{
    return ounces<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_MASS_HPP
