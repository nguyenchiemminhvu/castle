// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_io.hpp
 * @brief UBX-MON-IO (0x0a 0x02): I/O system status.
 *
 * Periodic/polled output (deprecated in favour of UBX-MON-COMMS): one 20-byte
 * block per receiver port (`[0..n]·20`). The payload has no header and no count
 * field; the port count is implied by the payload length.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_IO_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_IO_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
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

/** @brief One per-port block of UBX-MON-IO. */
struct mon_io_port
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 20U;

    uint32_t rx_bytes = 0U;      ///< Number of bytes ever received [bytes].
    uint32_t tx_bytes = 0U;      ///< Number of bytes ever sent [bytes].
    uint16_t parity_errs = 0U;   ///< Number of 100 ms timeslots with parity errors.
    uint16_t framing_errs = 0U;  ///< Number of 100 ms timeslots with framing errors.
    uint16_t overrun_errs = 0U;  ///< Number of 100 ms timeslots with overrun errors.
    uint16_t break_cond = 0U;    ///< Number of 100 ms timeslots with break conditions.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.rx_bytes);
        stream.field(self.tx_bytes);
        stream.field(self.parity_errs);
        stream.field(self.framing_errs);
        stream.field(self.overrun_errs);
        stream.field(self.break_cond);
        stream.reserved(4U);
    }
};

/** @brief UBX-MON-IO output payload. */
template <castle::size_type MaxPorts = 8U>
struct mon_io
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_IO;
    static CASTLE_CONSTEXPR castle::size_type header_length = 0U;
    static CASTLE_CONSTEXPR castle::size_type block_length = mon_io_port::wire_size;

    /** @brief Port blocks in wire order; ports beyond `MaxPorts` are skipped. */
    castle::container::vector<mon_io_port, MaxPorts> ports{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes up to `MaxPorts` port blocks.
     * @return `false` unless the payload length is a whole multiple of 20
     * bytes (zero ports, i.e. an empty payload, is valid).
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_io& out) CASTLE_NOEXCEPT
    {
        out = mon_io{};
        castle::size_type count = 0U;
        if (!matches(raw) || !detail::implied_group_count(raw.payload_length(), header_length, block_length, count))
        {
            return false;
        }
        if (count == 0U)
        {
            // An empty payload reaches us as a null view, which payload_reader rejects.
            return true;
        }

        payload_reader reader(raw.payload);
        detail::read_group(reader, count, out.ports);
        if (!reader.ok())
        {
            out = mon_io{};
            return false;
        }
        return true;
    }

    /** @brief Writes every stored port block. */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        detail::write_group(writer, ports);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_IO_HPP
