// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_UPD_SOS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_UPD_SOS_HPP

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

/** @brief UBX-UPD-SOS: backup/restore acknowledgement for the save-on-shutdown feature. */
struct upd_sos_output
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_UPD;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_UPD_SOS;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 8U;

    uint8_t cmd = 0U;
    uint8_t response = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, upd_sos_output& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = upd_sos_output{};
        out.cmd = reader.read_u8();
        reader.skip(3U);
        out.response = reader.read_u8();
        reader.skip(3U);
        if ((out.cmd != UPD_SOS_CMD_ACK) && (out.cmd != UPD_SOS_CMD_RESTORE))
        {
            return false;
        }

        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_UPD_SOS_HPP
