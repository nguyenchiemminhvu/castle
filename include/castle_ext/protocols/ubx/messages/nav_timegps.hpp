// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGPS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGPS_HPP

#include "castle/core/compiler.hpp"
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

/** @brief UBX-NAV-TIMEGPS: GPS time solution. */
struct nav_timegps
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_TIMEGPS;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 16U;

    uint32_t i_tow = 0U;
    int32_t f_tow = 0;
    int16_t week = 0;
    int8_t leap_s = 0;
    uint8_t valid = 0U;
    uint32_t t_acc = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_timegps& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = nav_timegps{};
        out.i_tow = reader.read_u32();
        out.f_tow = reader.read_i32();
        out.week = reader.read_i16();
        out.leap_s = reader.read_i8();
        out.valid = reader.read_u8();
        out.t_acc = reader.read_u32();
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGPS_HPP
