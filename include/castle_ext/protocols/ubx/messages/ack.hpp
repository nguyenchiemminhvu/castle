// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ack.hpp
 * @brief UBX-ACK-ACK (0x05 0x01) / UBX-ACK-NAK (0x05 0x00): input message acknowledgement.
 *
 * Output sent by the receiver after processing an input message (e.g. a
 * UBX-CFG-VALSET), within one second. Both IDs carry the same 2-byte payload
 * naming the class/ID of the acknowledged message, so one template models
 * both (`ack_ack`, `ack_nak`). The `ack` aggregate accepts either ID and
 * reports which one arrived in `accepted`.
 *
 * @code
 * castle::protocols::ubx::messages::ack reply{};
 * if (castle::protocols::ubx::messages::ack::decode(raw, reply) && reply.accepted) { ... }
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ACK_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ACK_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/messages/detail/fixed_message.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-ACK-ACK / UBX-ACK-NAK payload, selected by `MsgId`. */
template <uint8_t MsgId>
struct ack_message : detail::fixed_message<ack_message<MsgId>, UBX_CLASS_ACK, MsgId, 2U>
{
    uint8_t cls_id = 0U;       ///< Class of the (not) acknowledged message.
    uint8_t acked_msg_id = 0U; ///< Message ID of the (not) acknowledged message.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.cls_id);
        stream.field(self.acked_msg_id);
    }
};

/** @brief UBX-ACK-ACK: message acknowledged. */
using ack_ack = ack_message<UBX_ID_ACK_ACK>;

/** @brief UBX-ACK-NAK: message not acknowledged. */
using ack_nak = ack_message<UBX_ID_ACK_NAK>;

/** @brief Either UBX-ACK message; `accepted` tells ACK-ACK from ACK-NAK. */
struct ack
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_ACK;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 2U;

    uint8_t acked_class = 0U; ///< Class of the (not) acknowledged message.
    uint8_t acked_id = 0U;    ///< Message ID of the (not) acknowledged message.
    bool accepted = false;    ///< `true` for ACK-ACK, `false` for ACK-NAK.

    /** @brief Reports whether `raw` is UBX-ACK-ACK or UBX-ACK-NAK. */
    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(UBX_CLASS_ACK, UBX_ID_ACK_ACK) || raw.is(UBX_CLASS_ACK, UBX_ID_ACK_NAK);
    }

    /**
     * @brief Decodes a 2-byte ACK-ACK or ACK-NAK payload.
     * @return `false` for another class/ID or a wrong length; `out` is then value-initialised.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, ack& out) CASTLE_NOEXCEPT
    {
        out = ack{};
        if (!matches(raw) || (raw.payload_length() != payload_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        reader.field(out.acked_class);
        reader.field(out.acked_id);
        if (!reader.ok())
        {
            out = ack{};
            return false;
        }
        out.accepted = (raw.msg_id() == UBX_ID_ACK_ACK);
        return true;
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ACK_HPP
