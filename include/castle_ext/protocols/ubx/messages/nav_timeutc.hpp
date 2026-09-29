// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEUTC_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEUTC_HPP

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

/** @brief UBX-NAV-TIMEUTC: UTC time solution. */
struct nav_timeutc
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_TIMEUTC;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 20U;

    uint32_t i_tow = 0U;
    uint32_t t_acc = 0U;
    int32_t nano = 0;
    uint16_t year = 0U;
    uint8_t month = 0U;
    uint8_t day = 0U;
    uint8_t hour = 0U;
    uint8_t min = 0U;
    uint8_t sec = 0U;
    uint8_t valid = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_timeutc& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = nav_timeutc{};
        out.i_tow = reader.read_u32();
        out.t_acc = reader.read_u32();
        out.nano = reader.read_i32();
        out.year = reader.read_u16();
        out.month = reader.read_u8();
        out.day = reader.read_u8();
        out.hour = reader.read_u8();
        out.min = reader.read_u8();
        out.sec = reader.read_u8();
        out.valid = reader.read_u8();
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEUTC_HPP
