// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_TP_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_TP_HPP

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

/** @brief UBX-TIM-TP: time pulse timing information. */
struct tim_tp
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_TIM;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_TIM_TP;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 16U;

    uint32_t tow_ms = 0U;
    uint32_t tow_sub_ms = 0U;
    int32_t q_err = 0;
    uint16_t week = 0U;
    uint8_t flags = 0U;
    uint8_t ref_info = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, tim_tp& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = tim_tp{};
        out.tow_ms = reader.read_u32();
        out.tow_sub_ms = reader.read_u32();
        out.q_err = reader.read_i32();
        out.week = reader.read_u16();
        out.flags = reader.read_u8();
        out.ref_info = reader.read_u8();
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_TP_HPP
