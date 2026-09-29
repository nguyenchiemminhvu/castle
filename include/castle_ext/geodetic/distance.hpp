// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file distance.hpp
 * @brief Great-circle and ellipsoidal distance/bearing calculations.
 *
 * This header provides two families of algorithms over `geo_point<T>`:
 *  - `haversine_distance()` / `initial_bearing()` / `final_bearing()`: fast,
 *    spherical-earth approximations suitable when sub-0.5% distance error is
 *    acceptable; and
 *  - `vincenty_inverse()` / `vincenty_direct()`: iterative, ellipsoidal
 *    solutions to the geodetic inverse and direct problems, accurate to
 *    millimeters on the configured reference ellipsoid.
 *
 * Both families are parameterized on an ellipsoid policy type (see
 * `datum.hpp`) so callers can switch reference ellipsoids without
 * touching call sites. All functions depend on `<math.h>` trigonometric
 * wrappers, so unlike `coord.hpp`, none of them are `constexpr`.
 *
 * @code
 * #include "castle_ext/geodetic/distance.hpp"
 *
 * using castle_ext::geodetic::geo_point;
 * const geo_point<double> hanoi(21.0278, 105.8342);
 * const geo_point<double> saigon(10.7626, 106.6602);
 *
 * const double approx_m = castle_ext::geodetic::haversine_distance(hanoi, saigon);
 *
 * castle_ext::geodetic::vincenty_result<double> precise;
 * const castle::status result = castle_ext::geodetic::vincenty_inverse(hanoi, saigon, precise);
 * @endcode
 */
#ifndef CASTLE_EXT_GEODETIC_DISTANCE_HPP
#define CASTLE_EXT_GEODETIC_DISTANCE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
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
 * @brief Tests whether a floating-point value is IEEE-754 NaN.
 * @tparam T Floating-point type.
 * @param value Value to test.
 * @return `true` when `value` is NaN.
 * @note Relies on the IEEE-754 property that NaN never compares equal to
 * itself, avoiding a dependency on `<math.h>` classification helpers.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if_t<castle::meta::is_floating_point<T>::value, bool>
is_nan(T value) CASTLE_NOEXCEPT
{
    return value != value; // LCOV_EXCL_BR_LINE
}

/**
 * @brief Normalizes a bearing in decimal degrees into `[0, 360)`.
 * @tparam T Floating-point type.
 * @param bearing_deg Bearing in decimal degrees, any finite value.
 * @return The equivalent bearing wrapped into `[0, 360)`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR T normalize_bearing(T bearing_deg) CASTLE_NOEXCEPT
{
    T wrapped = static_cast<T>(::fmod(static_cast<double>(bearing_deg), 360.0));
    if (wrapped < static_cast<T>(0))
    {
        wrapped += static_cast<T>(360);
    }
    return wrapped;
}

} // namespace detail

/**
 * @brief Computes the great-circle distance between two points on a sphere
 * using the Haversine formula.
 * @tparam T Floating-point type.
 * @tparam Ellipsoid Ellipsoid policy supplying the spherical radius; only
 * `semi_major_axis()` is used. Defaults to `wgs84_ellipsoid<T>`.
 * @param from Starting point.
 * @param to Destination point.
 * @return Distance between `from` and `to`, in meters.
 * @note This treats the earth as a perfect sphere; expect up to ~0.5% error
 * relative to an ellipsoidal solution such as `vincenty_inverse()`.
 */
template <typename T, typename Ellipsoid = wgs84_ellipsoid<T>>
CASTLE_NODISCARD T haversine_distance(CASTLE_CONST geo_point<T>& from, CASTLE_CONST geo_point<T>& to) CASTLE_NOEXCEPT
{
    static_assert(castle::meta::is_floating_point<T>::value, "haversine_distance requires a floating-point type");

    CASTLE_CONST T lat1 = castle::math::degrees_to_radians(from.latitude());
    CASTLE_CONST T lat2 = castle::math::degrees_to_radians(to.latitude());
    CASTLE_CONST T dlat = lat2 - lat1;
    CASTLE_CONST T dlon = castle::math::degrees_to_radians(to.longitude() - from.longitude());

    CASTLE_CONST T sin_dlat_2 = castle::math::radians_sin(dlat / static_cast<T>(2));
    CASTLE_CONST T sin_dlon_2 = castle::math::radians_sin(dlon / static_cast<T>(2));

    CASTLE_CONST T a = (sin_dlat_2 * sin_dlat_2)
                     + (castle::math::radians_cos(lat1) * castle::math::radians_cos(lat2) * sin_dlon_2 * sin_dlon_2);
    CASTLE_CONST T c = static_cast<T>(2) * castle::math::radians_atan2(castle::math::sqrt_real(a),
                                                                    castle::math::sqrt_real(static_cast<T>(1) - a));

    return Ellipsoid::semi_major_axis() * c;
}

