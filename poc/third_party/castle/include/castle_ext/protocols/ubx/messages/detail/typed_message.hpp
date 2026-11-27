// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file typed_message.hpp
 * @brief Fixed-length UBX layout selected by a leading `type` byte.
 *
 * Several UBX class/ID pairs carry more than one payload layout and tell them
 * apart by the first payload byte (UBX-MGA-GPS-EPH vs -ALM vs -UTC, ...,
 * UBX-MGA-INI-POS_XYZ vs -TIME_UTC, UBX-RXM-RLM short vs long). Each layout is
 * modelled as its own struct; `typed_message` narrows `matches()` to the
 * layout's type byte and exact length so a decoder registry only dispatches
 * the frame to the one model that can decode it.
 *
 * The layout must start with `stream.constant(type)`, so encoding always
 * writes the discriminator and decoding re-validates it.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_TYPED_MESSAGE_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_TYPED_MESSAGE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
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
namespace detail
{

/**
 * @brief Fixed-length UBX layout discriminated by payload byte 0.
 *
 * @tparam Derived Layout type (see `fixed_message`).
 * @tparam MsgClass UBX class byte.
 * @tparam MsgId UBX message ID byte.
 * @tparam Type Value of payload byte 0 identifying this layout.
 * @tparam PayloadLength Exact payload length in bytes.
 */
template <typename Derived, uint8_t MsgClass, uint8_t MsgId, uint8_t Type, castle::size_type PayloadLength>
struct typed_message : fixed_message<Derived, MsgClass, MsgId, PayloadLength>
{
    static CASTLE_CONSTEXPR uint8_t message_type = Type;

    /** @brief Reports whether `raw` has this class/ID, exact length, and type byte. */
    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(MsgClass, MsgId)
            && (raw.payload_length() == PayloadLength)
            && (raw.payload[0U] == Type);
    }
};

} // namespace detail
} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_TYPED_MESSAGE_HPP
