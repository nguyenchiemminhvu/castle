// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_velecef.hpp
 * @brief UBX-NAV-VELECEF (0x01 0x11): velocity solution in ECEF.
 *
 * Periodic/polled output. Velocities and accuracy are integer cm/s.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_VELECEF_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_VELECEF_HPP

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

/** @brief UBX-NAV-VELECEF output payload. */
struct nav_velecef : detail::fixed_message<nav_velecef, UBX_CLASS_NAV, UBX_ID_NAV_VELECEF, 20U>
{
    uint32_t i_tow = 0U; ///< GPS time of week [ms].
    int32_t ecef_vx = 0; ///< ECEF X velocity [cm/s].
    int32_t ecef_vy = 0; ///< ECEF Y velocity [cm/s].
    int32_t ecef_vz = 0; ///< ECEF Z velocity [cm/s].
    uint32_t s_acc = 0U; ///< Speed accuracy estimate [cm/s].

    CASTLE_NODISCARD double ecef_vx_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(ecef_vx) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double ecef_vy_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(ecef_vy) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double ecef_vz_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(ecef_vz) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double s_acc_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(s_acc) * castle::math::pow10< -2, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.ecef_vx);
        stream.field(self.ecef_vy);
        stream.field(self.ecef_vz);
        stream.field(self.s_acc);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_VELECEF_HPP
