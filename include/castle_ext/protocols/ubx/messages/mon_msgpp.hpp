// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_msgpp.hpp
 * @brief UBX-MON-MSGPP (0x0a 0x06): message parse and process status.
 *
 * Periodic/polled output (deprecated in favour of UBX-MON-COMMS), 120 bytes.
 * The spec's six fields `msg1`..`msg6` (one `U2[8]` per port 0..5) are stored
 * as one 6x8 table: `msg[port][protocol]`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_MSGPP_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_MSGPP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
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

/** @brief UBX-MON-MSGPP output payload. */
struct mon_msgpp : detail::fixed_message<mon_msgpp, UBX_CLASS_MON, UBX_ID_MON_MSGPP, 120U>
{
    static CASTLE_CONSTEXPR castle::size_type num_ports = 6U;
    static CASTLE_CONSTEXPR castle::size_type num_protocols = 8U;

    /** @brief `msg[p][k]` = spec `msg<p+1>[k]`: messages successfully parsed for protocol `k` on port `p` [msgs]. */
    castle::container::array<castle::container::array<uint16_t, num_protocols>, num_ports> msg{};
    castle::container::array<uint32_t, num_ports> skipped{}; ///< Skipped bytes per port [bytes].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.msg);
        stream.field(self.skipped);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_MSGPP_HPP
