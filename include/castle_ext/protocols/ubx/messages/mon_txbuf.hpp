// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_txbuf.hpp
 * @brief UBX-MON-TXBUF (0x0a 0x08): transmitter buffer status.
 *
 * Periodic/polled output (deprecated in favour of UBX-MON-COMMS), 28 bytes.
 * Per-target arrays are indexed by I/O target (port) number 0..5.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_TXBUF_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_TXBUF_HPP

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

/** @brief UBX-MON-TXBUF output payload. */
struct mon_txbuf : detail::fixed_message<mon_txbuf, UBX_CLASS_MON, UBX_ID_MON_TXBUF, 28U>
{
    static CASTLE_CONSTEXPR castle::size_type num_targets = 6U;

    /** @name `errors` bits @{ */
    static CASTLE_CONSTEXPR uint8_t ERRORS_LIMIT = 0x3FU; ///< Bits 5..0: buffer limit of target N reached (bit N).
    static CASTLE_CONSTEXPR uint8_t ERRORS_MEM = 0x40U;   ///< Bit 6: memory allocation error.
    static CASTLE_CONSTEXPR uint8_t ERRORS_ALLOC = 0x80U; ///< Bit 7: allocation error (TX buffer full).
    /** @} */

    castle::container::array<uint16_t, num_targets> pending{};  ///< Bytes pending per target [bytes].
    castle::container::array<uint8_t, num_targets> usage{};     ///< Max usage during last sysmon period per target [%].
    castle::container::array<uint8_t, num_targets> peak_usage{}; ///< Max usage per target [%].
    uint8_t t_used = 0U;  ///< `tUsage`: max usage during last sysmon period, all targets [%].
    uint8_t t_peak = 0U;  ///< `tPeakusage`: max usage, all targets [%].
    uint8_t errors = 0U;  ///< Error bitmask; see `ERRORS_*`.

    /** @brief Returns the 6-bit `limit` field: bit N set = buffer limit of target N reached. */
    CASTLE_NODISCARD uint8_t limit() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(errors & ERRORS_LIMIT);
    }

    /** @brief Reports whether the buffer limit of `target` (0..5) was reached; `false` for other targets. */
    CASTLE_NODISCARD bool limit_reached(castle::size_type target) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (target < num_targets) && ((limit() & static_cast<uint8_t>(1U << target)) != 0U);
    }

    /** @brief Reports a memory allocation error (`mem`). */
    CASTLE_NODISCARD bool mem_error() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (errors & ERRORS_MEM) != 0U;
    }

    /** @brief Reports an allocation error, i.e. the TX buffer is full (`alloc`). */
    CASTLE_NODISCARD bool alloc_error() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (errors & ERRORS_ALLOC) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.pending);
        stream.field(self.usage);
        stream.field(self.peak_usage);
        stream.field(self.t_used);
        stream.field(self.t_peak);
        stream.field(self.errors);
        stream.reserved(1U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_TXBUF_HPP
