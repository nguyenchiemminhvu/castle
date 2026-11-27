// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file angle.hpp
 * @brief Plane-angle units, unit-safe trigonometry and angle wrapping.
 *
 * Angles are a separate dimension in Castle units, so a heading in degrees can
 * never be passed where a raw scalar or a frequency is expected. Degrees,
 * arc-minutes, arc-seconds and revolutions carry an exact pi factor relative
 * to radians, so conversions between them are exact for integral values that
 * share the pi factor and are evaluated once in floating point otherwise.
 *
 * Example:
 * @code
 * #include "castle/units/angle.hpp"
 *
 * using namespace castle::units::literals;
 *
 * castle::units::degrees<double> heading = 370.0_deg;
 * heading = castle::units::wrap_positive(heading);        // 10 deg
 * const double s = castle::units::sin(heading);           // sin(10 deg)
 * castle::units::radians<double> r = heading;            // implicit, lossless
 * @endcode
 *
 * Key constraints:
 * - Trigonometric helpers wrap the platform C math library through
 *   `castle/math/linalg/trigonometry.hpp`.
 * - Wrapping a floating angle uses `fmod`; wrapping an integral angle needs a
 *   whole number of units per revolution (degrees, arc-minutes, ...).
 */
#ifndef CASTLE_UNITS_ANGLE_HPP
#define CASTLE_UNITS_ANGLE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/math/linalg/trigonometry.hpp"
#include "castle/math/mod.hpp"
#include "castle/units/quantity.hpp"
#include "castle/units/scale.hpp"

#include <math.h>
#include <stdint.h>

namespace castle
{
namespace units
{

/// @brief Radian, the coherent plane-angle unit.
using radian_unit = unit<dim::angle>;
/// @brief Milliradian.
using milliradian_unit = scaled_unit_t<radian_unit, milli>;
/// @brief Degree, pi / 180 rad.
using degree_unit = unit<dim::angle, math::ratio<1, 180>, 1>;
/// @brief Arc-minute, 1/60 degree.
using arcminute_unit = unit<dim::angle, math::ratio<1, 10800>, 1>;
/// @brief Arc-second, 1/3600 degree.
using arcsecond_unit = unit<dim::angle, math::ratio<1, 648000>, 1>;
/// @brief Revolution (turn), 2 pi rad.
using revolution_unit = unit<dim::angle, math::ratio<2>, 1>;

/// @brief Angle in radians.
template <typename Rep>
using radians = quantity<radian_unit, Rep>;
/// @brief Angle in milliradians.
template <typename Rep>
using milliradians = quantity<milliradian_unit, Rep>;
/// @brief Angle in degrees.
template <typename Rep>
using degrees = quantity<degree_unit, Rep>;
/// @brief Angle in arc-minutes.
template <typename Rep>
using arcminutes = quantity<arcminute_unit, Rep>;
/// @brief Angle in arc-seconds.
template <typename Rep>
using arcseconds = quantity<arcsecond_unit, Rep>;
/// @brief Angle in revolutions.
template <typename Rep>
using revolutions = quantity<revolution_unit, Rep>;

namespace detail
{

/**
 * @brief Angle traits: floating type for trigonometry and the radian unit of the same kind.
 */
template <typename Unit, typename Rep>
struct angle_traits
{
    static CASTLE_CONSTEXPR bool value = meta::is_same<typename Unit::dimension, dim::angle>::value;
    using real = floating_rep_t<Rep, Rep>;
    using radian = unit<dim::angle, math::ratio<1>, 0, math::ratio<0>, typename Unit::kind>;
};

template <typename Unit, typename Rep>
using enable_if_angle_t = typename meta::enable_if<angle_traits<Unit, Rep>::value,
                                                   typename angle_traits<Unit, Rep>::real>::type;

template <typename Unit, typename Rep>
CASTLE_NODISCARD CASTLE_CONSTEXPR typename angle_traits<Unit, Rep>::real
radian_value(CASTLE_CONST quantity<Unit, Rep>& angle) CASTLE_NOEXCEPT
{
    using traits = angle_traits<Unit, Rep>;
    return convert_value<Unit, typename traits::radian, typename traits::real>(angle.value());
}

CASTLE_INLINE float fmod(float x, float y) CASTLE_NOEXCEPT
{
    return ::fmodf(x, y);
}

CASTLE_INLINE double fmod(double x, double y) CASTLE_NOEXCEPT
{
    return ::fmod(x, y);
}

CASTLE_INLINE long double fmod(long double x, long double y) CASTLE_NOEXCEPT
{
    return ::fmodl(x, y);
}

/**
 * @brief Number of `Unit` steps in one revolution, as an exact integer.
 * @note Requires a pi factor of one and a whole number of units per turn.
 */
template <typename Unit>
struct integral_turn
{
    static_assert(Unit::pi_exponent == 1,
                  "castle::units: integral angle wrapping needs a unit with a pi factor (degrees, ...)");
    static_assert(((2 * Unit::ratio::den) % Unit::ratio::num) == 0,
                  "castle::units: integral angle wrapping needs a whole number of units per revolution");

