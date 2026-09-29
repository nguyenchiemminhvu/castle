// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nmea_generator.hpp
 * @brief Header-only NMEA 0183 sentence generator for Castle.
 *
 * This module depends only on `castle_ext/protocols/nmea/nmea.hpp` (not on
 * the parser) and reuses its checksum/delimiter primitives instead of
 * re-implementing them. It provides:
 *  - plain input structs (`gga_input`, `rmc_input`, `gsa_input`, `gsv_input`,
 *    `vtg_input`, `gll_input`, `zda_input`) describing one sentence's fields;
 *  - one `generate_xxx()` free function per sentence type, each building a
 *    complete `$talker+type,fields...*HH\r\n` sentence into a caller-provided
 *    buffer.
 *
 * Fields are assembled with `castle::string_builder` into a bounded scratch
 * buffer (sized by the `MaxSentenceLen` template parameter) rather than via
 * `encode_sentence()`, because several fields are conditionally empty
 * (e.g. HDOP, DGPS age) which does not fit `encode_sentence()`'s fixed
 * field-list shape. The checksum itself still reuses `compute_checksum()`.
 *
 * No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers
 * are used.
 *
 * @code
 * #include "castle_ext/protocols/nmea/nmea_generator.hpp"
 *
 * using namespace castle_ext::protocols::nmea::generator;
 *
 * gga_input input;
 * input.hour = 9; input.minute = 27; input.second = 25;
 * input.latitude_deg = 47.2852333; input.longitude_deg = 8.5652650;
 * input.fix_quality = 1; input.num_satellites = 8;
 * input.altitude_msl_m = 499.6; input.geoid_sep_m = 48.0;
 * input.valid = true;
 *
 * char sentence[96U];
 * castle::size_type written = 0U;
 * generate_gga(input, castle::container::string_view("GP"), sentence, sizeof(sentence), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_NMEA_NMEA_GENERATOR_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_NMEA_GENERATOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/array_view.hpp"
#include "castle/container/string_view.hpp"
#include "castle/utility/string_builder.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"

#include <stdint.h>

namespace castle_ext
{
namespace protocols
{
namespace nmea
{
namespace generator
{

/**
 * @brief Default scratch-buffer capacity used while assembling one sentence.
 */
static CASTLE_CONSTEXPR castle::size_type NMEA_GENERATOR_MAX_SENTENCE_LEN = NMEA_SAFE_MAX_SENTENCE_LEN;

/**
 * @brief Maximum satellites carried by one GSV sentence (NMEA 0183 v4.11).
 */
static CASTLE_CONSTEXPR castle::size_type NMEA_GSV_SATS_PER_MESSAGE = 4U;

// -----------------------------------------------------------------------------
// Input structs (one per supported sentence type)
// -----------------------------------------------------------------------------

/**
 * @brief Input for a `$--GGA` sentence (Global Positioning System Fix Data).
 */
struct gga_input
{
    uint8_t hour = 0U;
    uint8_t minute = 0U;
    uint8_t second = 0U;
    uint32_t millisecond = 0U;

    double latitude_deg = 0.0;  ///< Signed: positive = North.
    double longitude_deg = 0.0; ///< Signed: positive = East.

    uint8_t fix_quality = 0U; ///< 0=invalid 1=GPS 2=DGPS 4=RTK-fix 5=RTK-float 6=DR.
    uint8_t num_satellites = 0U;
    double hdop = 0.0;           ///< 0.0 leaves the field empty (unavailable).
    double altitude_msl_m = 0.0; ///< Metres above mean sea level.
    double geoid_sep_m = 0.0;    ///< Geoid separation [m].

    double dgps_age_s = -1.0;      ///< Age of DGPS correction [s]; < 0 = not available.
    uint16_t dgps_station_id = 0U; ///< Used only when `dgps_age_s >= 0`.

    bool valid = false;
};

/**
 * @brief Input for a `$--RMC` sentence (Recommended Minimum Specific GNSS Data).
 */
struct rmc_input
{
    uint8_t hour = 0U;
    uint8_t minute = 0U;
    uint8_t second = 0U;
    uint32_t millisecond = 0U;

    bool status_active = false; ///< true = 'A' (active/valid), false = 'V' (void).

    double latitude_deg = 0.0;
    double longitude_deg = 0.0;

    double speed_knots = 0.0; ///< Speed over ground [knots].
    double course_true = 0.0; ///< Course over ground, true [degrees].

    uint8_t day = 0U;
    uint8_t month = 0U;
    uint16_t year = 0U;

    double mag_variation = 0.0; ///< Magnetic variation [degrees]; positive = East.
    bool mag_var_valid = false; ///< false leaves the magnetic-variation fields empty.

    char mode = 'N'; ///< A/D/E/N/S (NMEA 2.3+).

    bool valid = false;
};

/**
 * @brief Input for a `$--GSA` sentence (GNSS DOP and Active Satellites).
 */
struct gsa_input
{
    char op_mode = 'A';  ///< 'A' = auto 2D/3D, 'M' = manual.
    uint8_t nav_mode = 1U; ///< 1=no fix, 2=2D, 3=3D.

    /// Satellite IDs used in the navigation solution (0 = unused slot).
    uint8_t sat_ids[12U] = {};

    double pdop = 0.0;
    double hdop = 0.0;
    double vdop = 0.0;

    uint8_t system_id = 0U; ///< NMEA 4.11 system ID; 0 omits the field.

    bool valid = false;
};

/**
 * @brief One satellite entry within a `gsv_input`.
 */
struct gsv_satellite_record
{
    uint8_t sv_id = 0U;
    int8_t elevation_deg = -1; ///< < 0 leaves the field empty (not available).
    int16_t azimuth_deg = 0;
    uint8_t snr = 0U; ///< dBHz; 0 leaves the field empty (not tracking).
};

/**
 * @brief Input for one constellation's `$--GSV` sentence set.
 *
 * `generate_gsv_message()` emits one of `gsv_message_count()` messages per
 * call; the caller loops over message numbers to emit the full set.
 */
struct gsv_input
{
    castle::container::array_view<CASTLE_CONST gsv_satellite_record> satellites{};
    bool valid = false;
};

/**
 * @brief Input for a `$--VTG` sentence (Course Over Ground and Ground Speed).
 */
struct vtg_input
{
    double course_true = 0.0; ///< Course over ground, true north [degrees].
    double course_mag = 0.0;  ///< Course over ground, magnetic [degrees].
    bool course_mag_valid = false;

    double speed_knots = 0.0;
    double speed_kmh = 0.0;

    char mode = 'N'; ///< A/D/E/N/S.

    bool valid = false;
};

/**
 * @brief Input for a `$--GLL` sentence (Geographic Position, Latitude/Longitude).
 */
struct gll_input
{
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;

    uint8_t hour = 0U;
    uint8_t minute = 0U;
    uint8_t second = 0U;
    uint32_t millisecond = 0U;

    bool status_active = false;
    char mode = 'N'; ///< A/D/E/N/S.

    bool valid = false;
};

/**
 * @brief Input for a `$--ZDA` sentence (Time and Date).
 */
struct zda_input
{
    uint8_t hour = 0U;
    uint8_t minute = 0U;
    uint8_t second = 0U;
    uint32_t millisecond = 0U;

    uint8_t day = 0U;
    uint8_t month = 0U;
    uint16_t year = 0U;

    int8_t tz_hour = 0;  ///< Local zone offset hours [-13..+13]; 0 for UTC.
    uint8_t tz_min = 0U; ///< Local zone offset minutes.

    bool valid = false;
};

// -----------------------------------------------------------------------------
// Field-formatting helpers
// -----------------------------------------------------------------------------

/**
 * @brief Appends `value` zero-padded to at least `width` digits.
 */
template <typename Builder>
inline void append_padded(Builder& builder, uint32_t value, castle::size_type width) CASTLE_NOEXCEPT
{
    uint32_t divisor = 1U;
    for (castle::size_type i = 1U; i < width; ++i)
    {
        divisor *= 10U;
    }
    while ((divisor > 1U) && (value < divisor))
    {
        builder.append('0');
        divisor /= 10U;
    }
    builder.append(value);
}

/**
 * @brief Appends a UTC time field as `hhmmss` plus a fractional-second suffix.
 * @param decimal_digits Number of fractional digits (1 or 2; NMEA 0183 v4.11 uses 2).
 */
template <typename Builder>
inline void append_utc_time(
    Builder& builder, uint8_t hour, uint8_t minute, uint8_t second,
    uint32_t millisecond, castle::size_type decimal_digits = 2U) CASTLE_NOEXCEPT
{
    append_padded(builder, hour, 2U);
    append_padded(builder, minute, 2U);
    append_padded(builder, second, 2U);
    builder.append('.');
    CASTLE_CONST uint32_t scale = (decimal_digits >= 2U) ? 100U : 10U;
    CASTLE_CONST uint32_t divisor = 1000U / scale;
    append_padded(builder, (millisecond / divisor) % scale, decimal_digits >= 2U ? 2U : 1U);
}

/**
 * @brief Appends a date field as `ddmmyy`.
 */
template <typename Builder>
inline void append_date(Builder& builder, uint8_t day, uint8_t month, uint16_t year) CASTLE_NOEXCEPT
{
    append_padded(builder, day, 2U);
    append_padded(builder, month, 2U);
    append_padded(builder, static_cast<uint32_t>(year % 100U), 2U);
}

/**
 * @brief Appends a latitude magnitude as `DDmm.mmmmm` (no N/S indicator).
 */
template <typename Builder>
inline void append_latitude(Builder& builder, double latitude_deg, castle::size_type decimals = 5U) CASTLE_NOEXCEPT
{
    CASTLE_CONST double magnitude = (latitude_deg < 0.0) ? -latitude_deg : latitude_deg;
    CASTLE_CONST double degrees = static_cast<double>(static_cast<uint32_t>(magnitude));
    CASTLE_CONST double minutes = (magnitude - degrees) * 60.0;

    append_padded(builder, static_cast<uint32_t>(degrees), 2U);
    if (minutes < 10.0)
    {
        builder.append('0');
    }
    builder.append(minutes, decimals);
}

/**
 * @brief Appends a longitude magnitude as `DDDmm.mmmmm` (no E/W indicator).
 */
template <typename Builder>
inline void append_longitude(Builder& builder, double longitude_deg, castle::size_type decimals = 5U) CASTLE_NOEXCEPT
{
    CASTLE_CONST double magnitude = (longitude_deg < 0.0) ? -longitude_deg : longitude_deg;
    CASTLE_CONST double degrees = static_cast<double>(static_cast<uint32_t>(magnitude));
    CASTLE_CONST double minutes = (magnitude - degrees) * 60.0;

    append_padded(builder, static_cast<uint32_t>(degrees), 3U);
    if (minutes < 10.0)
    {
        builder.append('0');
    }
    builder.append(minutes, decimals);
}

/**
 * @brief Returns 'N' for a non-negative latitude, otherwise 'S'.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR char ns_indicator(double latitude_deg) CASTLE_NOEXCEPT
{
    return (latitude_deg >= 0.0) ? 'N' : 'S';
}

/**
 * @brief Returns 'E' for a non-negative longitude, otherwise 'W'.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR char ew_indicator(double longitude_deg) CASTLE_NOEXCEPT
{
    return (longitude_deg >= 0.0) ? 'E' : 'W';
}

/**
 * @brief Computes the checksum over a builder's content after the leading '$',
 * appends `*HH\r\n`, then copies the result into the caller's output buffer.
 */
template <typename Builder>
CASTLE_NODISCARD inline castle::status finalize_sentence(
    Builder& builder, char* output, castle::size_type output_capacity, castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    CASTLE_CONST uint8_t check = compute_checksum(builder.view().substr(1U));

    builder.append(NMEA_CHECKSUM_CHAR);
    char hex[2U];
    write_hex_byte(check, hex);
    builder.append(hex[0U]);
    builder.append(hex[1U]);
    builder.append(NMEA_CR);
    builder.append(NMEA_LF);

    if (builder.truncated() || (builder.size() > output_capacity))
    {
        bytes_written = 0U;
        return castle::status::full;
    }

    CASTLE_CONST castle::container::string_view content = builder.view();
    for (castle::size_type index = 0U; index < content.size(); ++index)
    {
        output[index] = content[index];
    }
    bytes_written = content.size();
    return castle::status::ok;
}

// -----------------------------------------------------------------------------
// Sentence generators
// -----------------------------------------------------------------------------

/**
 * @brief Builds a complete `$--GGA` sentence into a caller-provided buffer.
 * @return `castle::status::invalid_argument` when `!input.valid` or `output == nullptr`;
 * `castle::status::full` when the sentence does not fit; otherwise `castle::status::ok`.
 */
template <castle::size_type MaxSentenceLen = NMEA_GENERATOR_MAX_SENTENCE_LEN>
CASTLE_NODISCARD inline castle::status generate_gga(
    CASTLE_CONST gga_input& input, castle::container::string_view talker,
    char* output, castle::size_type output_capacity, castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;
    if (!input.valid || (output == nullptr))
    {
        return castle::status::invalid_argument;
    }

    castle::string_builder<MaxSentenceLen> builder;
    builder.append(NMEA_START_CHAR);
    builder.append(talker);
    builder.append(SENTENCE_GGA);
    builder.append(NMEA_FIELD_SEP);

    append_utc_time(builder, input.hour, input.minute, input.second, input.millisecond);
    builder.append(NMEA_FIELD_SEP);

    append_latitude(builder, input.latitude_deg);
    builder.append(NMEA_FIELD_SEP);
    builder.append(ns_indicator(input.latitude_deg));
    builder.append(NMEA_FIELD_SEP);

    append_longitude(builder, input.longitude_deg);
    builder.append(NMEA_FIELD_SEP);
    builder.append(ew_indicator(input.longitude_deg));
    builder.append(NMEA_FIELD_SEP);

    builder.append(static_cast<uint32_t>(input.fix_quality));
    builder.append(NMEA_FIELD_SEP);

    append_padded(builder, static_cast<uint32_t>(input.num_satellites), 2U);
    builder.append(NMEA_FIELD_SEP);

    if (input.hdop > 0.0)
    {
        builder.append(input.hdop, 1U);
    }
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.altitude_msl_m, 1U);
    builder.append(NMEA_FIELD_SEP);
    builder.append('M');
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.geoid_sep_m, 1U);
    builder.append(NMEA_FIELD_SEP);
    builder.append('M');
    builder.append(NMEA_FIELD_SEP);

    if (input.dgps_age_s >= 0.0)
    {
        builder.append(input.dgps_age_s, 1U);
    }
    builder.append(NMEA_FIELD_SEP);

    if (input.dgps_age_s >= 0.0)
    {
        builder.append(static_cast<uint32_t>(input.dgps_station_id));
    }

    return finalize_sentence(builder, output, output_capacity, bytes_written);
}

/**
 * @brief Builds a complete `$--RMC` sentence into a caller-provided buffer.
 * @return See `generate_gga()`.
 */
template <castle::size_type MaxSentenceLen = NMEA_GENERATOR_MAX_SENTENCE_LEN>
CASTLE_NODISCARD inline castle::status generate_rmc(
    CASTLE_CONST rmc_input& input, castle::container::string_view talker,
    char* output, castle::size_type output_capacity, castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;
    if (!input.valid || (output == nullptr))
    {
        return castle::status::invalid_argument;
    }

    castle::string_builder<MaxSentenceLen> builder;
    builder.append(NMEA_START_CHAR);
    builder.append(talker);
    builder.append(SENTENCE_RMC);
    builder.append(NMEA_FIELD_SEP);

    append_utc_time(builder, input.hour, input.minute, input.second, input.millisecond);
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.status_active ? 'A' : 'V');
    builder.append(NMEA_FIELD_SEP);

    append_latitude(builder, input.latitude_deg);
    builder.append(NMEA_FIELD_SEP);
    builder.append(ns_indicator(input.latitude_deg));
    builder.append(NMEA_FIELD_SEP);

    append_longitude(builder, input.longitude_deg);
    builder.append(NMEA_FIELD_SEP);
    builder.append(ew_indicator(input.longitude_deg));
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.speed_knots, 2U);
    builder.append(NMEA_FIELD_SEP);
    builder.append(input.course_true, 2U);
    builder.append(NMEA_FIELD_SEP);

    append_date(builder, input.day, input.month, input.year);
    builder.append(NMEA_FIELD_SEP);

    if (input.mag_var_valid)
    {
        builder.append(input.mag_variation < 0.0 ? -input.mag_variation : input.mag_variation, 1U);
        builder.append(NMEA_FIELD_SEP);
        builder.append(input.mag_variation >= 0.0 ? 'E' : 'W');
    }
    else
    {
        builder.append(NMEA_FIELD_SEP);
    }
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.mode);

    return finalize_sentence(builder, output, output_capacity, bytes_written);
}

