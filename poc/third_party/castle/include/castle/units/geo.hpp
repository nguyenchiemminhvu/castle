// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file geo.hpp
 * @brief Geographic coordinate angles (latitude/longitude) kept apart from plane angles by a kind tag.
 *
 * Example:
 * @code
 * #include "castle/units/geo.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::geo_degrees_e7<int32_t> raw(375665000);           // u-blox UBX lat, 1e-7 deg
 * castle::units::geo_degrees<double> latitude = raw;                // implicit, 37.5665 deg
 * const bool ok = castle::units::is_valid_latitude(latitude);       // true
 * castle::units::geo_degrees<double> lon = 127.0_geo_deg;
 * @endcode
 *
 * Key constraints:
 * - Geographic angles carry `geo_kind`, so they never mix implicitly with plane
 *   angles such as `degrees<double>`; use `quantity_cast` to convert explicitly.
 * - `geo_degree_e7_unit` matches u-blox UBX int32 lat/lon (1e-7 deg);
 *   `geo_semicircle_unit` matches Garmin FIT int32 positions (180 deg / 2^31);
 *   `geo_arcsecond_unit` (1/3600 deg) and `geo_milliarcsecond_unit`
 *   (1/3600000 deg) suit arc-second based position formats.
 * - The angle.hpp helpers (`sin`, `cos`, `wrap_signed`, ...) accept geographic
 *   angles when angle.hpp is included.
 */
#ifndef CASTLE_UNITS_GEO_HPP
#define CASTLE_UNITS_GEO_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/units/quantity.hpp"

#include <stdint.h>

