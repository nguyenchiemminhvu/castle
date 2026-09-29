// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_IO_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_IO_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief Per-port I/O statistics block decoded from UBX-MON-IO. */
struct mon_io_port
{
    uint32_t rx_bytes = 0U;
    uint32_t tx_bytes = 0U;
    uint16_t parity_errs = 0U;
    uint16_t framing_errs = 0U;
    uint16_t overrun_errs = 0U;
    uint16_t break_cond = 0U;
    uint8_t rx_busy = 0U;
    uint8_t tx_busy = 0U;
};

/** @brief UBX-MON-IO: per-port I/O subsystem status. */
template <castle::size_type MaxPorts = 8U>
struct mon_io
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_IO;
    static CASTLE_CONSTEXPR castle::size_type block_length = 20U;

    uint8_t num_ports = 0U;
    castle::container::array<mon_io_port, MaxPorts> ports{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_io& out) CASTLE_NOEXCEPT
    {
        if ((raw.payload_length() == 0U) || ((raw.payload_length() % block_length) != 0U))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = mon_io{};
        castle::size_type count = raw.payload_length() / block_length;
        castle::size_type decode_count = (count < MaxPorts) ? count : MaxPorts;
        out.num_ports = static_cast<uint8_t>(decode_count);
        for (castle::size_type index = 0U; index < decode_count; ++index)
        {
            auto& port = out.ports[index];
            port.rx_bytes = reader.read_u32();
            port.tx_bytes = reader.read_u32();
            port.parity_errs = reader.read_u16();
            port.framing_errs = reader.read_u16();
            port.overrun_errs = reader.read_u16();
            port.break_cond = reader.read_u16();
            port.rx_busy = reader.read_u8();
            port.tx_busy = reader.read_u8();
            reader.skip(2U);
        }
        if (count > decode_count)
        {
            reader.skip((count - decode_count) * block_length);
        }

        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_IO_HPP
