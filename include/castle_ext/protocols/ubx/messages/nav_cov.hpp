// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_cov.hpp
 * @brief UBX-NAV-COV (0x01 0x36): position and velocity covariance matrices.
 *
 * Periodic/polled output in the local-level North/East/Down frame. The
 * matrices are symmetric, so only the upper triangle is transmitted, as
 * IEEE-754 single-precision values.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_COV_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_COV_HPP

#include "castle/core/compiler.hpp"
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

/** @brief UBX-NAV-COV output payload. */
struct nav_cov : detail::fixed_message<nav_cov, UBX_CLASS_NAV, UBX_ID_NAV_COV, 64U>
{
    uint32_t i_tow = 0U;         ///< GPS time of week [ms].
    uint8_t version = 0U;        ///< Message version (0x00 for this layout).
    uint8_t pos_cov_valid = 0U;  ///< Position covariance matrix validity flag.
    uint8_t vel_cov_valid = 0U;  ///< Velocity covariance matrix validity flag.
    float pos_cov_nn = 0.0F;     ///< Position covariance p_NN [m^2].
    float pos_cov_ne = 0.0F;     ///< Position covariance p_NE [m^2].
    float pos_cov_nd = 0.0F;     ///< Position covariance p_ND [m^2].
    float pos_cov_ee = 0.0F;     ///< Position covariance p_EE [m^2].
    float pos_cov_ed = 0.0F;     ///< Position covariance p_ED [m^2].
    float pos_cov_dd = 0.0F;     ///< Position covariance p_DD [m^2].
    float vel_cov_nn = 0.0F;     ///< Velocity covariance v_NN [m^2/s^2].
    float vel_cov_ne = 0.0F;     ///< Velocity covariance v_NE [m^2/s^2].
    float vel_cov_nd = 0.0F;     ///< Velocity covariance v_ND [m^2/s^2].
    float vel_cov_ee = 0.0F;     ///< Velocity covariance v_EE [m^2/s^2].
    float vel_cov_ed = 0.0F;     ///< Velocity covariance v_ED [m^2/s^2].
    float vel_cov_dd = 0.0F;     ///< Velocity covariance v_DD [m^2/s^2].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.version);
        stream.field(self.pos_cov_valid);
        stream.field(self.vel_cov_valid);
        stream.reserved(9U);
        stream.field(self.pos_cov_nn);
        stream.field(self.pos_cov_ne);
        stream.field(self.pos_cov_nd);
        stream.field(self.pos_cov_ee);
        stream.field(self.pos_cov_ed);
        stream.field(self.pos_cov_dd);
        stream.field(self.vel_cov_nn);
        stream.field(self.vel_cov_ne);
        stream.field(self.vel_cov_nd);
        stream.field(self.vel_cov_ee);
        stream.field(self.vel_cov_ed);
        stream.field(self.vel_cov_dd);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_COV_HPP