/**
 * @brief Computes the initial (forward) great-circle bearing from one point
 * to another on a sphere.
 * @tparam T Floating-point type.
 * @param from Starting point.
 * @param to Destination point.
 * @return Bearing at `from`, in decimal degrees clockwise from true north,
 * normalized to `[0, 360)`.
 */
template <typename T>
CASTLE_NODISCARD T initial_bearing(CASTLE_CONST geo_point<T>& from, CASTLE_CONST geo_point<T>& to) CASTLE_NOEXCEPT
{
    static_assert(castle::meta::is_floating_point<T>::value, "initial_bearing requires a floating-point type");

    CASTLE_CONST T lat1 = castle::math::degrees_to_radians(from.latitude());
    CASTLE_CONST T lat2 = castle::math::degrees_to_radians(to.latitude());
    CASTLE_CONST T dlon = castle::math::degrees_to_radians(to.longitude() - from.longitude());

    CASTLE_CONST T y = castle::math::radians_sin(dlon) * castle::math::radians_cos(lat2);
    CASTLE_CONST T x = (castle::math::radians_cos(lat1) * castle::math::radians_sin(lat2))
                      - (castle::math::radians_sin(lat1) * castle::math::radians_cos(lat2) * castle::math::radians_cos(dlon));

    CASTLE_CONST T bearing_deg = castle::math::radians_to_degrees(castle::math::radians_atan2(y, x));
    return detail::normalize_bearing(bearing_deg);
}

/**
 * @brief Computes the final (arrival) great-circle bearing from one point to
 * another on a sphere.
 * @tparam T Floating-point type.
 * @param from Starting point.
 * @param to Destination point.
 * @return Bearing on arrival at `to`, in decimal degrees clockwise from true
 * north, normalized to `[0, 360)`.
 * @note Computed as the reciprocal of the initial bearing of the reversed
 * path, matching the standard great-circle definition.
 */
template <typename T>
CASTLE_NODISCARD T final_bearing(CASTLE_CONST geo_point<T>& from, CASTLE_CONST geo_point<T>& to) CASTLE_NOEXCEPT
{
    CASTLE_CONST T reverse_bearing = initial_bearing(to, from);
    return detail::normalize_bearing(reverse_bearing + static_cast<T>(180));
}

/**
 * @brief Bundles the outputs of `vincenty_inverse()`.
 * @tparam T Floating-point type.
 */
template <typename T>
struct vincenty_result
{
    /** @brief Ellipsoidal distance between the two points, in meters. */
    T distance;
    /** @brief Bearing at the starting point, in decimal degrees, normalized to `[0, 360)`. */
    T initial_bearing_deg;
    /** @brief Bearing on arrival at the destination, in decimal degrees, normalized to `[0, 360)`. */
    T final_bearing_deg;
};

/**
 * @brief Solves the geodetic inverse problem (distance and bearings between
 * two points) on a reference ellipsoid using Vincenty's iterative formula.
 * @tparam T Floating-point type.
 * @tparam Ellipsoid Ellipsoid policy type (see `datum.hpp`). Defaults to
 * `wgs84_ellipsoid<T>`.
 * @param from Starting point.
 * @param to Destination point.
 * @param result Receives the distance and bearings on success.
 * @param max_iterations Iteration budget for the Lambda fixed-point loop.
 * @return `castle::status::ok` on convergence; `castle::status::unknown_error`
 * when the iteration does not converge within `max_iterations` (this is a
 * known limitation of Vincenty's formula for nearly-antipodal points).
 * @note `result` is left unmodified when this function returns anything
 * other than `castle::status::ok`.
 */
