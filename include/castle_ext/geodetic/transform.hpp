// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file transform.hpp
 * @brief Coordinate datum transformation between WGS-84 and GCJ-02.
 *
 * GCJ-02 ("Mars Coordinates") is the obfuscated coordinate system mandated
 * for public mapping services within mainland China. This header implements
 * the widely used non-linear offset algorithm (as published by, e.g., the
 * `eviltransform` project) that encodes a WGS-84 position into GCJ-02, plus
 * an iterative numerical inverse. Because the forward transform has no closed
 * form inverse, `gcj02_to_wgs84()` refines an initial guess by repeatedly
 * re-encoding it and correcting for the residual offset; a handful of
 * iterations is sufficient for sub-meter accuracy.
 *
 * @code
 * #include "castle_ext/geodetic/transform.hpp"
 *
 * using castle_ext::geodetic::geo_point;
 * const geo_point<double> beijing_wgs84(39.8946, 116.291);
 * const geo_point<double> beijing_gcj02 = castle_ext::geodetic::wgs84_to_gcj02(beijing_wgs84);
 * const geo_point<double> round_trip = castle_ext::geodetic::gcj02_to_wgs84(beijing_gcj02);
 * @endcode
 *
 * @note Like `distance.hpp`, this header depends on `<math.h>` trigonometric
 * wrappers and is therefore not `constexpr`.
 */
#ifndef CASTLE_EXT_GEODETIC_TRANSFORM_HPP
#define CASTLE_EXT_GEODETIC_TRANSFORM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/sqrt_real.hpp"
#include "castle/math/linalg/trigonometry.hpp"
#include "castle_ext/geodetic/coord.hpp"
#include "castle_ext/geodetic/geo_utils.hpp"
#include "castle_ext/geodetic/datum.hpp"