/**
 * @brief Builds a complete `$--GSA` sentence into a caller-provided buffer.
 * @return See `generate_gga()`.
 */
template <castle::size_type MaxSentenceLen = NMEA_GENERATOR_MAX_SENTENCE_LEN>
CASTLE_NODISCARD inline castle::status generate_gsa(
    CASTLE_CONST gsa_input& input, castle::container::string_view talker,
    char* output, castle::size_type output_capacity, castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;
    if (!input.valid || (output == nullptr))
    {
        return castle::status::invalid_argument;
    }

    castle::string_builder<MaxSentenceLen> builder;
    builder.append(NMEA_START_CHAR);
    builder.append(talker);
    builder.append(SENTENCE_GSA);
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.op_mode);
    builder.append(NMEA_FIELD_SEP);
    builder.append(static_cast<uint32_t>(input.nav_mode));
    builder.append(NMEA_FIELD_SEP);

    static CASTLE_CONSTEXPR castle::size_type sat_id_count = 12U;
    for (castle::size_type index = 0U; index < sat_id_count; ++index)
    {
        if (input.sat_ids[index] != 0U)
        {
            append_padded(builder, static_cast<uint32_t>(input.sat_ids[index]), 2U);
        }
        builder.append(NMEA_FIELD_SEP);
    }

    if (input.pdop > 0.0)
    {
        builder.append(input.pdop, 2U);
    }
    builder.append(NMEA_FIELD_SEP);

    if (input.hdop > 0.0)
    {
        builder.append(input.hdop, 2U);
    }
    builder.append(NMEA_FIELD_SEP);

    if (input.vdop > 0.0)
    {
        builder.append(input.vdop, 2U);
    }

    if (input.system_id != 0U)
    {
        builder.append(NMEA_FIELD_SEP);
        builder.append(static_cast<uint32_t>(input.system_id));
    }

    return finalize_sentence(builder, output, output_capacity, bytes_written);
}

