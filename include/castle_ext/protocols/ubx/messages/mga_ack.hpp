// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mga_ack.hpp
 * @brief UBX-MGA-ACK (0x13 0x60): acknowledgement of one assistance message.
 *
 * Output sent by the receiver when CFG-NAVSPG-ACKAIDING is enabled. One
 * fixed 8-byte layout. After a UBX-MGA-DBD poll the receiver also sends one
 * UBX-MGA-ACK whose `msgPayloadStart` holds a U4 count of the database
 * entries it transmitted (see `dbd_entry_count()`).
 *
 * @code
 * castle::protocols::ubx::messages::mga_ack ack{};
 * if (castle::protocols::ubx::messages::mga_ack::decode(raw, ack) && ack.accepted()) { ... }
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_ACK_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_ACK_HPP

#include "castle/core/compiler.hpp"
#include "castle/container/array.hpp"
#include "castle/utility/bytes.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
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

/** @brief UBX-MGA-ACK `type`: whether the acknowledged message was used. */
enum class mga_ack_type : uint8_t
{
    not_used = 0U, ///< Not used by the receiver; see `info_code`.
    accepted = 1U  ///< Accepted for use; `info_code` is `accepted`.
};

/** @brief UBX-MGA-ACK `infoCode`: what the receiver did with the message. */
enum class mga_ack_info_code : uint8_t
{
    accepted = 0U,              ///< The receiver accepted the data.
    no_time = 1U,               ///< Receiver time unknown; send UBX-MGA-INI-TIME_UTC first.
    version_not_supported = 2U, ///< Message version not supported.
    size_mismatch = 3U,         ///< Message size does not match the message version.
    store_failed = 4U,          ///< Data could not be stored to the database.
    not_ready = 5U,             ///< Receiver not ready to use the data.
    unknown_type = 6U           ///< Message type unknown.
};

/** @brief Message version byte of UBX-MGA-ACK in this revision. */
static CASTLE_CONSTEXPR uint8_t MGA_ACK_VERSION = 0x00U;

/** @brief UBX-MGA-ACK output payload (8 bytes). */
struct mga_ack : detail::fixed_message<mga_ack, UBX_CLASS_MGA, UBX_ID_MGA_ACK, 8U>
{
    mga_ack_type type = mga_ack_type::not_used;
    mga_ack_info_code info_code = mga_ack_info_code::accepted;
    /** @brief UBX ID of the acknowledged MGA message (spec `msgId`; renamed to avoid `msg_id`). */
    uint8_t acked_msg_id = 0U;
    /** @brief First 4 payload bytes of the acknowledged message. */
    castle::container::array<uint8_t, 4U> msg_payload_start{};

    /** @brief Reports `type == accepted`. */
    CASTLE_NODISCARD bool accepted() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return type == mga_ack_type::accepted;
    }

    /**
     * @brief `msg_payload_start` read as a little-endian U4: the number of
     * UBX-MGA-DBD entries sent, for the ACK that ends a database poll.
     */
    CASTLE_NODISCARD uint32_t dbd_entry_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::read_le32(msg_payload_start.data());
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.type);
        stream.constant(MGA_ACK_VERSION);
        stream.field(self.info_code);
        stream.field(self.acked_msg_id);
        stream.field(self.msg_payload_start);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_ACK_HPP