namespace castle_ext
{
namespace geodetic
{

namespace detail
{

/**
 * @brief Evaluates the GCJ-02 latitude offset polynomial.
 * @tparam T Floating-point type.
 * @param x Longitude offset from the GCJ-02 series origin (`longitude - 105`).
 * @param y Latitude offset from the GCJ-02 series origin (`latitude - 35`).
 * @return Raw (unscaled) latitude offset contribution, in the same units as
 * `x`/`y` before the ellipsoidal scaling applied by `wgs84_to_gcj02()`.
 */
template <typename T>
CASTLE_NODISCARD T gcj02_transform_lat(T x, T y) CASTLE_NOEXCEPT
{
    CASTLE_CONST T pi = castle::math::pi<T>();
    T ret = static_cast<T>(-100) + (static_cast<T>(2) * x) + (static_cast<T>(3) * y) + (static_cast<T>(0.2) * y * y)
          + (static_cast<T>(0.1) * x * y) + (static_cast<T>(0.2) * castle::math::sqrt_real(castle::math::abs(x)));
    ret += (static_cast<T>(20) * castle::math::radians_sin(static_cast<T>(6) * x * pi)
          + static_cast<T>(20) * castle::math::radians_sin(static_cast<T>(2) * x * pi)) * static_cast<T>(2) / static_cast<T>(3);
    ret += (static_cast<T>(20) * castle::math::radians_sin(y * pi)
          + static_cast<T>(40) * castle::math::radians_sin(y / static_cast<T>(3) * pi)) * static_cast<T>(2) / static_cast<T>(3);
    ret += (static_cast<T>(160) * castle::math::radians_sin(y / static_cast<T>(12) * pi)
          + static_cast<T>(320) * castle::math::radians_sin(y * pi / static_cast<T>(30))) * static_cast<T>(2) / static_cast<T>(3);
    return ret;
}

/**
 * @brief Evaluates the GCJ-02 longitude offset polynomial.
 * @tparam T Floating-point type.
 * @param x Longitude offset from the GCJ-02 series origin (`longitude - 105`).
 * @param y Latitude offset from the GCJ-02 series origin (`latitude - 35`).
 * @return Raw (unscaled) longitude offset contribution, in the same units as
 * `x`/`y` before the ellipsoidal scaling applied by `wgs84_to_gcj02()`.
 */
template <typename T>
CASTLE_NODISCARD T gcj02_transform_lon(T x, T y) CASTLE_NOEXCEPT
{
    CASTLE_CONST T pi = castle::math::pi<T>();
    T ret = static_cast<T>(300) + x + (static_cast<T>(2) * y) + (static_cast<T>(0.1) * x * x)
          + (static_cast<T>(0.1) * x * y) + (static_cast<T>(0.1) * castle::math::sqrt_real(castle::math::abs(x)));
    ret += (static_cast<T>(20) * castle::math::radians_sin(static_cast<T>(6) * x * pi)
          + static_cast<T>(20) * castle::math::radians_sin(static_cast<T>(2) * x * pi)) * static_cast<T>(2) / static_cast<T>(3);
    ret += (static_cast<T>(20) * castle::math::radians_sin(x * pi)
          + static_cast<T>(40) * castle::math::radians_sin(x / static_cast<T>(3) * pi)) * static_cast<T>(2) / static_cast<T>(3);
    ret += (static_cast<T>(150) * castle::math::radians_sin(x / static_cast<T>(12) * pi)
          + static_cast<T>(300) * castle::math::radians_sin(x / static_cast<T>(30) * pi)) * static_cast<T>(2) / static_cast<T>(3);
    return ret;
}

} // namespace detail

/**
 * @brief Tests whether a point lies inside the region where GCJ-02
 * obfuscation applies.
 * @tparam T Floating-point type.
 * @param point Point to test, interpreted as either WGS-84 or GCJ-02 (the
 * bounding box is coarse enough that either datum gives the same answer).
 * @return `true` when `point` falls within mainland China's bounding box, as
 * published for the GCJ-02 algorithm.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_in_gcj02_region(CASTLE_CONST geo_point<T>& point) CASTLE_NOEXCEPT
{
    return (point.longitude() >= gcj02_region<T>::min_longitude()) // LCOV_EXCL_BR_LINE
        && (point.longitude() <= gcj02_region<T>::max_longitude()) // LCOV_EXCL_BR_LINE
        && (point.latitude()  >= gcj02_region<T>::min_latitude())  // LCOV_EXCL_BR_LINE
        && (point.latitude()  <= gcj02_region<T>::max_latitude()); // LCOV_EXCL_BR_LINE
}

/**
 * @brief Converts a WGS-84 point to the GCJ-02 ("Mars Coordinates") datum.
 * @tparam T Floating-point type.
 * @param wgs84_point Point expressed in WGS-84.
 * @return The equivalent GCJ-02 point, or `wgs84_point` unchanged when it
 * falls outside `is_in_gcj02_region()`.
 */
template <typename T>
CASTLE_NODISCARD geo_point<T> wgs84_to_gcj02(CASTLE_CONST geo_point<T>& wgs84_point) CASTLE_NOEXCEPT
{
    static_assert(castle::meta::is_floating_point<T>::value, "wgs84_to_gcj02 requires a floating-point type");

    if (!is_in_gcj02_region(wgs84_point))
    {
        return wgs84_point;
    }

    CASTLE_CONST T a = krasovsky_ellipsoid<T>::semi_major_axis();
    CASTLE_CONST T ee = krasovsky_ellipsoid<T>::eccentricity_squared();
    CASTLE_CONST T pi = castle::math::pi<T>();

    T dlat = detail::gcj02_transform_lat(wgs84_point.longitude() - static_cast<T>(105),
                                         wgs84_point.latitude() - static_cast<T>(35));
    T dlon = detail::gcj02_transform_lon(wgs84_point.longitude() - static_cast<T>(105),
                                         wgs84_point.latitude() - static_cast<T>(35));

    CASTLE_CONST T rad_lat = castle::math::degrees_to_radians(wgs84_point.latitude());
    T magic = castle::math::radians_sin(rad_lat);
    magic = static_cast<T>(1) - (ee * magic * magic);
    CASTLE_CONST T sqrt_magic = castle::math::sqrt_real(magic);

    dlat = (dlat * static_cast<T>(180)) / (((a * (static_cast<T>(1) - ee)) / (magic * sqrt_magic)) * pi);
    dlon = (dlon * static_cast<T>(180)) / ((a / sqrt_magic) * castle::math::radians_cos(rad_lat) * pi);

    CASTLE_CONST T lat = clamp_latitude(wgs84_point.latitude() + dlat);
    CASTLE_CONST T lon = normalize_longitude(wgs84_point.longitude() + dlon);
    return geo_point<T>(lat, lon);
}

/**
 * @brief Converts a GCJ-02 ("Mars Coordinates") point back to WGS-84.
 * @tparam T Floating-point type.
 * @tparam Iterations Number of refinement passes used to numerically invert
 * `wgs84_to_gcj02()`. Three iterations (the default) converge to
 * sub-centimeter residuals for every point inside the GCJ-02 region.
 * @param gcj02_point Point expressed in GCJ-02.
 * @return The equivalent WGS-84 point, or `gcj02_point` unchanged when it
 * falls outside `is_in_gcj02_region()`.
 * @note GCJ-02 has no closed-form inverse; this refines an initial guess by
 * repeatedly re-encoding it with `wgs84_to_gcj02()` and correcting for the
 * residual offset, which converges quickly because the offset varies
 * smoothly and is bounded to a few hundred meters.
 */
template <typename T, castle::size_type Iterations = 3U>
CASTLE_NODISCARD geo_point<T> gcj02_to_wgs84(CASTLE_CONST geo_point<T>& gcj02_point) CASTLE_NOEXCEPT
{
    static_assert(castle::meta::is_floating_point<T>::value, "gcj02_to_wgs84 requires a floating-point type");
    static_assert(Iterations > 0U, "gcj02_to_wgs84 requires at least one refinement iteration");

    if (!is_in_gcj02_region(gcj02_point))
    {
        return gcj02_point;
    }

    geo_point<T> guess = gcj02_point;

    for (castle::size_type i = 0U; i < Iterations; ++i)
    {
        CASTLE_CONST geo_point<T> re_encoded = wgs84_to_gcj02(guess);
        CASTLE_CONST T lat = clamp_latitude(guess.latitude() - (re_encoded.latitude() - gcj02_point.latitude()));
        CASTLE_CONST T lon = normalize_longitude(guess.longitude() - (re_encoded.longitude() - gcj02_point.longitude()));
        guess = geo_point<T>(lat, lon);
    }

    return guess;
}

} // namespace geodetic
} // namespace castle_ext

#endif // CASTLE_EXT_GEODETIC_TRANSFORM_HPP
