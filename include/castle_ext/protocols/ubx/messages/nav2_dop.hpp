// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_DOP_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_DOP_HPP

#include "castle_ext/protocols/ubx/messages/nav_dop.hpp"

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-NAV2-DOP: secondary-output dilution of precision values, same layout as `nav_dop`. */
struct nav2_dop : nav_dop
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV2;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV2_DOP;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav2_dop& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != 18U)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = nav2_dop{};
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
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_DOP_HPP
