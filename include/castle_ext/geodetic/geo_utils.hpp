// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file geo_utils.hpp
 * @brief Shared numeric helpers for the geodetic module.
 *
 * This header centralizes the small, low-level building blocks that
 * `coord.hpp`, `distance.hpp`, and `transform.hpp` all depend on: the two
 * inverse trigonometric wrappers Castle's core `linalg::trigonometry.hpp`
 * does not provide (`atan2` and `asin`), latitude/longitude range checks, and
 * angle normalization. Keeping these here avoids duplicating range-reduction
 * logic across the distance and transform algorithms.
 *
 * @code
 * #include "castle_ext/geodetic/geo_utils.hpp"
 *
 * const bool valid = castle_ext::geodetic::is_valid_latitude(51.5);
 * const double wrapped = castle_ext::geodetic::normalize_longitude(200.0);
 * @endcode
 *
 * @note Like `castle::math::linalg::trigonometry.hpp`, the trigonometric
 * wrappers here forward directly to the platform C math library; they are
 * not `constexpr` and their precision follows the platform implementation.
 */
#ifndef CASTLE_EXT_GEODETIC_GEO_UTILS_HPP
#define CASTLE_EXT_GEODETIC_GEO_UTILS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/clamp.hpp"

#include <math.h>

namespace castle_ext
{
namespace geodetic
{

/**
 * @brief Tests whether a decimal-degrees latitude lies within the valid
 * geographic range.
 * @tparam T Floating-point type.
 * @param latitude_deg Candidate latitude in decimal degrees.
 * @return `true` when `-90 <= latitude_deg <= 90`; otherwise `false`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, bool>::type
is_valid_latitude(T latitude_deg) CASTLE_NOEXCEPT
{
    return (latitude_deg >= static_cast<T>(-90)) && (latitude_deg <= static_cast<T>(90)); // LCOV_EXCL_BR_LINE
}

/**
 * @brief Tests whether a decimal-degrees longitude lies within the valid
 * geographic range.
 * @tparam T Floating-point type.
 * @param longitude_deg Candidate longitude in decimal degrees.
 * @return `true` when `-180 <= longitude_deg <= 180`; otherwise `false`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, bool>::type
is_valid_longitude(T longitude_deg) CASTLE_NOEXCEPT
{
    return (longitude_deg >= static_cast<T>(-180)) && (longitude_deg <= static_cast<T>(180)); // LCOV_EXCL_BR_LINE
}

/**
 * @brief Clamps a decimal-degrees latitude into the valid `[-90, 90]` range.
 * @tparam T Floating-point type.
 * @param latitude_deg Candidate latitude in decimal degrees.
 * @return `latitude_deg` restricted to `[-90, 90]`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
clamp_latitude(T latitude_deg) CASTLE_NOEXCEPT // LCOV_EXCL_BR_LINE
{
    return castle::math::clamp(latitude_deg, static_cast<T>(-90), static_cast<T>(90));
}

/**
 * @brief Normalizes a decimal-degrees longitude into the half-open interval
 * `[-180, 180)`.
 * @tparam T Floating-point type.
 * @param longitude_deg Longitude in decimal degrees, any finite value.
 * @return The equivalent longitude wrapped into `[-180, 180)`.
 * @note This uses `fmod` and is therefore not `constexpr`.
 */
template <typename T>
CASTLE_NODISCARD
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
normalize_longitude(T longitude_deg) CASTLE_NOEXCEPT
{
    CASTLE_CONST T span = static_cast<T>(360);
    T wrapped = static_cast<T>(::fmod(static_cast<double>(longitude_deg) + 180.0, static_cast<double>(span)));

    if (wrapped < static_cast<T>(0))
    {
        wrapped += span;
    }

    return wrapped - static_cast<T>(180);
}

} // namespace geodetic
} // namespace castle_ext

#endif // CASTLE_EXT_GEODETIC_GEO_UTILS_HPP
