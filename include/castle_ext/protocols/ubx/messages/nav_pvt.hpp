// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_PVT_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_PVT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"

#include <stdint.h>

namespace castle_ext
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief Fix type reported by UBX-NAV-PVT. */
enum class nav_pvt_fix_type : uint8_t
{
    no_fix = 0U,
    dead_reck = 1U,
    fix_2d = 2U,
    fix_3d = 3U,
    gnss_dr = 4U,
    time_only = 5U
};

/** @brief UBX-NAV-PVT: navigation position/velocity/time solution. */
struct nav_pvt
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_PVT;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 92U;

    static CASTLE_CONSTEXPR uint8_t VALID_DATE = 0x01U;
    static CASTLE_CONSTEXPR uint8_t VALID_TIME = 0x02U;
    static CASTLE_CONSTEXPR uint8_t VALID_FULLY = 0x04U;
    static CASTLE_CONSTEXPR uint8_t VALID_MAG_DEC = 0x08U;

    static CASTLE_CONSTEXPR uint8_t FLAGS_GNSS_FIX_OK = 0x01U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_DIFF_SOLN = 0x02U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_PSM_STATE_MASK = 0x1CU;
    static CASTLE_CONSTEXPR uint8_t FLAGS_HEAD_VEH_VALID = 0x20U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_CARR_SOLN_MASK = 0xC0U;

    uint32_t i_tow = 0U;
    uint16_t year = 0U;
    uint8_t month = 0U;
    uint8_t day = 0U;
    uint8_t hour = 0U;
    uint8_t minute = 0U;
    uint8_t second = 0U;
    uint8_t valid = 0U;
    uint32_t t_acc = 0U;
    int32_t nano = 0;
    nav_pvt_fix_type fix_type = nav_pvt_fix_type::no_fix;
    uint8_t flags = 0U;
    uint8_t flags2 = 0U;
    uint8_t num_sv = 0U;
    int32_t lon = 0;
    int32_t lat = 0;
    int32_t height = 0;
    int32_t h_msl = 0;
    uint32_t h_acc = 0U;
    uint32_t v_acc = 0U;
    int32_t vel_n = 0;
    int32_t vel_e = 0;
    int32_t vel_d = 0;
    int32_t g_speed = 0;
    int32_t head_mot = 0;
    uint32_t s_acc = 0U;
    uint32_t head_acc = 0U;
    uint16_t p_dop = 0U;
    uint8_t flags3 = 0U;
    int32_t head_veh = 0;
    int16_t mag_dec = 0;
    uint16_t mag_acc = 0U;

    CASTLE_NODISCARD bool fix_ok() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_GNSS_FIX_OK) != 0U;
    }

    CASTLE_NODISCARD double longitude_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(lon) * 1e-7;
    }

    CASTLE_NODISCARD double latitude_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(lat) * 1e-7;
    }

    CASTLE_NODISCARD double height_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(height) / 1000.0;
    }

    CASTLE_NODISCARD double pdop_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(p_dop) * 0.01;
    }

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_pvt& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = nav_pvt{};
        out.i_tow = reader.read_u32();
        out.year = reader.read_u16();
        out.month = reader.read_u8();
        out.day = reader.read_u8();
        out.hour = reader.read_u8();
        out.minute = reader.read_u8();
        out.second = reader.read_u8();
        out.valid = reader.read_u8();
        out.t_acc = reader.read_u32();
        out.nano = reader.read_i32();
        out.fix_type = static_cast<nav_pvt_fix_type>(reader.read_u8());
        out.flags = reader.read_u8();
        out.flags2 = reader.read_u8();
        out.num_sv = reader.read_u8();
        out.lon = reader.read_i32();
        out.lat = reader.read_i32();
        out.height = reader.read_i32();
        out.h_msl = reader.read_i32();
        out.h_acc = reader.read_u32();
        out.v_acc = reader.read_u32();
        out.vel_n = reader.read_i32();
        out.vel_e = reader.read_i32();
        out.vel_d = reader.read_i32();
        out.g_speed = reader.read_i32();
        out.head_mot = reader.read_i32();
        out.s_acc = reader.read_u32();
        out.head_acc = reader.read_u32();
        out.p_dop = reader.read_u16();
        out.flags3 = reader.read_u8();
        reader.skip(5U);
        out.head_veh = reader.read_i32();
        out.mag_dec = reader.read_i16();
        out.mag_acc = reader.read_u16();
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_PVT_HPP
