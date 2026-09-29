// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_GBS_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_GBS_HPP

#include "castle/core/compiler.hpp"
#include "castle/container/string_view.hpp"
#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/field_cursor.hpp"

#include <stdint.h>

namespace castle_ext
{
namespace protocols
{
namespace nmea
{
namespace messages
{

/** @brief NMEA $--GBS: RAIM fault detection and exclusion. */
struct gbs
{
    bool valid = false;
    castle::container::string_view talker{};
    double utc_time = 0.0;
    double err_lat = 0.0;
    double err_lon = 0.0;
    double err_alt = 0.0;
    double probability = 0.0;
    double bias = 0.0;
    double std_dev = 0.0;
    int failed_sv_id = -1;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(SENTENCE_GBS);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, gbs& out) CASTLE_NOEXCEPT
    {
        if (raw.field_count() < 4U)
        {
            return false;
        }

        out = gbs{};
        out.talker = raw.talker;
        field_cursor cursor(raw);
        static_cast<void>(cursor.next_double(out.utc_time));
        static_cast<void>(cursor.next_double(out.err_lat));
        static_cast<void>(cursor.next_double(out.err_lon));
        static_cast<void>(cursor.next_double(out.err_alt));

        int sv = 0;
        if (!raw.field(4U).empty() && cursor.next_int(sv)) // LCOV_EXCL_BR_LINE
        {
            out.failed_sv_id = sv;
        }

        out.probability = cursor.next_double_or(0.0);
        out.bias = cursor.next_double_or(0.0);
        out.std_dev = cursor.next_double_or(0.0);
        out.valid = true;
        return true;
    }
};

} // namespace messages
} // namespace nmea
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_NMEA_SENTENCES_GBS_HPP
