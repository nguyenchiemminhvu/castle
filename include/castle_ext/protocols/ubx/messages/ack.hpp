// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ACK_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ACK_HPP

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

/** @brief UBX-ACK-ACK/NAK: acknowledges or rejects one previously sent message. */
template <uint8_t MessageId>
struct ack_message
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_ACK;
    static CASTLE_CONSTEXPR uint8_t msg_id = MessageId;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 2U;

    uint8_t cls_id = 0U;
    uint8_t msg_id_ = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, ack_message& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = ack_message{};
        out.cls_id = reader.read_u8();
        out.msg_id_ = reader.read_u8();
        return reader.ok();
    }
};

using ack_ack = ack_message<UBX_ID_ACK_ACK>;
using ack_nak = ack_message<UBX_ID_ACK_NAK>;

/** @brief Compatibility aggregate that models both ACK message IDs. */
struct ack
{
    static CASTLE_CONSTEXPR castle::size_type payload_length = 2U;

    uint8_t acked_class = 0U;
    uint8_t acked_id = 0U;
    bool accepted = false;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return (raw.msg_class() == UBX_CLASS_ACK)
            && ((raw.msg_id() == UBX_ID_ACK_ACK) || (raw.msg_id() == UBX_ID_ACK_NAK)); // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, ack& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != payload_length) // LCOV_EXCL_BR_LINE
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = ack{};
        out.acked_class = reader.read_u8();
        out.acked_id = reader.read_u8();
        out.accepted = (raw.msg_id() == UBX_ID_ACK_ACK);
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ACK_HPP
