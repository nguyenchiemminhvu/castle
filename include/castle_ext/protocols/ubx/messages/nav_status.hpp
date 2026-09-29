// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_STATUS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_STATUS_HPP

#include "castle/core/compiler.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief GPS fix type reported by UBX-NAV-STATUS. */
enum class nav_status_gps_fix : uint8_t
{
    no_fix = 0U,
    dead_reck = 1U,
    fix_2d = 2U,
    fix_3d = 3U,
    gnss_dr = 4U,
    time_only = 5U
};

/** @brief UBX-NAV-STATUS: receiver navigation status. */
struct nav_status
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_STATUS;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 16U;

    static CASTLE_CONSTEXPR uint8_t FLAGS_GPS_FIX_OK = 0x01U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_DIFF_SOLN = 0x02U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_WKN_SET = 0x04U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_TOW_SET = 0x08U;

    uint32_t i_tow = 0U;
    nav_status_gps_fix gps_fix = nav_status_gps_fix::no_fix;
    uint8_t flags = 0U;
    uint8_t fix_stat = 0U;
    uint8_t flags2 = 0U;
    uint32_t ttff = 0U;
    uint32_t msss = 0U;

    CASTLE_NODISCARD bool fix_ok() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_GPS_FIX_OK) != 0U;
    }

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_status& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = nav_status{};
        out.i_tow = reader.read_u32();
        out.gps_fix = static_cast<nav_status_gps_fix>(reader.read_u8());
        out.flags = reader.read_u8();
        out.fix_stat = reader.read_u8();
        out.flags2 = reader.read_u8();
        out.ttff = reader.read_u32();
        out.msss = reader.read_u32();
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_STATUS_HPP
