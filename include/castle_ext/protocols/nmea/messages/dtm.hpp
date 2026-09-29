// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_DTM_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_DTM_HPP

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

/** @brief NMEA $--DTM: datum reference and offsets to WGS84. */
struct dtm
{
    bool valid = false;
    castle::container::string_view talker{};
    castle::container::string_view datum_code{};
    castle::container::string_view datum_sub{};
    castle::container::string_view ref_datum{};
    double lat_offset = 0.0;
    double lon_offset = 0.0;
    double alt_offset = 0.0;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(SENTENCE_DTM);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, dtm& out) CASTLE_NOEXCEPT
    {
        if (raw.field_count() < 7U)
        {
            return false;
        }

        out = dtm{};
        out.talker = raw.talker;
        field_cursor cursor(raw);
        out.datum_code = cursor.next();
        out.datum_sub = cursor.next();

        double value = 0.0;
        if (cursor.next_double(value)) // LCOV_EXCL_BR_LINE
        {
            out.lat_offset = value;
            auto direction = cursor.next();
            if (!direction.empty() && ((direction.front() == 'S') || (direction.front() == 's'))) // LCOV_EXCL_BR_LINE
            {
                out.lat_offset = -out.lat_offset;
            }
        }
        else
        {
            cursor.skip();
        }

        if (cursor.next_double(value)) // LCOV_EXCL_BR_LINE
        {
            out.lon_offset = value;
            auto direction = cursor.next();
            if (!direction.empty() && ((direction.front() == 'W') || (direction.front() == 'w'))) // LCOV_EXCL_BR_LINE
            {
                out.lon_offset = -out.lon_offset;
            }
        }
        else
        {
            cursor.skip();
        }

        out.alt_offset = cursor.next_double_or(0.0);
        if (raw.field_count() > 7U) // LCOV_EXCL_BR_LINE
        {
            out.ref_datum = cursor.next();
        }

        out.valid = true;
        return true;
    }
};

} // namespace messages
} // namespace nmea
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_DTM_HPP
