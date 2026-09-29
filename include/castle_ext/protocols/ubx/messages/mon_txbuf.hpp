// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_TXBUF_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_TXBUF_HPP

#include "castle/core/compiler.hpp"
#include "castle/container/array.hpp"
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

/** @brief UBX-MON-TXBUF: transmitter buffer usage per target. */
struct mon_txbuf
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_TXBUF;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 28U;
    static CASTLE_CONSTEXPR castle::size_type num_targets = 6U;

    castle::container::array<uint16_t, num_targets> pending{};
    castle::container::array<uint8_t, num_targets> usage{};
    castle::container::array<uint8_t, num_targets> peak_usage{};
    uint8_t t_used = 0U;
    uint8_t t_peak = 0U;
    uint8_t errors = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_txbuf& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = mon_txbuf{};
        for (castle::size_type index = 0U; index < num_targets; ++index)
        {
            out.pending[index] = reader.read_u16();
        }
        for (castle::size_type index = 0U; index < num_targets; ++index)
        {
            out.usage[index] = reader.read_u8();
        }
        for (castle::size_type index = 0U; index < num_targets; ++index)
        {
            out.peak_usage[index] = reader.read_u8();
        }
        out.t_used = reader.read_u8();
        out.t_peak = reader.read_u8();
        out.errors = reader.read_u8();
        reader.skip(1U);
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_TXBUF_HPP