/**
 * @brief Builds a complete `$--VTG` sentence into a caller-provided buffer.
 * @return See `generate_gga()`.
 */
template <castle::size_type MaxSentenceLen = NMEA_GENERATOR_MAX_SENTENCE_LEN>
CASTLE_NODISCARD inline castle::status generate_vtg(
    CASTLE_CONST vtg_input& input, castle::container::string_view talker,
    char* output, castle::size_type output_capacity, castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;
    if (!input.valid || (output == nullptr))
    {
        return castle::status::invalid_argument;
    }

    castle::string_builder<MaxSentenceLen> builder;
    builder.append(NMEA_START_CHAR);
    builder.append(talker);
    builder.append(SENTENCE_VTG);
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.course_true, 2U);
    builder.append(NMEA_FIELD_SEP);
    builder.append('T');
    builder.append(NMEA_FIELD_SEP);

    if (input.course_mag_valid)
    {
        builder.append(input.course_mag, 2U);
    }
    builder.append(NMEA_FIELD_SEP);
    builder.append('M');
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.speed_knots, 2U);
    builder.append(NMEA_FIELD_SEP);
    builder.append('N');
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.speed_kmh, 2U);
    builder.append(NMEA_FIELD_SEP);
    builder.append('K');
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.mode);

    return finalize_sentence(builder, output, output_capacity, bytes_written);
}

