// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GLL_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GLL_HPP

#include "castle/core/compiler.hpp"
#include "castle/container/string_view.hpp"
#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/field_cursor.hpp"

namespace castle
{
namespace protocols
{
namespace nmea
{
namespace messages
{

/** @brief NMEA $--GLL: geographic position (latitude/longitude). */
struct gll
{
    bool valid = false;
    castle::container::string_view talker{};
    double latitude = 0.0;
    double longitude = 0.0;
    double utc_time = 0.0;
    bool status_active = false;
    char mode = '\0';

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(SENTENCE_GLL);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, gll& out) CASTLE_NOEXCEPT
    {
        if (raw.field_count() < 5U)
        {
            return false;
        }

        out = gll{};
        out.talker = raw.talker;
        field_cursor cursor(raw);
        if (!cursor.next_latlon(out.latitude)) // LCOV_EXCL_BR_LINE
        {
            return false;
        }
        if (!cursor.next_latlon(out.longitude)) // LCOV_EXCL_BR_LINE    
        {
            return false;
        }

        static_cast<void>(cursor.next_double(out.utc_time));
        if (raw.field_count() > 5U) // LCOV_EXCL_BR_LINE
        {
            char status = '\0';
            if (cursor.next_char(status)) // LCOV_EXCL_BR_LINE
            {
                out.status_active = ((status == 'A') || (status == 'a')); // LCOV_EXCL_BR_LINE
            }
        }
        if (raw.field_count() > 6U) // LCOV_EXCL_BR_LINE
        {
            static_cast<void>(cursor.next_char(out.mode));
        }

        out.valid = true;
        return true;
    }
};

} // namespace messages
} // namespace nmea
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GLL_HPP
