// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_posecef.hpp
 * @brief UBX-NAV-POSECEF (0x01 0x01): position solution in ECEF.
 *
 * Periodic/polled output. Coordinates and accuracy are integer centimetres.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_POSECEF_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_POSECEF_HPP

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

/** @brief UBX-NAV-POSECEF output payload. */
struct nav_posecef : detail::fixed_message<nav_posecef, UBX_CLASS_NAV, UBX_ID_NAV_POSECEF, 20U>
{
    uint32_t i_tow = 0U; ///< GPS time of week [ms].
    int32_t ecef_x = 0;  ///< ECEF X coordinate [cm].
    int32_t ecef_y = 0;  ///< ECEF Y coordinate [cm].
    int32_t ecef_z = 0;  ///< ECEF Z coordinate [cm].
    uint32_t p_acc = 0U; ///< Position accuracy estimate [cm].

    CASTLE_NODISCARD double ecef_x_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(ecef_x) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double ecef_y_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(ecef_y) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double ecef_z_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(ecef_z) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double p_acc_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(p_acc) * castle::math::pow10< -2, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.ecef_x);
        stream.field(self.ecef_y);
        stream.field(self.ecef_z);
        stream.field(self.p_acc);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_POSECEF_HPP
