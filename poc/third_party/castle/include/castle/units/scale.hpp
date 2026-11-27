// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file scale.hpp
 * @brief Official SI prefix and time-scale ratios for Castle units and chrono.
 *
 * Every scale is a reduced `castle::math::ratio` that expresses one unit in
 * terms of its coherent SI unit. SI prefixes (`milli`, `kilo`, ...) scale any
 * unit, and the time scales (`second`, `minute`, ...) are the periods used by
 * `castle::chrono::duration` and the `castle::units` time aliases.
 *
 * Example:
 * @code
 * #include "castle/units/scale.hpp"
 *
 * static_assert(castle::units::kilo::num == 1000, "kilo");
 * static_assert(castle::units::minute::num == 60, "minute");
 * @endcode
 *
 * @note Time scales use singular names; the plural names (`seconds`,
 *       `minutes`, ...) are the quantity alias templates in `units/time.hpp`.
 */
#ifndef CASTLE_UNITS_SCALE_HPP
#define CASTLE_UNITS_SCALE_HPP

#include "castle/math/ratio.hpp"

namespace castle
{
namespace units
{

/// @brief SI prefix 10^-18.
using atto  = math::ratio<1LL, 1000000000000000000LL>;
/// @brief SI prefix 10^-15.
using femto = math::ratio<1LL, 1000000000000000LL>;
/// @brief SI prefix 10^-12.
using pico  = math::ratio<1LL, 1000000000000LL>;
/// @brief SI prefix 10^-9.
using nano  = math::ratio<1LL, 1000000000LL>;
/// @brief SI prefix 10^-6.
using micro = math::ratio<1LL, 1000000LL>;
/// @brief SI prefix 10^-3.
using milli = math::ratio<1LL, 1000LL>;
/// @brief SI prefix 10^-2.
using centi = math::ratio<1LL, 100LL>;
/// @brief SI prefix 10^-1.
using deci  = math::ratio<1LL, 10LL>;

/// @brief SI prefix 10^1.
using deca  = math::ratio<10LL, 1LL>;
/// @brief SI prefix 10^2.
using hecto = math::ratio<100LL, 1LL>;
/// @brief SI prefix 10^3.
using kilo  = math::ratio<1000LL, 1LL>;
/// @brief SI prefix 10^6.
using mega  = math::ratio<1000000LL, 1LL>;
/// @brief SI prefix 10^9.
using giga  = math::ratio<1000000000LL, 1LL>;

/// @brief One second, the coherent SI time unit.
using second = math::ratio<1LL, 1LL>;
/// @brief One minute in seconds.
using minute = math::ratio<60LL, 1LL>;
/// @brief One hour in seconds.
using hour   = math::ratio<3600LL, 1LL>;
/// @brief One day in seconds.
using day    = math::ratio<86400LL, 1LL>;
/// @brief One week in seconds.
using week   = math::ratio<604800LL, 1LL>;

} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_SCALE_HPP
