// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_GST_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_GST_HPP

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

/** @brief NMEA $--GST: pseudorange error statistics. */
struct gst
{
    bool valid = false;
    castle::container::string_view talker{};
    double utc_time = 0.0;
    double rms_dev = 0.0;
    double semi_major = 0.0;
    double semi_minor = 0.0;
    double orient = 0.0;
    double lat_err = 0.0;
    double lon_err = 0.0;
    double alt_err = 0.0;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(SENTENCE_GST);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, gst& out) CASTLE_NOEXCEPT
    {
        if (raw.field_count() < 8U)
        {
            return false;
        }

        out = gst{};
        out.talker = raw.talker;
        field_cursor cursor(raw);
        static_cast<void>(cursor.next_double(out.utc_time));
        static_cast<void>(cursor.next_double(out.rms_dev));
        static_cast<void>(cursor.next_double(out.semi_major));
        static_cast<void>(cursor.next_double(out.semi_minor));
        static_cast<void>(cursor.next_double(out.orient));
        static_cast<void>(cursor.next_double(out.lat_err));
        static_cast<void>(cursor.next_double(out.lon_err));
        static_cast<void>(cursor.next_double(out.alt_err));
        out.valid = true;
        return true;
    }
};

} // namespace messages
} // namespace nmea
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_GST_HPP
