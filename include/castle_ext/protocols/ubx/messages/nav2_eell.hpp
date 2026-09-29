// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_EELL_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_EELL_HPP

#include "castle_ext/protocols/ubx/messages/nav_eell.hpp"

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-NAV2-EELL: secondary-output position error ellipse, same layout as `nav_eell`. */
struct nav2_eell : nav_eell
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV2;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV2_EELL;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav2_eell& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != 16U)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = nav2_eell{};
        out.i_tow = reader.read_u32();
        out.version = reader.read_u8();
        reader.skip(3U);
        out.err_maj = reader.read_u16();
        out.err_min = reader.read_u16();
        out.err_orient = reader.read_u16();
        reader.skip(2U);
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_EELL_HPP
