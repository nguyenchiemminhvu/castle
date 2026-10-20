// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_comms.hpp
 * @brief UBX-MON-COMMS (0x0a 0x36): communication port information.
 *
 * Periodic/polled output: an 8-byte header followed by `nPorts` 40-byte port
 * blocks (`8 + nPorts·40`). A port is only included once communication has
 * been initiated on it. Each port's `msgs[i]` counts the messages parsed for
 * the protocol identified by the header's `prot_ids[i]`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_COMMS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_COMMS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
#include "castle/container/vector.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"
#include "castle_ext/protocols/ubx/messages/detail/repeated_group.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief Protocol identifier carried in UBX-MON-COMMS `protIds`. */
enum class mon_comms_protocol : uint8_t
{
    ubx = 0U,
    nmea = 1U,
    rtcm2 = 2U,
    rtcm3 = 5U,
    spartn = 6U,
    none = 0xFFU ///< No protocol reported in this slot.
};

/** @brief Number of protocol slots (`protIds` / `msgs` entries). */
static CASTLE_CONSTEXPR castle::size_type MON_COMMS_PROTOCOL_SLOTS = 4U;

/** @brief One port block of UBX-MON-COMMS. */
struct mon_comms_port
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 40U;

    uint16_t port_id = 0U;        ///< Unique port identifier (see the Integration manual).
    uint16_t tx_pending = 0U;     ///< Bytes pending in the transmitter buffer [bytes].
    uint32_t tx_bytes = 0U;       ///< Bytes ever sent [bytes].
    uint8_t tx_usage = 0U;        ///< Max transmitter buffer usage during the last sysmon period [%].
    uint8_t tx_peak_usage = 0U;   ///< Max transmitter buffer usage [%].
    uint16_t rx_pending = 0U;     ///< Bytes in the receiver buffer [bytes].
    uint32_t rx_bytes = 0U;       ///< Bytes ever received [bytes].
    uint8_t rx_usage = 0U;        ///< Max receiver buffer usage during the last sysmon period [%].
    uint8_t rx_peak_usage = 0U;   ///< Max receiver buffer usage [%].
    uint16_t overrun_errs = 0U;   ///< Number of 100 ms timeslots with overrun errors.
    castle::container::array<uint16_t, MON_COMMS_PROTOCOL_SLOTS> msgs{}; ///< Parsed messages per `prot_ids` slot [msg].
    uint32_t skipped = 0U;        ///< Skipped bytes [bytes].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.port_id);
        stream.field(self.tx_pending);
        stream.field(self.tx_bytes);
        stream.field(self.tx_usage);
        stream.field(self.tx_peak_usage);
        stream.field(self.rx_pending);
        stream.field(self.rx_bytes);
        stream.field(self.rx_usage);
        stream.field(self.rx_peak_usage);
        stream.field(self.overrun_errs);
        stream.field(self.msgs);
        stream.reserved(8U);
        stream.field(self.skipped);
    }
};

/** @brief UBX-MON-COMMS output payload. */
template <castle::size_type MaxPorts = 5U>
struct mon_comms
{
    static_assert(MaxPorts <= 255U, "nPorts is a U1 field");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_COMMS;
    static CASTLE_CONSTEXPR castle::size_type header_length = 8U;
    static CASTLE_CONSTEXPR castle::size_type block_length = mon_comms_port::wire_size;

    /** @name `txErrors` bits @{ */
    static CASTLE_CONSTEXPR uint8_t TX_ERRORS_MEM = 0x01U;   ///< Memory allocation error.
    static CASTLE_CONSTEXPR uint8_t TX_ERRORS_ALLOC = 0x02U; ///< Allocation error (TX buffer full).
    /** @} */

    uint8_t version = 0U;   ///< Message version (0x00).
    uint8_t n_ports = 0U;   ///< On-wire port count; may exceed `ports.size()`.
    uint8_t tx_errors = 0U;
    castle::container::array<mon_comms_protocol, MON_COMMS_PROTOCOL_SLOTS> prot_ids{
        mon_comms_protocol::none, mon_comms_protocol::none, mon_comms_protocol::none, mon_comms_protocol::none};
    castle::container::vector<mon_comms_port, MaxPorts> ports{};

    CASTLE_NODISCARD bool tx_error_mem() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (tx_errors & TX_ERRORS_MEM) != 0U;
    }

    CASTLE_NODISCARD bool tx_error_alloc() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (tx_errors & TX_ERRORS_ALLOC) != 0U;
    }

    /**
     * @brief Looks up the parsed-message count of `protocol` on `port`.
     * @param[out] count `port.msgs` entry of the first slot reporting `protocol`; zero when absent.
     * @return `false` when no `prot_ids` slot reports `protocol`.
     */
    CASTLE_NODISCARD bool parsed_messages(
        CASTLE_CONST mon_comms_port& port,
        mon_comms_protocol protocol,
        uint16_t& count) CASTLE_CONST CASTLE_NOEXCEPT
    {
        count = 0U;
        for (castle::size_type slot = 0U; slot < MON_COMMS_PROTOCOL_SLOTS; ++slot)
        {
            if (prot_ids[slot] == protocol)
            {
                count = port.msgs[slot];
                return true;
            }
        }
        return false;
    }

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxPorts` port blocks.
     * @return `false` unless the payload length is exactly `8 + nPorts·40`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_comms& out) CASTLE_NOEXCEPT
    {
        out = mon_comms{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.n_ports, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.n_ports, block_length))
        {
            out = mon_comms{};
            return false;
        }

        detail::read_group(reader, out.n_ports, out.ports);
        return reader.ok();
    }

    /** @brief Writes the header and every stored port (`nPorts` is taken from `ports.size()`). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t count = static_cast<uint8_t>(ports.size());
        layout_header(*this, count, writer);
        detail::write_group(writer, ports);
    }

private:
    template <typename Self, typename Count, typename Stream>
    static void layout_header(Self& self, Count& count, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.field(count);
        stream.field(self.tx_errors);
        stream.reserved(1U);
        stream.field(self.prot_ids);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_COMMS_HPP
