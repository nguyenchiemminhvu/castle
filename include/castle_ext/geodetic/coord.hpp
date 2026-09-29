// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file coord.hpp
 * @brief Geographic coordinate representation and angle-format conversions.
 *
 * This header provides:
 *  - `geo_point<T>`, a validated latitude/longitude pair used throughout the
 *    geodetic module (by `distance.hpp` and `transform.hpp`);
 *  - `dms<T>` / `ddm<T>`, structured Degrees-Minutes-Seconds and
 *    Degrees-Decimal-Minutes representations with conversions to and from
 *    decimal degrees;
 *  - free-function conversions between decimal degrees and arcseconds,
 *    milliarcseconds, microarcseconds, gradians, and turns; and
 *  - an optional NATO/GEOREF-style DMS text formatter built on
 *    `castle::basic_string_builder`.
 *
 * Every conversion here is pure arithmetic, so all of it is available at
 * compile time via `constexpr`. Radian conversions are intentionally not
 * duplicated: use `castle::math::degrees_to_radians` /
 * `castle::math::radians_to_degrees` from `castle/core/constants.hpp`.
 *
 * @code
 * #include "castle_ext/geodetic/coord.hpp"
 *
 * constexpr castle_ext::geodetic::geo_point<double> hanoi(21.0278, 105.8342);
 * constexpr castle_ext::geodetic::dms<double> lat_dms =
 *     castle_ext::geodetic::degrees_to_dms(hanoi.latitude());
 * @endcode
 */
#ifndef CASTLE_EXT_GEODETIC_COORD_HPP
#define CASTLE_EXT_GEODETIC_COORD_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/math/math.hpp"

#include "castle/utility/string_builder.hpp"
#include "castle_ext/geodetic/geo_utils.hpp"

#include <stdint.h>

namespace castle_ext
{
namespace geodetic
{

/**
 * @brief Stores a validated geographic position as decimal-degree latitude
 * and longitude.
 * @tparam T Floating-point coordinate type.
 * @note This type is intentionally distinct from `castle::math::point2d` so
 * that latitude/longitude semantics (and their valid ranges) stay explicit,
 * matching the point/vector separation already used by Castle's geometry
 * module.
 */
template <typename T>
class geo_point
{
    static_assert(castle::meta::is_floating_point<T>::value,
                  "geo_point requires a floating-point type");

public:
    using value_type = T;

    /** @brief Constructs the point at (0, 0). */
    CASTLE_CONSTEXPR geo_point() CASTLE_NOEXCEPT : latitude_(T{}), longitude_(T{}) {}

