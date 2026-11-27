// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_dop.hpp
 * @brief UBX-NAV-DOP (0x01 0x04): dilution of precision.
 *
 * Periodic/polled output. All DOP values are dimensionless and scaled by 100
 * (156 on the wire = 1.56). UBX-NAV2-DOP (0x29 0x04) carries the identical
 * payload, so both are one template, `basic_nav_dop<Class, Id>`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_DOP_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_DOP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
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

/**
 * @brief NAV/NAV2-DOP payload, shared by both classes.
 * @tparam MsgClass `UBX_CLASS_NAV` or `UBX_CLASS_NAV2`.
 * @tparam MsgId `UBX_ID_NAV_DOP` or `UBX_ID_NAV2_DOP`.
 */
template <uint8_t MsgClass, uint8_t MsgId>
struct basic_nav_dop : detail::fixed_message<basic_nav_dop<MsgClass, MsgId>, MsgClass, MsgId, 18U>
{
    uint32_t i_tow = 0U;  ///< GPS time of week [ms].
    uint16_t g_dop = 0U;  ///< Geometric DOP [0.01].
    uint16_t p_dop = 0U;  ///< Position DOP [0.01].
    uint16_t t_dop = 0U;  ///< Time DOP [0.01].
    uint16_t v_dop = 0U;  ///< Vertical DOP [0.01].
    uint16_t h_dop = 0U;  ///< Horizontal DOP [0.01].
    uint16_t n_dop = 0U;  ///< Northing DOP [0.01].
    uint16_t e_dop = 0U;  ///< Easting DOP [0.01].

    CASTLE_NODISCARD double gdop_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return scaled(g_dop);
    }

    CASTLE_NODISCARD double pdop_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return scaled(p_dop);
    }

    CASTLE_NODISCARD double tdop_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return scaled(t_dop);
    }

    CASTLE_NODISCARD double vdop_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return scaled(v_dop);
    }

    CASTLE_NODISCARD double hdop_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return scaled(h_dop);
    }

    CASTLE_NODISCARD double ndop_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return scaled(n_dop);
    }

    CASTLE_NODISCARD double edop_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return scaled(e_dop);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.g_dop);
        stream.field(self.p_dop);
        stream.field(self.t_dop);
        stream.field(self.v_dop);
        stream.field(self.h_dop);
        stream.field(self.n_dop);
        stream.field(self.e_dop);
    }

private:
    CASTLE_NODISCARD static double scaled(uint16_t raw) CASTLE_NOEXCEPT
    {
        return static_cast<double>(raw) * castle::math::pow10< -2, double>();
    }
};

/** @brief UBX-NAV-DOP (0x01 0x04). */
using nav_dop = basic_nav_dop<UBX_CLASS_NAV, UBX_ID_NAV_DOP>;

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_DOP_HPP
