// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_CRC_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_CRC_HPP

#include "castle/core/compiler.hpp"
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

/** @brief UBX-SEC-CRC: CRC of a configuration or firmware region. */
struct sec_crc
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_SEC;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_SEC_CRC;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 8U;

    uint8_t version = 0U;
    uint8_t crc_type = 0U;
    uint16_t reserved = 0U;
    uint32_t crc32 = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, sec_crc& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = sec_crc{};
        out.version = reader.read_u8();
        out.crc_type = reader.read_u8();
        out.reserved = reader.read_u16();
        out.crc32 = reader.read_u32();
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_CRC_HPP
