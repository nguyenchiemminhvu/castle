// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_RMC_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_RMC_HPP

#include "castle/core/compiler.hpp"
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

/** @brief Positioning mode indicator reported by NMEA $--RMC. */
enum class rmc_mode : char
{
    autonomous = 'A',
    differential = 'D',
    estimated = 'E',
    manual = 'M',
    not_valid = 'N',
    simulated = 'S',
    unknown = '\0'
};

/** @brief NMEA $--RMC: recommended minimum specific GNSS data. */
struct rmc
{
    bool valid = false;
    castle::container::string_view talker{};
    double utc_time = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double speed_knots = 0.0;
    double course_true = 0.0;
    double mag_variation = 0.0;
    bool status_active = false;
    uint32_t date = 0U;
    rmc_mode mode = rmc_mode::unknown;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(SENTENCE_RMC);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, rmc& out) CASTLE_NOEXCEPT
    {
        if (raw.field_count() < 9U) // LCOV_EXCL_BR_LINE
        {
            return false;
        }

        out = rmc{};
        out.talker = raw.talker;
        field_cursor cursor(raw);
        char status = '\0';
        bool ok = true;
        ok = ok && cursor.next_double(out.utc_time); // LCOV_EXCL_BR_LINE
        ok = ok && cursor.next_char(status); // LCOV_EXCL_BR_LINE
        out.status_active = ((status == 'A') || (status == 'a'));
        ok = ok && cursor.next_latlon(out.latitude); // LCOV_EXCL_BR_LINE
        ok = ok && cursor.next_latlon(out.longitude); // LCOV_EXCL_BR_LINE
        ok = ok && cursor.next_double(out.speed_knots); // LCOV_EXCL_BR_LINE
        ok = ok && cursor.next_double(out.course_true); // LCOV_EXCL_BR_LINE
        ok = ok && cursor.next_uint(out.date); // LCOV_EXCL_BR_LINE
        out.mag_variation = cursor.next_double_or(0.0);

        char mag_direction = '\0';
        if (!raw.field(10U).empty())
        {
            static_cast<void>(cursor.next_char(mag_direction));
            if ((mag_direction == 'W') || (mag_direction == 'w')) // LCOV_EXCL_BR_LINE
            {
                out.mag_variation = -out.mag_variation;
            }
        }

        char mode = '\0';
        if ((raw.field_count() > 11U) && cursor.next_char(mode)) // LCOV_EXCL_BR_LINE
        {
            out.mode = static_cast<rmc_mode>(mode);
        }

        out.valid = ok;
        return ok;
    }
};

} // namespace messages
} // namespace nmea
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_RMC_HPP
