// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file datum.hpp
 * @brief Reference ellipsoid and datum constants for the geodetic module.
 *
 * This header defines the numeric constants that anchor every other geodetic calculation.
 * Values are exposed as static constexpr accessors on small policy types
 * (rather than free constants) so that distance and transform algorithms
 * can be parameterized on the ellipsoid via a template argument,
 * matching Castle's policy-based configuration style.
 *
 * @code
 * #include "castle_ext/geodetic/datum.hpp"
 *
 * constexpr double a  = castle_ext::geodetic::wgs84_ellipsoid<double>::semi_major_axis();
 * constexpr double f  = castle_ext::geodetic::wgs84_ellipsoid<double>::flattening();
 * constexpr double b  = castle_ext::geodetic::wgs84_ellipsoid<double>::semi_minor_axis();
 * @endcode
 *
 * @note All members are `static constexpr` functions rather than data members
 * so the policy types remain empty, trivially constructible, and usable
 * purely as compile-time template arguments.
 */
#ifndef CASTLE_EXT_GEODETIC_DATUM_HPP
#define CASTLE_EXT_GEODETIC_DATUM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle_ext
{
namespace geodetic
{

/**
 * @brief WGS-84 reference ellipsoid parameters.
 * @tparam T Floating-point type used to represent the constants.
 * @note WGS-84 is the datum used by GPS and by every other GNSS constellation
 * after datum transformation. It is the default ellipsoid policy for
 * `castle_ext::geodetic` distance and transform algorithms.
 */
template <typename T>
struct wgs84_ellipsoid
{
    static_assert(castle::meta::is_floating_point<T>::value,
                  "wgs84_ellipsoid requires a floating-point type");

    /** @brief Semi-major axis (equatorial radius) in meters. */
    static CASTLE_CONSTEXPR T semi_major_axis() CASTLE_NOEXCEPT
    {
        return static_cast<T>(6378137.0);
    }

    /** @brief Flattening `f = (a - b) / a`. */
    static CASTLE_CONSTEXPR T flattening() CASTLE_NOEXCEPT
    {
        return static_cast<T>(1.0 / 298.257223563);
    }

    /** @brief Semi-minor axis (polar radius) in meters, derived from `a` and `f`. */
    static CASTLE_CONSTEXPR T semi_minor_axis() CASTLE_NOEXCEPT
    {
        return semi_major_axis() * (static_cast<T>(1) - flattening());
    }

    /** @brief First eccentricity squared, derived from `f`. */
    static CASTLE_CONSTEXPR T eccentricity_squared() CASTLE_NOEXCEPT
    {
        return flattening() * (static_cast<T>(2) - flattening());
    }
};

/**
 * @brief PZ-90.11 reference ellipsoid parameters.
 * @tparam T Floating-point type used to represent the constants.
 * @note PZ-90 ("Parametry Zemli 1990", updated as PZ-90.11) is the datum
 * used by the Russian GLONASS GNSS constellation, analogous to WGS-84 for
 * GPS. Its ellipsoid is very close to, but distinct from, WGS-84's.
 */
template <typename T>
struct pz90_ellipsoid
{
    static_assert(castle::meta::is_floating_point<T>::value,
                  "pz90_ellipsoid requires a floating-point type");

    /** @brief Semi-major axis (equatorial radius) in meters. */
    static CASTLE_CONSTEXPR T semi_major_axis() CASTLE_NOEXCEPT
    {
        return static_cast<T>(6378136.0);
    }

    /** @brief Flattening `f = (a - b) / a`. */
    static CASTLE_CONSTEXPR T flattening() CASTLE_NOEXCEPT
    {
        return static_cast<T>(1.0 / 298.257839303);
    }

    /** @brief Semi-minor axis (polar radius) in meters, derived from `a` and `f`. */
    static CASTLE_CONSTEXPR T semi_minor_axis() CASTLE_NOEXCEPT
    {
        return semi_major_axis() * (static_cast<T>(1) - flattening());
    }

    /** @brief First eccentricity squared, derived from `f`. */
    static CASTLE_CONSTEXPR T eccentricity_squared() CASTLE_NOEXCEPT
    {
        return flattening() * (static_cast<T>(2) - flattening());
    }
};

/**
 * @brief Krasovsky (1940) reference ellipsoid parameters.
 * @tparam T Floating-point type used to represent the constants.
 * @note The GCJ-02 obfuscation algorithm is defined in terms of this
 * ellipsoid; it is not a general-purpose geodetic datum.
 */
template <typename T>
struct krasovsky_ellipsoid
{
    static_assert(castle::meta::is_floating_point<T>::value,
                  "krasovsky_ellipsoid requires a floating-point type");

    /** @brief Semi-major axis (equatorial radius) in meters. */
    static CASTLE_CONSTEXPR T semi_major_axis() CASTLE_NOEXCEPT
    {
        return static_cast<T>(6378245.0);
    }

    /** @brief First eccentricity squared, as published for the GCJ-02 algorithm. */
    static CASTLE_CONSTEXPR T eccentricity_squared() CASTLE_NOEXCEPT
    {
        return static_cast<T>(0.00669342162296594323);
    }

    /**
     * @note This policy intentionally omits `semi_minor_axis()`. The GCJ-02
     * offset formulas in `transform.hpp` only require `semi_major_axis()` and
     * `eccentricity_squared()`; callers needing the semi-minor axis can
     * derive it as `castle::math::sqrt_real(a * a * (1 - e^2))`.
     */
};

/**
 * @brief Geographic bounding box outside of which GCJ-02 obfuscation does not
 * apply.
 * @tparam T Floating-point type used to represent the constants.
 * @note Coordinates outside this region already use WGS-84 in every public
 * Chinese mapping service, so `castle_ext::geodetic` transform functions
 * return their input unchanged outside of this box.
 */
template <typename T>
struct gcj02_region
{
    static_assert(castle::meta::is_floating_point<T>::value,
                  "gcj02_region requires a floating-point type");

    /** @brief Minimum (westernmost) longitude of the obfuscated region, in decimal degrees. */
    static CASTLE_CONSTEXPR T min_longitude() CASTLE_NOEXCEPT { return static_cast<T>(72.004); }
    /** @brief Maximum (easternmost) longitude of the obfuscated region, in decimal degrees. */
    static CASTLE_CONSTEXPR T max_longitude() CASTLE_NOEXCEPT { return static_cast<T>(137.8347); }
    /** @brief Minimum (southernmost) latitude of the obfuscated region, in decimal degrees. */
    static CASTLE_CONSTEXPR T min_latitude() CASTLE_NOEXCEPT { return static_cast<T>(0.8293); }
    /** @brief Maximum (northernmost) latitude of the obfuscated region, in decimal degrees. */
    static CASTLE_CONSTEXPR T max_latitude() CASTLE_NOEXCEPT { return static_cast<T>(55.8271); }
};

} // namespace geodetic
} // namespace castle_ext

#endif // CASTLE_EXT_GEODETIC_DATUM_HPP