namespace castle
{
namespace units
{

/// @brief Kind tag for geographic coordinate angles.
struct geo_kind
{
};

/// @brief Geographic radian, the coherent geographic angle unit.
using geo_radian_unit = unit<dim::angle, math::ratio<1>, 0, math::ratio<0>, geo_kind>;
/// @brief Geographic degree, pi / 180 rad.
using geo_degree_unit = unit<dim::angle, math::ratio<1, 180>, 1, math::ratio<0>, geo_kind>;
/// @brief Geographic 1e-7 degree, as in u-blox UBX int32 latitude/longitude.
using geo_degree_e7_unit = unit<dim::angle, math::ratio<1, 1800000000>, 1, math::ratio<0>, geo_kind>;
/// @brief Geographic semicircle, 180 deg / 2^31, as in Garmin FIT positions.
using geo_semicircle_unit = unit<dim::angle, math::ratio<1, 2147483648>, 1, math::ratio<0>, geo_kind>;
/// @brief Geographic arc-second, 1/3600 degree.
using geo_arcsecond_unit = unit<dim::angle, math::ratio<1, 648000>, 1, math::ratio<0>, geo_kind>;
/// @brief Geographic milliarc-second, 1/3600000 degree.
using geo_milliarcsecond_unit = unit<dim::angle, math::ratio<1, 648000000>, 1, math::ratio<0>, geo_kind>;

/// @brief Geographic angle in radians.
template <typename Rep>
using geo_radians = quantity<geo_radian_unit, Rep>;
/// @brief Geographic angle in degrees.
template <typename Rep>
using geo_degrees = quantity<geo_degree_unit, Rep>;
/// @brief Geographic angle in 1e-7 degrees.
template <typename Rep>
using geo_degrees_e7 = quantity<geo_degree_e7_unit, Rep>;
/// @brief Geographic angle in semicircles.
template <typename Rep>
using geo_semicircles = quantity<geo_semicircle_unit, Rep>;
/// @brief Geographic angle in arc-seconds.
template <typename Rep>
using geo_arcseconds = quantity<geo_arcsecond_unit, Rep>;
/// @brief Geographic angle in milliarc-seconds.
template <typename Rep>
using geo_milliarcseconds = quantity<geo_milliarcsecond_unit, Rep>;

namespace detail
{

/**
 * @brief True when `Unit` is an angle unit tagged with `geo_kind`.
 */
template <typename Unit>
struct is_geo_angle
    : meta::bool_constant<meta::is_same<typename Unit::dimension, dim::angle>::value &&
                          meta::is_same<typename Unit::kind, geo_kind>::value>
{
};

} // namespace detail

/**
 * @brief Checks that a geographic angle is a valid latitude, -90 deg <= q <= 90 deg.
 * @param angle Geographic angle in any unit and representation.
 * @return True when the latitude lies within the closed range.
 * @note The comparison runs in `geo_degrees<double>`, so integral e7,
 *       semicircle or arc-second values cannot overflow.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD CASTLE_CONSTEXPR typename meta::enable_if<detail::is_geo_angle<Unit>::value, bool>::type
is_valid_latitude(CASTLE_CONST quantity<Unit, Rep>& angle) CASTLE_NOEXCEPT
{
    CASTLE_CONST geo_degrees<double> deg(angle);
    return (deg >= geo_degrees<double>(-90.0)) && (deg <= geo_degrees<double>(90.0));
}

/**
 * @brief Checks that a geographic angle is a valid longitude, -180 deg <= q <= 180 deg.
 * @param angle Geographic angle in any unit and representation.
 * @return True when the longitude lies within the closed range.
 * @note The comparison runs in `geo_degrees<double>`, so integral e7,
 *       semicircle or arc-second values cannot overflow.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD CASTLE_CONSTEXPR typename meta::enable_if<detail::is_geo_angle<Unit>::value, bool>::type
is_valid_longitude(CASTLE_CONST quantity<Unit, Rep>& angle) CASTLE_NOEXCEPT
{
    CASTLE_CONST geo_degrees<double> deg(angle);
    return (deg >= geo_degrees<double>(-180.0)) && (deg <= geo_degrees<double>(180.0));
}

namespace literals
{

/// @brief Floating geographic degrees literal, e.g. `37.5_geo_deg`.
CASTLE_CONSTEXPR geo_degrees<double> operator""_geo_deg(long double value) CASTLE_NOEXCEPT
{
    return geo_degrees<double>(static_cast<double>(value));
}

/// @brief Integer geographic degrees literal, e.g. `127_geo_deg`.
CASTLE_CONSTEXPR geo_degrees<int64_t> operator""_geo_deg(unsigned long long value) CASTLE_NOEXCEPT
{
    return geo_degrees<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating geographic radians literal, e.g. `0.5_geo_rad`.
CASTLE_CONSTEXPR geo_radians<double> operator""_geo_rad(long double value) CASTLE_NOEXCEPT
{
    return geo_radians<double>(static_cast<double>(value));
}

/// @brief Integer geographic radians literal, e.g. `1_geo_rad`.
CASTLE_CONSTEXPR geo_radians<int64_t> operator""_geo_rad(unsigned long long value) CASTLE_NOEXCEPT
{
    return geo_radians<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating geographic arc-seconds literal, e.g. `1.5_geo_arcsec`.
CASTLE_CONSTEXPR geo_arcseconds<double> operator""_geo_arcsec(long double value) CASTLE_NOEXCEPT
{
    return geo_arcseconds<double>(static_cast<double>(value));
}

/// @brief Integer geographic arc-seconds literal, e.g. `3600_geo_arcsec`.
CASTLE_CONSTEXPR geo_arcseconds<int64_t> operator""_geo_arcsec(unsigned long long value) CASTLE_NOEXCEPT
{
    return geo_arcseconds<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating geographic milliarc-seconds literal, e.g. `1.5_geo_mas`.
CASTLE_CONSTEXPR geo_milliarcseconds<double> operator""_geo_mas(long double value) CASTLE_NOEXCEPT
{
    return geo_milliarcseconds<double>(static_cast<double>(value));
}

/// @brief Integer geographic milliarc-seconds literal, e.g. `135239400_geo_mas`.
CASTLE_CONSTEXPR geo_milliarcseconds<int64_t> operator""_geo_mas(unsigned long long value) CASTLE_NOEXCEPT
{
    return geo_milliarcseconds<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_GEO_HPP