    /**
     * @brief Constructs a geographic point from decimal-degree coordinates.
     * @param latitude_deg Latitude in decimal degrees, expected in `[-90, 90]`.
     * @param longitude_deg Longitude in decimal degrees, expected in `[-180, 180]`.
     * @warning Out-of-range inputs violate the precondition and trigger
     * Castle's error handling when checks are enabled; the value is stored
     * unchanged either way.
     */
    CASTLE_CONSTEXPR geo_point(T latitude_deg, T longitude_deg) CASTLE_NOEXCEPT
        : latitude_(latitude_deg), longitude_(longitude_deg)
    {
        CASTLE_ASSERT(is_valid_latitude(latitude_deg), // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle_ext::geodetic::geo_point: latitude out of range"));
        CASTLE_ASSERT(is_valid_longitude(longitude_deg), // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle_ext::geodetic::geo_point: longitude out of range"));
    }

    /** @brief Returns the stored latitude in decimal degrees. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T latitude() CASTLE_CONST CASTLE_NOEXCEPT { return latitude_; }
    /** @brief Returns the stored longitude in decimal degrees. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T longitude() CASTLE_CONST CASTLE_NOEXCEPT { return longitude_; }

    /**
     * @brief Compares two points component-wise.
     * @param other Point to compare with.
     * @return `true` when both coordinates compare equal.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(CASTLE_CONST geo_point& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::math::is_equal(latitude_, other.latitude_) // LCOV_EXCL_BR_LINE
            && castle::math::is_equal(longitude_, other.longitude_); // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Compares two points component-wise.
     * @param other Point to compare with.
     * @return `true` when any coordinate differs.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST geo_point& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    T latitude_;
    T longitude_;
};

/**
 * @brief Degrees-Minutes-Seconds representation of an angle.
 * @tparam T Floating-point type used for the seconds field.
 * @note The sign of the original angle is carried in `negative` rather than
 * in `degrees`/`minutes`/`seconds`, so every field can use an unsigned or
 * always-positive representation, matching the conventional DMS notation
 * used with N/S/E/W hemisphere letters.
 */
template <typename T>
struct dms
{
    /** @brief `true` when the original decimal-degree angle was negative. */
    bool negative;
    /** @brief Whole degrees component, always non-negative. */
    uint32_t degrees;
    /** @brief Whole arcminutes component, in `[0, 60)`. */
    uint32_t minutes;
    /** @brief Arcseconds component, in `[0, 60)`. */
    T seconds;
};

/**
 * @brief Degrees and Decimal-Minutes representation of an angle.
 * @tparam T Floating-point type used for the minutes field.
 */
template <typename T>
struct ddm
{
    /** @brief `true` when the original decimal-degree angle was negative. */
    bool negative;
    /** @brief Whole degrees component, always non-negative. */
    uint32_t degrees;
    /** @brief Decimal arcminutes component, in `[0, 60)`. */
    T minutes;
};

/**
 * @brief Converts a decimal-degree angle to Degrees-Minutes-Seconds.
 * @tparam T Floating-point type.
 * @param decimal_degrees Angle in decimal degrees.
 * @return The equivalent `dms<T>` value.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, dms<T>>::type
degrees_to_dms(T decimal_degrees) CASTLE_NOEXCEPT
{
    CASTLE_CONST bool negative = decimal_degrees < static_cast<T>(0);
    CASTLE_CONST T magnitude = castle::math::abs<T>(decimal_degrees);
    CASTLE_CONST uint32_t whole_degrees = static_cast<uint32_t>(magnitude);
    CASTLE_CONST T minutes_total = (magnitude - static_cast<T>(whole_degrees)) * static_cast<T>(60);
    CASTLE_CONST uint32_t whole_minutes = static_cast<uint32_t>(minutes_total);
    CASTLE_CONST T seconds = (minutes_total - static_cast<T>(whole_minutes)) * static_cast<T>(60);

    return dms<T>{negative, whole_degrees, whole_minutes, seconds};
}

/**
 * @brief Converts a Degrees-Minutes-Seconds value to decimal degrees.
 * @tparam T Floating-point type.
 * @param value DMS value to convert.
 * @return The equivalent decimal-degree angle.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
dms_to_degrees(CASTLE_CONST dms<T>& value) CASTLE_NOEXCEPT
{
    CASTLE_CONST T magnitude = static_cast<T>(value.degrees)
                       + (static_cast<T>(value.minutes) / static_cast<T>(60))
                       + (value.seconds / static_cast<T>(3600));
    return value.negative ? -magnitude : magnitude;
}

/**
 * @brief Converts a decimal-degree angle to Degrees and Decimal-Minutes.
 * @tparam T Floating-point type.
 * @param decimal_degrees Angle in decimal degrees.
 * @return The equivalent `ddm<T>` value.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, ddm<T>>::type
degrees_to_ddm(T decimal_degrees) CASTLE_NOEXCEPT
{
    CASTLE_CONST bool negative = decimal_degrees < static_cast<T>(0);
    CASTLE_CONST T magnitude = castle::math::abs<T>(decimal_degrees);
    CASTLE_CONST uint32_t whole_degrees = static_cast<uint32_t>(magnitude);
    CASTLE_CONST T decimal_minutes = (magnitude - static_cast<T>(whole_degrees)) * static_cast<T>(60);

    return ddm<T>{negative, whole_degrees, decimal_minutes};
}

/**
 * @brief Converts a Degrees and Decimal-Minutes value to decimal degrees.
 * @tparam T Floating-point type.
 * @param value DDM value to convert.
 * @return The equivalent decimal-degree angle.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
ddm_to_degrees(CASTLE_CONST ddm<T>& value) CASTLE_NOEXCEPT
{
    CASTLE_CONST T magnitude = static_cast<T>(value.degrees) + (value.minutes / static_cast<T>(60));
    return value.negative ? -magnitude : magnitude;
}

/// @name Angle unit conversions
/// @brief Conversions between decimal degrees and other common angle units.
/// @note Degrees/radians conversions live in `castle::math` (`degrees_to_radians`,
/// `radians_to_degrees`) and are intentionally not duplicated here.
/// @{

/** @brief Converts decimal degrees to arcseconds. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
degrees_to_arcseconds(T decimal_degrees) CASTLE_NOEXCEPT
{
    return decimal_degrees * static_cast<T>(3600);
}

/** @brief Converts arcseconds to decimal degrees. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
arcseconds_to_degrees(T arcseconds) CASTLE_NOEXCEPT
{
    return arcseconds / static_cast<T>(3600);
}

/** @brief Converts decimal degrees to milliarcseconds. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
degrees_to_milliarcseconds(T decimal_degrees) CASTLE_NOEXCEPT
{
    return decimal_degrees * static_cast<T>(3600000);
}

/** @brief Converts milliarcseconds to decimal degrees. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
milliarcseconds_to_degrees(T milliarcseconds) CASTLE_NOEXCEPT
{
    return milliarcseconds / static_cast<T>(3600000);
}

/** @brief Converts decimal degrees to microarcseconds. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
degrees_to_microarcseconds(T decimal_degrees) CASTLE_NOEXCEPT
{
    return decimal_degrees * static_cast<T>(3600000000);
}

/** @brief Converts microarcseconds to decimal degrees. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
microarcseconds_to_degrees(T microarcseconds) CASTLE_NOEXCEPT
{
    return microarcseconds / static_cast<T>(3600000000);
}

/** @brief Converts decimal degrees to gradians (gon). */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
degrees_to_gradians(T decimal_degrees) CASTLE_NOEXCEPT
{
    return decimal_degrees * (static_cast<T>(400) / static_cast<T>(360));
}

/** @brief Converts gradians (gon) to decimal degrees. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
gradians_to_degrees(T gradians) CASTLE_NOEXCEPT
{
    return gradians * (static_cast<T>(360) / static_cast<T>(400));
}

/** @brief Converts decimal degrees to turns (full rotations). */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
degrees_to_turns(T decimal_degrees) CASTLE_NOEXCEPT
{
    return decimal_degrees / static_cast<T>(360);
}

/** @brief Converts turns (full rotations) to decimal degrees. */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename castle::meta::enable_if<castle::meta::is_floating_point<T>::value, T>::type
turns_to_degrees(T turns) CASTLE_NOEXCEPT
{
    return turns * static_cast<T>(360);
}

/// @}

/**
 * @brief Appends a NATO/GEOREF-style DMS string (e.g. `51 28'39.383"N`) to a
 * Castle string builder.
 * @tparam BuilderT A `castle::basic_string_builder<...>` instantiation.
 * @tparam T Floating-point type of the DMS value.
 * @param builder Builder receiving the formatted text.
 * @param value DMS value to format; its sign selects the hemisphere symbol.
 * @param positive_symbol Symbol appended when `value.negative == false`
 * (e.g. `'N'` for latitude, `'E'` for longitude).
 * @param negative_symbol Symbol appended when `value.negative == true`
 * (e.g. `'S'` for latitude, `'W'` for longitude).
 * @param precision Number of decimal digits used for the seconds field.
 * @return `builder`, for chaining.
 * @note Overflow is handled by the builder itself (truncation with `...`),
 * matching every other `castle::basic_string_builder` consumer.
 */
template <typename BuilderT, typename T>
BuilderT& append_dms(BuilderT& builder, CASTLE_CONST dms<T>& value,
                      char positive_symbol, char negative_symbol,
                      castle::size_type precision = 3U) CASTLE_NOEXCEPT
{
    builder.append(static_cast<uint32_t>(value.degrees));
    builder.append(' ');
    builder.append(static_cast<uint32_t>(value.minutes));
    builder.append('\'');
    builder.append(value.seconds, precision);
    builder.append('"');
    builder.append(value.negative ? negative_symbol : positive_symbol);
    return builder;
}

/**
 * @brief Appends a NATO/GEOREF-style DMS string for latitude (N/S) to a Castle string builder.
 * @tparam BuilderT A `castle::basic_string_builder<...>` instantiation.
 * @tparam T Floating-point type of the DMS value.
 * @param builder Builder receiving the formatted text.
 * @param value DMS value representing the latitude.
 * @param negative Whether the latitude is negative (south). This is typically derived from the `negative` field of the `dms` value.
 * @param precision Number of decimal digits used for the seconds field.
 * @return `builder`, for chaining.
 */
template <typename BuilderT, typename T>
BuilderT& append_dms_lat(BuilderT& builder, CASTLE_CONST dms<T>& value,
                         castle::size_type precision = 3U) CASTLE_NOEXCEPT
{
    return append_dms(builder, value, 'N', 'S', precision);
}

/**
 * @brief Appends a NATO/GEOREF-style DMS string for longitude (E/W) to a Castle string builder.
 * @tparam BuilderT A `castle::basic_string_builder<...>` instantiation.
 * @tparam T Floating-point type of the DMS value.
 * @param builder Builder receiving the formatted text.
 * @param value DMS value representing the longitude.
 * @param negative Whether the longitude is negative (west). This is typically derived from the `negative` field of the `dms` value.
 * @param precision Number of decimal digits used for the seconds field.
 * @return `builder`, for chaining.
 */
template <typename BuilderT, typename T>
BuilderT& append_dms_lon(BuilderT& builder, CASTLE_CONST dms<T>& value,
                         castle::size_type precision = 3U) CASTLE_NOEXCEPT
{
    return append_dms(builder, value, 'E', 'W', precision);
}

} // namespace geodetic
} // namespace castle_ext

#endif // CASTLE_EXT_GEODETIC_COORD_HPP