template <typename T, typename Ellipsoid = wgs84_ellipsoid<T>>
CASTLE_NODISCARD castle::status vincenty_inverse(CASTLE_CONST geo_point<T>& from, CASTLE_CONST geo_point<T>& to,
                                                  vincenty_result<T>& result,
                                                  castle::size_type max_iterations = 200U) CASTLE_NOEXCEPT
{
    static_assert(castle::meta::is_floating_point<T>::value, "vincenty_inverse requires a floating-point type");

    CASTLE_CONST T a = Ellipsoid::semi_major_axis();
    CASTLE_CONST T f = Ellipsoid::flattening();
    CASTLE_CONST T b = Ellipsoid::semi_minor_axis();

    CASTLE_CONST T u1 = castle::math::radians_atan2((static_cast<T>(1) - f) * castle::math::radians_sin(castle::math::degrees_to_radians(from.latitude())),
                                                 castle::math::radians_cos(castle::math::degrees_to_radians(from.latitude())));
    CASTLE_CONST T u2 = castle::math::radians_atan2((static_cast<T>(1) - f) * castle::math::radians_sin(castle::math::degrees_to_radians(to.latitude())),
                                                 castle::math::radians_cos(castle::math::degrees_to_radians(to.latitude())));
    CASTLE_CONST T big_l = castle::math::degrees_to_radians(to.longitude() - from.longitude());

    CASTLE_CONST T sin_u1 = castle::math::radians_sin(u1);
    CASTLE_CONST T cos_u1 = castle::math::radians_cos(u1);
    CASTLE_CONST T sin_u2 = castle::math::radians_sin(u2);
    CASTLE_CONST T cos_u2 = castle::math::radians_cos(u2);

    T lambda = big_l;
    T sin_sigma = static_cast<T>(0);
    T cos_sigma = static_cast<T>(0);
    T sigma = static_cast<T>(0);
    T cos_sq_alpha = static_cast<T>(0);
    T cos_2sigma_m = static_cast<T>(0);
    bool converged = false;

    for (castle::size_type i = 0U; i < max_iterations; ++i) // LCOV_EXCL_BR_LINE
    {
        CASTLE_CONST T sin_lambda = castle::math::radians_sin(lambda);
        CASTLE_CONST T cos_lambda = castle::math::radians_cos(lambda);

        CASTLE_CONST T term1 = cos_u2 * sin_lambda;
        CASTLE_CONST T term2 = (cos_u1 * sin_u2) - (sin_u1 * cos_u2 * cos_lambda);
        sin_sigma = castle::math::sqrt_real((term1 * term1) + (term2 * term2));

        if (sin_sigma == static_cast<T>(0))
        {
            // Coincident (or antipodal-degenerate) points: zero distance, undefined bearings.
            result.distance = static_cast<T>(0);
            result.initial_bearing_deg = static_cast<T>(0);
            result.final_bearing_deg = static_cast<T>(0);
            return castle::status::ok;
        }

        cos_sigma = (sin_u1 * sin_u2) + (cos_u1 * cos_u2 * cos_lambda);
        sigma = castle::math::radians_atan2(sin_sigma, cos_sigma);

        CASTLE_CONST T sin_alpha = (cos_u1 * cos_u2 * sin_lambda) / sin_sigma;
        cos_sq_alpha = static_cast<T>(1) - (sin_alpha * sin_alpha);

        cos_2sigma_m = (cos_sq_alpha != static_cast<T>(0))
                       ? (cos_sigma - (static_cast<T>(2) * sin_u1 * sin_u2 / cos_sq_alpha))
                       : static_cast<T>(0); // Equatorial line: cos_sq_alpha == 0.

        CASTLE_CONST T big_c = (f / static_cast<T>(16)) * cos_sq_alpha
                              * (static_cast<T>(4) + (f * (static_cast<T>(4) - (static_cast<T>(3) * cos_sq_alpha))));
        CASTLE_CONST T lambda_prev = lambda;
        lambda = big_l + ((static_cast<T>(1) - big_c) * f * sin_alpha
                * (sigma + (big_c * sin_sigma
                     * (cos_2sigma_m + (big_c * cos_sigma * (static_cast<T>(-1) + (static_cast<T>(2) * cos_2sigma_m * cos_2sigma_m)))))));

        if (castle::math::abs(lambda - lambda_prev) < static_cast<T>(1e-12)) // LCOV_EXCL_BR_LINE
        {
            converged = true;
            break;
        }
    }

    // LCOV_EXCL_START
    if (!converged)
    {
        return castle::status::unknown_error;
    }
    // LCOV_EXCL_STOP

    CASTLE_CONST T u_sq = cos_sq_alpha * ((a * a) - (b * b)) / (b * b);
    CASTLE_CONST T big_a = static_cast<T>(1) + (u_sq / static_cast<T>(16384))
                         * (static_cast<T>(4096) + (u_sq * (static_cast<T>(-768) + (u_sq * (static_cast<T>(320) - (static_cast<T>(175) * u_sq))))));
    CASTLE_CONST T big_b = (u_sq / static_cast<T>(1024))
                         * (static_cast<T>(256) + (u_sq * (static_cast<T>(-128) + (u_sq * (static_cast<T>(74) - (static_cast<T>(47) * u_sq))))));
    CASTLE_CONST T delta_sigma = big_b * sin_sigma
        * (cos_2sigma_m + ((big_b / static_cast<T>(4))
            * ((cos_sigma * (static_cast<T>(-1) + (static_cast<T>(2) * cos_2sigma_m * cos_2sigma_m)))
               - ((big_b / static_cast<T>(6)) * cos_2sigma_m * (static_cast<T>(-3) + (static_cast<T>(4) * sin_sigma * sin_sigma))
                  * (static_cast<T>(-3) + (static_cast<T>(4) * cos_2sigma_m * cos_2sigma_m))))));

    result.distance = b * big_a * (sigma - delta_sigma);

    CASTLE_CONST T sin_lambda_final = castle::math::radians_sin(lambda);
    CASTLE_CONST T cos_lambda_final = castle::math::radians_cos(lambda);
    CASTLE_CONST T alpha1 = castle::math::radians_atan2(cos_u2 * sin_lambda_final,
                                                     (cos_u1 * sin_u2) - (sin_u1 * cos_u2 * cos_lambda_final));
    CASTLE_CONST T alpha2 = castle::math::radians_atan2(cos_u1 * sin_lambda_final,
                                                     (-sin_u1 * cos_u2) + (cos_u1 * sin_u2 * cos_lambda_final));

    result.initial_bearing_deg = detail::normalize_bearing(castle::math::radians_to_degrees(alpha1));
    result.final_bearing_deg = detail::normalize_bearing(castle::math::radians_to_degrees(alpha2));
    return castle::status::ok;
}

