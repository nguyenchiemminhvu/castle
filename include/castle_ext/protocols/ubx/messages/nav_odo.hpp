// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ODO_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ODO_HPP

#include "castle/core/compiler.hpp"
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

/** @brief UBX-NAV-ODO: accumulated odometer distance. */
struct nav_odo
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_ODO;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 20U;

    uint8_t version = 0U;
    uint32_t i_tow = 0U;
    uint32_t distance = 0U;
    uint32_t total_distance = 0U;
    uint32_t distance_std = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_odo& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = nav_odo{};
        out.version = reader.read_u8();
        reader.skip(3U);
        out.i_tow = reader.read_u32();
        out.distance = reader.read_u32();
        out.total_distance = reader.read_u32();
        out.distance_std = reader.read_u32();
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ODO_HPP