/**
 * @brief Builds a complete `$--GLL` sentence into a caller-provided buffer.
 * @return See `generate_gga()`.
 */
template <castle::size_type MaxSentenceLen = NMEA_GENERATOR_MAX_SENTENCE_LEN>
CASTLE_NODISCARD inline castle::status generate_gll(
    CASTLE_CONST gll_input& input, castle::container::string_view talker,
    char* output, castle::size_type output_capacity, castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;
    if (!input.valid || (output == nullptr))
    {
        return castle::status::invalid_argument;
    }

    castle::string_builder<MaxSentenceLen> builder;
    builder.append(NMEA_START_CHAR);
    builder.append(talker);
    builder.append(SENTENCE_GLL);
    builder.append(NMEA_FIELD_SEP);

    append_latitude(builder, input.latitude_deg);
    builder.append(NMEA_FIELD_SEP);
    builder.append(ns_indicator(input.latitude_deg));
    builder.append(NMEA_FIELD_SEP);

    append_longitude(builder, input.longitude_deg);
    builder.append(NMEA_FIELD_SEP);
    builder.append(ew_indicator(input.longitude_deg));
    builder.append(NMEA_FIELD_SEP);

    append_utc_time(builder, input.hour, input.minute, input.second, input.millisecond);
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.status_active ? 'A' : 'V');
    builder.append(NMEA_FIELD_SEP);

    builder.append(input.mode);

    return finalize_sentence(builder, output, output_capacity, bytes_written);
}

