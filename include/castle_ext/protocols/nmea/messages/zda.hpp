// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_ZDA_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_ZDA_HPP

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

/** @brief NMEA $--ZDA: UTC date/time and local time zone. */
struct zda
{
    bool valid = false;
    castle::container::string_view talker{};
    double utc_time = 0.0;
    uint8_t day = 0U;
    uint8_t month = 0U;
    uint16_t year = 0U;
    int8_t tz_hour = 0;
    uint8_t tz_min = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(SENTENCE_ZDA);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, zda& out) CASTLE_NOEXCEPT
    {
        if (raw.field_count() < 4U) // LCOV_EXCL_BR_LINE
        {
            return false;
        }

        out = zda{};
        out.talker = raw.talker;
        field_cursor cursor(raw);
        static_cast<void>(cursor.next_double(out.utc_time));

        uint32_t value = 0U;
        if (cursor.next_uint(value)) // LCOV_EXCL_BR_LINE
        {
            out.day = static_cast<uint8_t>(value);
        }
        if (cursor.next_uint(value)) // LCOV_EXCL_BR_LINE
        {
            out.month = static_cast<uint8_t>(value);
        }
        if (cursor.next_uint(value)) // LCOV_EXCL_BR_LINE
        {
            out.year = static_cast<uint16_t>(value);
        }

        int hour = 0;
        if ((raw.field_count() > 4U) && cursor.next_int(hour)) // LCOV_EXCL_BR_LINE
        {
            out.tz_hour = static_cast<int8_t>(hour);
        }
        if ((raw.field_count() > 5U) && cursor.next_uint(value)) // LCOV_EXCL_BR_LINE
        {
            out.tz_min = static_cast<uint8_t>(value);
        }

        out.valid = true;
        return true;
    }
};

} // namespace messages
} // namespace nmea
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_ZDA_HPP
