// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file units.hpp
 * @brief Umbrella include for Castle's type-safe physical units.
 *
 * Include this header when a translation unit needs many quantities at once.
 * It adds no semantics of its own; every unit, operator and literal comes from
 * the included headers. All literals live in `castle::units::literals`.
 *
 * Example:
 * @code
 * #include "castle/units/units.hpp"
 *
 * using namespace castle::units::literals;
 *
 * void set_velocity(castle::units::meters_per_second<double> speed);
 *
 * set_velocity(36.0_kmph);       // converted to 10 m/s
 * set_velocity(100.0_m / 4_s);   // derived: 25 m/s
 * // set_velocity(5.0_deg);      // does not compile: angle is not a velocity
 * @endcode
 *
 * @note Prefer the individual headers when compile-time dependencies must stay small.
 */
#ifndef CASTLE_UNITS_UNITS_HPP
#define CASTLE_UNITS_UNITS_HPP

#include "castle/units/scale.hpp"
#include "castle/units/quantity.hpp"
#include "castle/units/time.hpp"
#include "castle/units/angle.hpp"
#include "castle/units/geo.hpp"
#include "castle/units/distance.hpp"
#include "castle/units/velocity.hpp"
#include "castle/units/acceleration.hpp"
#include "castle/units/angular_velocity.hpp"
#include "castle/units/frequency.hpp"
#include "castle/units/temperature.hpp"
#include "castle/units/pressure.hpp"
#include "castle/units/mass.hpp"
#include "castle/units/force.hpp"
#include "castle/units/energy.hpp"
#include "castle/units/power.hpp"
#include "castle/units/voltage.hpp"
#include "castle/units/current.hpp"
#include "castle/units/resistance.hpp"
#include "castle/units/charge.hpp"
#include "castle/units/gain.hpp"

#endif // CASTLE_UNITS_UNITS_HPP