/**
 * @brief Builds a complete `$--ZDA` sentence into a caller-provided buffer.
 * @return See `generate_gga()`.
 */
template <castle::size_type MaxSentenceLen = NMEA_GENERATOR_MAX_SENTENCE_LEN>
CASTLE_NODISCARD inline castle::status generate_zda(
    CASTLE_CONST zda_input& input, castle::container::string_view talker,
    char* output, castle::size_type output_capacity, castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;
    if (!input.valid || (output == nullptr))
    {
        return castle::status::invalid_argument;
    }

    castle::string_builder<MaxSentenceLen> builder;
    builder.append(NMEA_START_CHAR);
    builder.append(talker);
    builder.append(SENTENCE_ZDA);
    builder.append(NMEA_FIELD_SEP);

    append_utc_time(builder, input.hour, input.minute, input.second, input.millisecond);
    builder.append(NMEA_FIELD_SEP);

    append_padded(builder, static_cast<uint32_t>(input.day), 2U);
    builder.append(NMEA_FIELD_SEP);
    append_padded(builder, static_cast<uint32_t>(input.month), 2U);
    builder.append(NMEA_FIELD_SEP);
    builder.append(static_cast<uint32_t>(input.year));
    builder.append(NMEA_FIELD_SEP);

    if (input.tz_hour < 0)
    {
        builder.append('-');
        append_padded(builder, static_cast<uint32_t>(-input.tz_hour), 2U);
    }
    else
    {
        append_padded(builder, static_cast<uint32_t>(input.tz_hour), 2U);
    }
    builder.append(NMEA_FIELD_SEP);
    append_padded(builder, static_cast<uint32_t>(input.tz_min), 2U);

    return finalize_sentence(builder, output, output_capacity, bytes_written);
}

