// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GGA_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GGA_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string_view.hpp"
#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/field_cursor.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace nmea
{
namespace messages
{

/** @brief Fix quality reported by NMEA $--GGA. */
enum class gga_fix_quality : uint8_t
{
    invalid = 0U,
    gps_sps = 1U,
    dgps = 2U,
    pps = 3U,
    rtk_fixed = 4U,
    rtk_float = 5U,
    estimated = 6U,
    manual = 7U,
    simulation = 8U
};

/** @brief NMEA $--GGA: global positioning system fix data. */
struct gga
{
    bool valid = false;
    castle::container::string_view talker{};
    double utc_time = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    gga_fix_quality fix_quality = gga_fix_quality::invalid;
    uint8_t num_satellites = 0U;
    double hdop = 0.0;
    double altitude_msl = 0.0;
    double geoid_separation = 0.0;
    double dgps_age = -1.0;
    uint16_t dgps_station_id = 0U;

    CASTLE_NODISCARD bool fix_available() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return fix_quality != gga_fix_quality::invalid;
    }

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(SENTENCE_GGA);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, gga& out) CASTLE_NOEXCEPT
    {
        if (raw.field_count() < 14U) // LCOV_EXCL_BR_LINE
        {
            return false;
        }

        out = gga{};
        out.talker = raw.talker;
        field_cursor cursor(raw);
        bool ok = true;
        ok = ok && cursor.next_double(out.utc_time); // LCOV_EXCL_BR_LINE
        ok = ok && cursor.next_latlon(out.latitude); // LCOV_EXCL_BR_LINE
        ok = ok && cursor.next_latlon(out.longitude); // LCOV_EXCL_BR_LINE

        uint32_t fix_quality = 0U;
        uint32_t num_satellites = 0U;
        ok = ok && cursor.next_uint(fix_quality); // LCOV_EXCL_BR_LINE
        out.fix_quality = static_cast<gga_fix_quality>(fix_quality);
        ok = ok && cursor.next_uint(num_satellites); // LCOV_EXCL_BR_LINE
        out.num_satellites = static_cast<uint8_t>(num_satellites);

        out.hdop = cursor.next_double_or(0.0);
        out.altitude_msl = cursor.next_double_or(0.0);
        cursor.skip();
        out.geoid_separation = cursor.next_double_or(0.0);
        cursor.skip();
        out.dgps_age = cursor.next_double_or(-1.0);
        uint32_t station = 0U;
        if (cursor.next_uint(station))
        {
            out.dgps_station_id = static_cast<uint16_t>(station);
        }

        out.valid = ok;
        return ok;
    }
};

} // namespace messages
} // namespace nmea
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GGA_HPP