    static CASTLE_CONSTEXPR intmax_t value = (2 * Unit::ratio::den) / Unit::ratio::num;
};

/**
 * @brief One revolution expressed in a floating `Unit`.
 */
template <typename Unit, typename Real>
CASTLE_NODISCARD CASTLE_CONSTEXPR Real floating_turn() CASTLE_NOEXCEPT
{
    static_assert((Unit::pi_exponent == 0) || (Unit::pi_exponent == 1),
                  "castle::units: angle wrapping supports pi exponents 0 and 1");

    return static_cast<Real>(2 * Unit::ratio::den) / static_cast<Real>(Unit::ratio::num) *
           pi_power<Real>(1 - Unit::pi_exponent);
}

template <typename Real>
CASTLE_NODISCARD Real wrap_floating(Real value, Real turn) CASTLE_NOEXCEPT
{
    Real result = detail::fmod(value, turn);
    if (result < static_cast<Real>(0))
    {
        result += turn;
    }
    // A tiny negative remainder can round up to exactly one turn.
    if (result >= turn)
    {
        result = static_cast<Real>(0);
    }
    return result;
}

} // namespace detail

/**
 * @brief Sine of an angle quantity.
 * @param angle Angle in any angle unit (any kind).
 * @return `sin(angle)`; floating type of `Rep`, or `double` for integral reps.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD detail::enable_if_angle_t<Unit, Rep>
sin(CASTLE_CONST quantity<Unit, Rep>& angle) CASTLE_NOEXCEPT
{
    return math::sin(detail::radian_value(angle));
}

/**
 * @brief Cosine of an angle quantity.
 * @param angle Angle in any angle unit (any kind).
 * @return `cos(angle)`; floating type of `Rep`, or `double` for integral reps.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD detail::enable_if_angle_t<Unit, Rep>
cos(CASTLE_CONST quantity<Unit, Rep>& angle) CASTLE_NOEXCEPT
{
    return math::cos(detail::radian_value(angle));
}

/**
 * @brief Tangent of an angle quantity.
 * @param angle Angle in any angle unit (any kind).
 * @return `sin(angle) / cos(angle)`.
 * @warning Not checked near odd multiples of 90 degrees.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD detail::enable_if_angle_t<Unit, Rep>
tan(CASTLE_CONST quantity<Unit, Rep>& angle) CASTLE_NOEXCEPT
{
    CASTLE_CONST math::sin_cos_result<detail::enable_if_angle_t<Unit, Rep>> sc =
        math::sin_cos(detail::radian_value(angle));
    return sc.sine / sc.cosine;
}

/**
 * @brief Arc sine returning a typed angle.
 * @tparam T Floating-point type.
 * @param value Input in `[-1, 1]`.
 * @return Angle in `[-pi/2, pi/2]` radians.
 */
template <typename T>
CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<T>::value, radians<T>>::type
asin(T value) CASTLE_NOEXCEPT
{
    return radians<T>(math::asin(value));
}

/**
 * @brief Arc cosine returning a typed angle.
 * @tparam T Floating-point type.
 * @param value Input in `[-1, 1]`.
 * @return Angle in `[0, pi]` radians, computed as `pi/2 - asin(value)`.
 */
template <typename T>
CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<T>::value, radians<T>>::type
acos(T value) CASTLE_NOEXCEPT
{
    return radians<T>(static_cast<T>((math::pi<T>() / static_cast<T>(2)) - math::asin(value)));
}

/**
 * @brief Quadrant-aware arc tangent of `y / x` returning a typed angle.
 * @tparam T Floating-point type.
 * @param y Ordinate.
 * @param x Abscissa.
 * @return Angle in `[-pi, pi]` radians.
 */
template <typename T>
CASTLE_NODISCARD typename meta::enable_if<meta::is_floating_point<T>::value, radians<T>>::type
atan2(T y, T x) CASTLE_NOEXCEPT
{
    return radians<T>(math::atan2(y, x));
}

/**
 * @brief Quadrant-aware arc tangent of two compatible quantities, e.g. two lengths.
 * @param y Ordinate quantity.
 * @param x Abscissa quantity of the same dimension and kind.
 * @return Angle in `[-pi, pi]` radians; floating type of the common representation.
 */
template <typename Unit1, typename Rep1, typename Unit2, typename Rep2>
CASTLE_NODISCARD radians<detail::floating_rep_t<
    typename detail::common_quantity<Unit1, Rep1, Unit2, Rep2>::type::rep,
    typename detail::common_quantity<Unit1, Rep1, Unit2, Rep2>::type::rep>>
atan2(CASTLE_CONST quantity<Unit1, Rep1>& y, CASTLE_CONST quantity<Unit2, Rep2>& x) CASTLE_NOEXCEPT
{
    using common = typename detail::common_quantity<Unit1, Rep1, Unit2, Rep2>::type;
    using real = detail::floating_rep_t<typename common::rep, typename common::rep>;
    return radians<real>(math::atan2(static_cast<real>(common(y).value()),
                                     static_cast<real>(common(x).value())));
}

/**
 * @brief Wraps an angle into `[0, one revolution)` in its own unit and kind.
 * @param angle Angle to wrap.
 * @return Equivalent angle, e.g. `[0, 360)` for degrees or `[0, 2 pi)` for radians.
 * @note Integral angles need a whole number of units per revolution that fits `Rep`.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD typename meta::enable_if<detail::angle_traits<Unit, Rep>::value, quantity<Unit, Rep>>::type
wrap_positive(CASTLE_CONST quantity<Unit, Rep>& angle) CASTLE_NOEXCEPT
{
    CASTLE_IF_CONSTEXPR (meta::is_floating_point<Rep>::value)
    {
        return quantity<Unit, Rep>(detail::wrap_floating(angle.value(), detail::floating_turn<Unit, Rep>()));
    }
    else
    {
        CASTLE_CONSTEXPR intmax_t turn = detail::integral_turn<Unit>::value;
        static_assert((turn - 1) <= static_cast<intmax_t>(castle::numeric_limits<Rep>::max()),
                      "castle::units::wrap_positive: one revolution does not fit the representation");
        return quantity<Unit, Rep>(static_cast<Rep>(
            math::wrap(static_cast<intmax_t>(angle.value()), static_cast<intmax_t>(0), turn)));
    }
}

/**
 * @brief Wraps an angle into `[-half revolution, half revolution)` in its own unit and kind.
 * @param angle Angle to wrap.
 * @return Equivalent angle, e.g. `[-180, 180)` for degrees or `[-pi, pi)` for radians.
 * @note Integral angles need an even number of units per revolution.
 */
template <typename Unit, typename Rep>
CASTLE_NODISCARD typename meta::enable_if<detail::angle_traits<Unit, Rep>::value, quantity<Unit, Rep>>::type
wrap_signed(CASTLE_CONST quantity<Unit, Rep>& angle) CASTLE_NOEXCEPT
{
    CASTLE_IF_CONSTEXPR (meta::is_floating_point<Rep>::value)
    {
        CASTLE_CONST Rep turn = detail::floating_turn<Unit, Rep>();
        CASTLE_CONST Rep half = turn / static_cast<Rep>(2);
        return quantity<Unit, Rep>(static_cast<Rep>(detail::wrap_floating(static_cast<Rep>(angle.value() + half), turn) - half));
    }
    else
    {
        CASTLE_CONSTEXPR intmax_t turn = detail::integral_turn<Unit>::value;
        static_assert((turn % 2) == 0,
                      "castle::units::wrap_signed: needs an even number of units per revolution");
        CASTLE_CONSTEXPR intmax_t half = turn / 2;
        static_assert(meta::is_signed<Rep>::value &&
                      ((half - 1) <= static_cast<intmax_t>(castle::numeric_limits<Rep>::max())),
                      "castle::units::wrap_signed: half a revolution does not fit the signed representation");
        return quantity<Unit, Rep>(static_cast<Rep>(
            math::wrap(static_cast<intmax_t>(angle.value()), -half, half)));
    }
}

namespace literals
{

/// @brief Floating radians literal, e.g. `1.57_rad`.
CASTLE_CONSTEXPR radians<double> operator""_rad(long double value) CASTLE_NOEXCEPT
{
    return radians<double>(static_cast<double>(value));
}

/// @brief Integer radians literal, e.g. `1_rad`.
CASTLE_CONSTEXPR radians<int64_t> operator""_rad(unsigned long long value) CASTLE_NOEXCEPT
{
    return radians<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating milliradians literal, e.g. `2.5_mrad`.
CASTLE_CONSTEXPR milliradians<double> operator""_mrad(long double value) CASTLE_NOEXCEPT
{
    return milliradians<double>(static_cast<double>(value));
}

/// @brief Integer milliradians literal, e.g. `25_mrad`.
CASTLE_CONSTEXPR milliradians<int64_t> operator""_mrad(unsigned long long value) CASTLE_NOEXCEPT
{
    return milliradians<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating degrees literal, e.g. `45.5_deg`.
CASTLE_CONSTEXPR degrees<double> operator""_deg(long double value) CASTLE_NOEXCEPT
{
    return degrees<double>(static_cast<double>(value));
}

/// @brief Integer degrees literal, e.g. `90_deg`.
CASTLE_CONSTEXPR degrees<int64_t> operator""_deg(unsigned long long value) CASTLE_NOEXCEPT
{
    return degrees<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating arc-minutes literal, e.g. `1.5_arcmin`.
CASTLE_CONSTEXPR arcminutes<double> operator""_arcmin(long double value) CASTLE_NOEXCEPT
{
    return arcminutes<double>(static_cast<double>(value));
}

/// @brief Integer arc-minutes literal, e.g. `30_arcmin`.
CASTLE_CONSTEXPR arcminutes<int64_t> operator""_arcmin(unsigned long long value) CASTLE_NOEXCEPT
{
    return arcminutes<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating arc-seconds literal, e.g. `1.5_arcsec`.
CASTLE_CONSTEXPR arcseconds<double> operator""_arcsec(long double value) CASTLE_NOEXCEPT
{
    return arcseconds<double>(static_cast<double>(value));
}

/// @brief Integer arc-seconds literal, e.g. `30_arcsec`.
CASTLE_CONSTEXPR arcseconds<int64_t> operator""_arcsec(unsigned long long value) CASTLE_NOEXCEPT
{
    return arcseconds<int64_t>(static_cast<int64_t>(value));
}

/// @brief Floating revolutions literal, e.g. `0.25_rev`.
CASTLE_CONSTEXPR revolutions<double> operator""_rev(long double value) CASTLE_NOEXCEPT
{
    return revolutions<double>(static_cast<double>(value));
}

/// @brief Integer revolutions literal, e.g. `3_rev`.
CASTLE_CONSTEXPR revolutions<int64_t> operator""_rev(unsigned long long value) CASTLE_NOEXCEPT
{
    return revolutions<int64_t>(static_cast<int64_t>(value));
}

} // namespace literals
} // namespace units
} // namespace castle

#endif // CASTLE_UNITS_ANGLE_HPP
