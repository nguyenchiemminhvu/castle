// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_DOP_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_DOP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
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

/** @brief UBX-NAV-DOP: dilution of precision values. */
struct nav_dop
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_DOP;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 18U;

    uint32_t i_tow = 0U;
    uint16_t g_dop = 0U;
    uint16_t p_dop = 0U;
    uint16_t t_dop = 0U;
    uint16_t v_dop = 0U;
    uint16_t h_dop = 0U;
    uint16_t n_dop = 0U;
    uint16_t e_dop = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_dop& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = nav_dop{};
        out.i_tow = reader.read_u32();
        out.g_dop = reader.read_u16();
        out.p_dop = reader.read_u16();
        out.t_dop = reader.read_u16();
        out.v_dop = reader.read_u16();
        out.h_dop = reader.read_u16();
        out.n_dop = reader.read_u16();
        out.e_dop = reader.read_u16();
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_DOP_HPP
