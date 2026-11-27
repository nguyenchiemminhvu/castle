// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_velned.hpp
 * @brief UBX-NAV-VELNED (0x01 0x12): velocity solution in the NED frame.
 *
 * Periodic/polled output. Velocities are integer cm/s; heading and its
 * accuracy are in 1e-5 deg.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_VELNED_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_VELNED_HPP

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

/** @brief UBX-NAV-VELNED output payload. */
struct nav_velned : detail::fixed_message<nav_velned, UBX_CLASS_NAV, UBX_ID_NAV_VELNED, 36U>
{
    uint32_t i_tow = 0U;   ///< GPS time of week [ms].
    int32_t vel_n = 0;     ///< North velocity [cm/s].
    int32_t vel_e = 0;     ///< East velocity [cm/s].
    int32_t vel_d = 0;     ///< Down velocity [cm/s].
    uint32_t speed = 0U;   ///< 3-D speed [cm/s].
    uint32_t g_speed = 0U; ///< 2-D ground speed [cm/s].
    int32_t heading = 0;   ///< 2-D heading of motion [1e-5 deg].
    uint32_t s_acc = 0U;   ///< Speed accuracy estimate [cm/s].
    uint32_t c_acc = 0U;   ///< Course/heading accuracy estimate [1e-5 deg].

    CASTLE_NODISCARD double vel_n_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(vel_n) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double vel_e_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(vel_e) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double vel_d_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(vel_d) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double speed_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(speed) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double g_speed_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(g_speed) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double heading_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(heading) * castle::math::pow10< -5, double>();
    }

    CASTLE_NODISCARD double s_acc_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(s_acc) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double c_acc_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(c_acc) * castle::math::pow10< -5, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.vel_n);
        stream.field(self.vel_e);
        stream.field(self.vel_d);
        stream.field(self.speed);
        stream.field(self.g_speed);
        stream.field(self.heading);
        stream.field(self.s_acc);
        stream.field(self.c_acc);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_VELNED_HPP
