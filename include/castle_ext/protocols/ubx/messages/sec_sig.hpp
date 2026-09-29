// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_SIG_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_SIG_HPP

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

/** @brief UBX-SEC-SIG: jamming/spoofing indicator. */
struct sec_sig
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_SEC;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_SEC_SIG;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 12U;

    uint8_t version = 0U;
    uint8_t jam_flags = 0U;
    uint8_t jam_det_enabled = 0U;
    uint8_t jamming_state = 0U;
    uint8_t spf_flags = 0U;
    uint8_t spf_det_enabled = 0U;
    uint8_t spoofing_state = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, sec_sig& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = sec_sig{};
        out.version = reader.read_u8();
        reader.skip(3U);
        out.jam_flags = reader.read_u8();
        out.jam_det_enabled = out.jam_flags & 1U;
        out.jamming_state = (out.jam_flags >> 1U) & 3U;
        reader.skip(3U);
        out.spf_flags = reader.read_u8();
        out.spf_det_enabled = out.spf_flags & 1U;
        out.spoofing_state = (out.spf_flags >> 1U) & 7U;
        reader.skip(3U);
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_SIG_HPP
