// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_eell.hpp
 * @brief UBX-NAV-EELL (0x01 0x3D): position error ellipse parameters.
 *
 * Periodic/polled output. UBX-NAV2-EELL (0x29 0x3D) carries the identical
 * 16-byte payload, so both are one template, `basic_nav_eell<Class, Id>`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_EELL_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_EELL_HPP

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
 * @brief NAV/NAV2-EELL payload, shared by both classes.
 * @tparam MsgClass `UBX_CLASS_NAV` or `UBX_CLASS_NAV2`.
 * @tparam MsgId `UBX_ID_NAV_EELL` or `UBX_ID_NAV2_EELL`.
 */
template <uint8_t MsgClass, uint8_t MsgId>
struct basic_nav_eell : detail::fixed_message<basic_nav_eell<MsgClass, MsgId>, MsgClass, MsgId, 16U>
{
    uint32_t i_tow = 0U;       ///< GPS time of week [ms].
    uint8_t version = 0U;      ///< Message version (0x00 for this version).
    uint16_t err_orient = 0U;  ///< Semi-major axis orientation from true north [1e-2 deg] (`errEllipseOrient`).
    uint32_t err_maj = 0U;     ///< Semi-major axis of the error ellipse [mm] (`errEllipseMajor`).
    uint32_t err_min = 0U;     ///< Semi-minor axis of the error ellipse [mm] (`errEllipseMinor`).

    CASTLE_NODISCARD double err_orient_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(err_orient) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double err_maj_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(err_maj) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double err_min_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(err_min) * castle::math::pow10< -3, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.version);
        stream.reserved(1U);
        stream.field(self.err_orient);
        stream.field(self.err_maj);
        stream.field(self.err_min);
    }
};

/** @brief UBX-NAV-EELL (0x01 0x3D). */
using nav_eell = basic_nav_eell<UBX_CLASS_NAV, UBX_ID_NAV_EELL>;

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_EELL_HPP
