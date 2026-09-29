// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GSV_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GSV_HPP

#include "castle/core/compiler.hpp"
#include "castle/container/array.hpp"
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

/** @brief One decoded satellite entry from NMEA $--GSV. */
struct gsv_satellite
{
    uint8_t sv_id = 0U;
    int8_t elevation = -1;
    uint16_t azimuth = 0U;
    int8_t snr = -1;
};

/** @brief NMEA $--GSV: satellites in view (one message of a multi-part series). */
struct gsv
{
    static CASTLE_CONSTEXPR castle::size_type max_satellites = 4U;

    bool valid = false;
    castle::container::string_view talker{};
    uint8_t num_msgs = 0U;
    uint8_t msg_num = 0U;
    uint8_t sats_in_view = 0U;
    uint8_t sat_count = 0U;
    uint8_t signal_id = 0U;
    castle::container::array<gsv_satellite, max_satellites> satellites{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(SENTENCE_GSV);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, gsv& out) CASTLE_NOEXCEPT
    {
        if (raw.field_count() < 3U) // LCOV_EXCL_BR_LINE
        {
            return false;
        }

        out = gsv{};
        out.talker = raw.talker;

        uint32_t value = 0U;
        if (!nmea::parse_uint(raw.field(0U), value)) // LCOV_EXCL_BR_LINE
        {
            return false;
        }
        out.num_msgs = static_cast<uint8_t>(value);
        if (!nmea::parse_uint(raw.field(1U), value)) // LCOV_EXCL_BR_LINE
        {
            return false;
        }
        out.msg_num = static_cast<uint8_t>(value);
        if (!nmea::parse_uint(raw.field(2U), value)) // LCOV_EXCL_BR_LINE
        {
            return false;
        }
        out.sats_in_view = static_cast<uint8_t>(value);

        field_cursor cursor(raw);
        cursor.skip(3U);
        castle::size_type remaining = (raw.field_count() > 3U) ? (raw.field_count() - 3U) : 0U; // LCOV_EXCL_BR_LINE
        castle::size_type num_blocks = ((remaining / 4U) > max_satellites) ? max_satellites : (remaining / 4U); // LCOV_EXCL_BR_LINE
        for (castle::size_type index = 0U; index < num_blocks; ++index)
        {
            gsv_satellite& sat = out.satellites[index];
            uint32_t sv_id = 0U;
            static_cast<void>(cursor.next_uint(sv_id));
            sat.sv_id = static_cast<uint8_t>(sv_id);

            // next_int() already consumes the field internally even when parsing fails,
            // so no extra cursor.next() is needed (or correct) on the failure path here.
            int field_value = 0;
            if (cursor.next_int(field_value))
            {
                sat.elevation = static_cast<int8_t>(field_value);
            }
            if (cursor.next_int(field_value))
            {
                sat.azimuth = static_cast<uint16_t>(field_value);
            }
            if (cursor.next_int(field_value))
            {
                sat.snr = static_cast<int8_t>(field_value);
            }
        }

        out.sat_count = static_cast<uint8_t>(num_blocks);
        castle::size_type sig_index = 3U + (num_blocks * 4U);
        if ((sig_index < raw.field_count()) && !raw.field(sig_index).empty()) // LCOV_EXCL_BR_LINE
        {
            if (nmea::parse_uint(raw.field(sig_index), value)) // LCOV_EXCL_BR_LINE
            {
                out.signal_id = static_cast<uint8_t>(value);
            }
        }

        out.valid = true;
        return true;
    }
};

} // namespace messages
} // namespace nmea
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GSV_HPP
