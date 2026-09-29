// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_VTG_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_VTG_HPP

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

/** @brief NMEA $--VTG: course and speed over ground. */
struct vtg
{
    bool valid = false;
    castle::container::string_view talker{};
    double course_true = 0.0;
    double course_mag = 0.0;
    double speed_knots = 0.0;
    double speed_kmh = 0.0;
    bool course_mag_valid = false;
    char mode = '\0';

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(SENTENCE_VTG);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, vtg& out) CASTLE_NOEXCEPT
    {
        if (raw.field_count() < 7U) // LCOV_EXCL_BR_LINE
        {
            return false;
        }

        out = vtg{};
        out.talker = raw.talker;
        field_cursor cursor(raw);
        static_cast<void>(cursor.next_double(out.course_true));
        cursor.skip();
        if (!raw.field(2U).empty())
        {
            out.course_mag_valid = cursor.next_double(out.course_mag);
        }
        else
        {
            cursor.skip();
        }
        cursor.skip();
        static_cast<void>(cursor.next_double(out.speed_knots));
        cursor.skip();
        static_cast<void>(cursor.next_double(out.speed_kmh));
        cursor.skip();
        if (raw.field_count() > 8U)
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

#endif // CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_VTG_HPP