/**
 * @brief Solves the geodetic direct problem (destination point and arrival
 * bearing given a start point, bearing, and distance) on a reference
 * ellipsoid using Vincenty's iterative formula.
 * @tparam T Floating-point type.
 * @tparam Ellipsoid Ellipsoid policy type (see `datum.hpp`). Defaults to
 * `wgs84_ellipsoid<T>`.
 * @param start Starting point.
 * @param initial_bearing_deg Initial bearing at `start`, in decimal degrees
 * clockwise from true north.
 * @param distance_m Distance to travel along the geodesic, in meters.
 * @param destination Receives the computed destination point on success.
 * @param final_bearing_deg Receives the arrival bearing, in decimal degrees,
 * on success.
 * @param max_iterations Iteration budget for the sigma fixed-point loop.
 * @return `castle::status::ok` on convergence; `castle::status::unknown_error`
 * when the iteration does not converge within `max_iterations`.
 * @note The computed destination longitude is normalized into `[-180, 180)`
 * and the destination latitude is clamped into `[-90, 90]` to absorb
 * floating-point rounding before constructing `destination`.
 */
template <typename T, typename Ellipsoid = wgs84_ellipsoid<T>>
CASTLE_NODISCARD castle::status vincenty_direct(CASTLE_CONST geo_point<T>& start, T initial_bearing_deg, T distance_m,
                                                 geo_point<T>& destination, T& final_bearing_deg,
                                                 castle::size_type max_iterations = 200U) CASTLE_NOEXCEPT
{
    static_assert(castle::meta::is_floating_point<T>::value, "vincenty_direct requires a floating-point type");

    CASTLE_CONST T a = Ellipsoid::semi_major_axis();
    CASTLE_CONST T f = Ellipsoid::flattening();
    CASTLE_CONST T b = Ellipsoid::semi_minor_axis();
    CASTLE_CONST T alpha1 = castle::math::degrees_to_radians(initial_bearing_deg);

    CASTLE_CONST T u1 = castle::math::radians_atan2((static_cast<T>(1) - f) * castle::math::radians_sin(castle::math::degrees_to_radians(start.latitude())),
                                                 castle::math::radians_cos(castle::math::degrees_to_radians(start.latitude())));
    CASTLE_CONST T sigma1 = castle::math::radians_atan2(castle::math::radians_sin(u1) / castle::math::radians_cos(u1), castle::math::radians_cos(alpha1));
    CASTLE_CONST T sin_alpha = castle::math::radians_cos(u1) * castle::math::radians_sin(alpha1);
    CASTLE_CONST T cos_sq_alpha = static_cast<T>(1) - (sin_alpha * sin_alpha);
    CASTLE_CONST T u_sq = cos_sq_alpha * ((a * a) - (b * b)) / (b * b);
    CASTLE_CONST T big_a = static_cast<T>(1) + (u_sq / static_cast<T>(16384))
                         * (static_cast<T>(4096) + (u_sq * (static_cast<T>(-768) + (u_sq * (static_cast<T>(320) - (static_cast<T>(175) * u_sq))))));
    CASTLE_CONST T big_b = (u_sq / static_cast<T>(1024))
                         * (static_cast<T>(256) + (u_sq * (static_cast<T>(-128) + (u_sq * (static_cast<T>(74) - (static_cast<T>(47) * u_sq))))));

    T sigma = distance_m / (b * big_a);
    T sigma_prev = static_cast<T>(0);
    T cos_2sigma_m = static_cast<T>(0);
    T sin_sigma = static_cast<T>(0);
    T cos_sigma = static_cast<T>(0);
    bool converged = false;

    for (castle::size_type i = 0U; i < max_iterations; ++i) // LCOV_EXCL_BR_LINE
    {
        cos_2sigma_m = castle::math::radians_cos((static_cast<T>(2) * sigma1) + sigma);
        sin_sigma = castle::math::radians_sin(sigma);
        cos_sigma = castle::math::radians_cos(sigma);

        CASTLE_CONST T delta_sigma = big_b * sin_sigma
            * (cos_2sigma_m + ((big_b / static_cast<T>(4))
                * ((cos_sigma * (static_cast<T>(-1) + (static_cast<T>(2) * cos_2sigma_m * cos_2sigma_m)))
                   - ((big_b / static_cast<T>(6)) * cos_2sigma_m * (static_cast<T>(-3) + (static_cast<T>(4) * sin_sigma * sin_sigma))
                      * (static_cast<T>(-3) + (static_cast<T>(4) * cos_2sigma_m * cos_2sigma_m))))));

        sigma_prev = sigma;
        sigma = (distance_m / (b * big_a)) + delta_sigma;

        if (castle::math::abs(sigma - sigma_prev) < static_cast<T>(1e-12))
        {
            converged = true;
            break;
        }
    }

    // LCOV_EXCL_START
    if (!converged)
    {
        return castle::status::unknown_error;
    }
    // LCOV_EXCL_STOP

    CASTLE_CONST T u1_sin = castle::math::radians_sin(u1);
    CASTLE_CONST T u1_cos = castle::math::radians_cos(u1);
    CASTLE_CONST T tmp = (u1_sin * sin_sigma) - (u1_cos * cos_sigma * castle::math::radians_cos(alpha1));

    CASTLE_CONST T lat2 = castle::math::radians_atan2(
        (u1_sin * cos_sigma) + (u1_cos * sin_sigma * castle::math::radians_cos(alpha1)),
        (static_cast<T>(1) - f) * castle::math::sqrt_real((sin_alpha * sin_alpha) + (tmp * tmp)));

    CASTLE_CONST T lambda = castle::math::radians_atan2(sin_sigma * castle::math::radians_sin(alpha1),
                                                     (u1_cos * cos_sigma) - (u1_sin * sin_sigma * castle::math::radians_cos(alpha1)));
    CASTLE_CONST T big_c = (f / static_cast<T>(16)) * cos_sq_alpha
                          * (static_cast<T>(4) + (f * (static_cast<T>(4) - (static_cast<T>(3) * cos_sq_alpha))));
    CASTLE_CONST T big_l = lambda - ((static_cast<T>(1) - big_c) * f * sin_alpha
        * (sigma + (big_c * sin_sigma
             * (cos_2sigma_m + (big_c * cos_sigma * (static_cast<T>(-1) + (static_cast<T>(2) * cos_2sigma_m * cos_2sigma_m)))))));

    CASTLE_CONST T lon2 = castle::math::degrees_to_radians(start.longitude()) + big_l;
    CASTLE_CONST T alpha2 = castle::math::radians_atan2(sin_alpha, -tmp);

    CASTLE_CONST T lat2_deg = clamp_latitude(castle::math::radians_to_degrees(lat2));
    CASTLE_CONST T lon2_deg = normalize_longitude(castle::math::radians_to_degrees(lon2));

    destination = geo_point<T>(lat2_deg, lon2_deg);
    final_bearing_deg = detail::normalize_bearing(castle::math::radians_to_degrees(alpha2));
    return castle::status::ok;
}

} // namespace geodetic
} // namespace castle_ext

#endif // CASTLE_EXT_GEODETIC_DISTANCE_HPP
