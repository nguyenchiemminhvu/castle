// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_GNS_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_GNS_HPP

#include "castle/core/compiler.hpp"
#include "castle/container/string_view.hpp"
#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/field_cursor.hpp"

namespace castle_ext
{
namespace protocols
{
namespace nmea
{
namespace messages
{

/** @brief NMEA $--GNS: GNSS fix data (multi-constellation). */
struct gns
{
    bool valid = false;
    castle::container::string_view talker{};
    castle::container::string_view mode_indicator{};
    double utc_time = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double hdop = 0.0;
    double altitude = 0.0;
    double geoid_sep = 0.0;
    double diff_age = -1.0;
    int num_sats = 0;
    int diff_ref = -1;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(SENTENCE_GNS);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, gns& out) CASTLE_NOEXCEPT
    {
        if (raw.field_count() < 8U) // LCOV_EXCL_BR_LINE
        {
            return false;
        }

        out = gns{};
        out.talker = raw.talker;
        field_cursor cursor(raw);
        static_cast<void>(cursor.next_double(out.utc_time));
        static_cast<void>(cursor.next_latlon(out.latitude));
        static_cast<void>(cursor.next_latlon(out.longitude));
        out.mode_indicator = cursor.next();
        static_cast<void>(cursor.next_int(out.num_sats));
        static_cast<void>(cursor.next_double(out.hdop));
        if (raw.field_count() > 8U)
        {
            static_cast<void>(cursor.next_double(out.altitude));
        }
        if (raw.field_count() > 9U)
        {
            static_cast<void>(cursor.next_double(out.geoid_sep));
        }
        if (raw.field_count() > 10U)
        {
            out.diff_age = cursor.next_double_or(-1.0);
        }
        if (raw.field_count() > 11U)
        {
            int ref = 0;
            if (!raw.field(11U).empty() && cursor.next_int(ref)) // LCOV_EXCL_BR_LINE
            {
                out.diff_ref = ref;
            }
        }

        out.valid = true;
        return true;
    }
};

} // namespace messages
} // namespace nmea
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_GNS_HPP