/**
 * @brief Returns the number of `$--GSV` messages needed for `input`'s satellite count.
 */
CASTLE_NODISCARD inline castle::size_type gsv_message_count(CASTLE_CONST gsv_input& input) CASTLE_NOEXCEPT
{
    CASTLE_CONST castle::size_type total_sats = input.satellites.size();
    return (total_sats == 0U)
        ? 1U
        : ((total_sats + NMEA_GSV_SATS_PER_MESSAGE - 1U) / NMEA_GSV_SATS_PER_MESSAGE);
}

/**
 * @brief Builds one `$--GSV` sentence (of `gsv_message_count()`) into a caller-provided buffer.
 * @param message_number One-based message index within the full GSV set.
 * @return `castle::status::invalid_argument` when `!input.valid`, `output == nullptr`,
 * or `message_number` is out of `[1, gsv_message_count(input)]`; otherwise see `generate_gga()`.
 */
template <castle::size_type MaxSentenceLen = NMEA_GENERATOR_MAX_SENTENCE_LEN>
CASTLE_NODISCARD inline castle::status generate_gsv_message(
    CASTLE_CONST gsv_input& input, castle::container::string_view talker, castle::size_type message_number,
    char* output, castle::size_type output_capacity, castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;
    CASTLE_CONST castle::size_type total_messages = gsv_message_count(input);
    if (!input.valid || (output == nullptr) || (message_number == 0U) || (message_number > total_messages))
    {
        return castle::status::invalid_argument;
    }

    castle::string_builder<MaxSentenceLen> builder;
    builder.append(NMEA_START_CHAR);
    builder.append(talker);
    builder.append(SENTENCE_GSV);
    builder.append(NMEA_FIELD_SEP);

    builder.append(static_cast<uint32_t>(total_messages));
    builder.append(NMEA_FIELD_SEP);
    builder.append(static_cast<uint32_t>(message_number));
    builder.append(NMEA_FIELD_SEP);
    append_padded(builder, static_cast<uint32_t>(input.satellites.size()), 2U);

    CASTLE_CONST castle::size_type total_sats = input.satellites.size();
    CASTLE_CONST castle::size_type offset = (message_number - 1U) * NMEA_GSV_SATS_PER_MESSAGE;
    CASTLE_CONST castle::size_type count_in_message =
        ((offset + NMEA_GSV_SATS_PER_MESSAGE) <= total_sats) ? NMEA_GSV_SATS_PER_MESSAGE : (total_sats - offset);

    for (castle::size_type index = 0U; index < count_in_message; ++index)
    {
        CASTLE_CONST gsv_satellite_record& satellite = input.satellites[offset + index];

        builder.append(NMEA_FIELD_SEP);
        builder.append(static_cast<uint32_t>(satellite.sv_id));
        builder.append(NMEA_FIELD_SEP);
        if (satellite.elevation_deg >= 0)
        {
            builder.append(static_cast<int32_t>(satellite.elevation_deg));
        }
        builder.append(NMEA_FIELD_SEP);
        builder.append(static_cast<int32_t>(satellite.azimuth_deg));
        builder.append(NMEA_FIELD_SEP);
        if (satellite.snr > 0U)
        {
            builder.append(static_cast<uint32_t>(satellite.snr));
        }
    }

    return finalize_sentence(builder, output, output_capacity, bytes_written);
}

} // namespace generator
} // namespace nmea
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_NMEA_NMEA_GENERATOR_HPP
