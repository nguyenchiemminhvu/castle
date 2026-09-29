// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GSA_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GSA_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
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

/** @brief Operating mode selector reported by NMEA $--GSA. */
enum class gsa_op_mode : char
{
    auto_mode = 'A',
    manual = 'M',
    unknown = '\0'
};

/** @brief Navigation (fix) mode reported by NMEA $--GSA. */
enum class gsa_nav_mode : uint8_t
{
    no_fix = 1U,
    fix_2d = 2U,
    fix_3d = 3U,
    unknown = 0U
};

/** @brief NMEA $--GSA: GNSS DOP and active satellites. */
struct gsa
{
    static CASTLE_CONSTEXPR castle::size_type max_satellites = 12U;

    bool valid = false;
    castle::container::string_view talker{};
    gsa_op_mode op_mode = gsa_op_mode::unknown;
    gsa_nav_mode nav_mode = gsa_nav_mode::unknown;
    castle::container::array<uint8_t, max_satellites> sat_ids{};
    double pdop = 0.0;
    double hdop = 0.0;
    double vdop = 0.0;
    uint8_t system_id = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(SENTENCE_GSA);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, gsa& out) CASTLE_NOEXCEPT
    {
        if (raw.field_count() < 17U)
        {
            return false;
        }

        out = gsa{};
        out.talker = raw.talker;
        field_cursor cursor(raw);

        char op = '\0';
        if (cursor.next_char(op)) // LCOV_EXCL_BR_LINE
        {
            if ((op == 'A') || (op == 'a')) // LCOV_EXCL_BR_LINE
            {
                out.op_mode = gsa_op_mode::auto_mode;
            }
            else if ((op == 'M') || (op == 'm')) // LCOV_EXCL_BR_LINE
            {
                out.op_mode = gsa_op_mode::manual;
            }
        }

        uint32_t nav = 0U;
        if (cursor.next_uint(nav))
        {
            out.nav_mode = static_cast<gsa_nav_mode>(nav);
        }

        for (castle::size_type index = 0U; index < max_satellites; ++index)
        {
            uint32_t sv = 0U;
            if (cursor.next_uint(sv))
            {
                out.sat_ids[index] = static_cast<uint8_t>(sv);
            }
        }

        out.pdop = cursor.next_double_or(0.0);
        out.hdop = cursor.next_double_or(0.0);
        out.vdop = cursor.next_double_or(0.0);
        if ((raw.field_count() > 17U) && !raw.field(17U).empty()) // LCOV_EXCL_BR_LINE
        {
            uint32_t system = 0U;
            if (cursor.next_uint(system)) // LCOV_EXCL_BR_LINE
            {
                out.system_id = static_cast<uint8_t>(system);
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

#endif // CASTLE_EXT_PROTOCOLS_NMEA_MESSAGES_GSA_HPP
