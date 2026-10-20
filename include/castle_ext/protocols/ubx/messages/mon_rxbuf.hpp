// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_rxbuf.hpp
 * @brief UBX-MON-RXBUF (0x0a 0x07): receiver buffer status.
 *
 * Periodic/polled output (deprecated in favour of UBX-MON-COMMS), 24 bytes.
 * Per-target arrays are indexed by I/O target (port) number 0..5.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_RXBUF_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_RXBUF_HPP

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

/** @brief UBX-MON-RXBUF output payload. */
struct mon_rxbuf : detail::fixed_message<mon_rxbuf, UBX_CLASS_MON, UBX_ID_MON_RXBUF, 24U>
{
    static CASTLE_CONSTEXPR castle::size_type num_targets = 6U;

    castle::container::array<uint16_t, num_targets> pending{};   ///< Bytes pending per target [bytes].
    castle::container::array<uint8_t, num_targets> usage{};      ///< Max usage during last sysmon period per target [%].
    castle::container::array<uint8_t, num_targets> peak_usage{}; ///< Max usage per target [%].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.pending);
        stream.field(self.usage);
        stream.field(self.peak_usage);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_RXBUF_HPP
