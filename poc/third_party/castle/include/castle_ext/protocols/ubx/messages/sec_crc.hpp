// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sec_crc.hpp
 * @brief UBX-SEC-CRC (0x27 0x01): CRC of a configuration or firmware region.
 *
 * Output. UBX-SEC-CRC is not part of the ZED-F9R interface description
 * (UBX-19056845); this model follows the generic u-blox layout of its 8-byte
 * prefix. Because other firmware may append data, `decode()` accepts any
 * payload of at least `payload_length` bytes, decodes the prefix and ignores
 * the tail. `encode()` always writes exactly `payload_length` bytes.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_CRC_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_CRC_HPP

#include "castle/core/compiler.hpp"
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

/** @brief UBX-SEC-CRC output payload (8-byte prefix). */
struct sec_crc : detail::fixed_message<sec_crc, UBX_CLASS_SEC, UBX_ID_SEC_CRC, 8U>
{
    uint8_t version = 0U;   ///< Message version.
    uint8_t crc_type = 0U;  ///< CRC type selector.
    uint16_t reserved = 0U; ///< Reserved bytes 2..3, kept verbatim.
    uint32_t crc32 = 0U;    ///< CRC-32 value.

    /**
     * @brief Decodes the first `payload_length` bytes of `raw` into `out`.
     * @return `false` for a different class/ID or a payload shorter than
     * `payload_length`; `out` is then value-initialised. Extra bytes are ignored.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, sec_crc& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < payload_length)
        {
            out = sec_crc{};
            return false;
        }
        message_view prefix = raw;
        prefix.header.payload_length = static_cast<uint16_t>(payload_length);
        prefix.payload = raw.payload.subview(0U, payload_length);
        return fixed_message::decode(prefix, out);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.field(self.crc_type);
        stream.field(self.reserved);
        stream.field(self.crc32);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_CRC_HPP
