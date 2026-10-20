// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_hpposecef.hpp
 * @brief UBX-NAV-HPPOSECEF (0x01 0x13): high-precision position solution in ECEF.
 *
 * Periodic/polled output. Each coordinate is split into a centimetre part
 * (`ecef_x`) and a 0.1 mm high-precision part (`ecef_x_hp`, -99..+99):
 * precise coordinate [cm] = ecefX + ecefXHp * 1e-2.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_HPPOSECEF_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_HPPOSECEF_HPP

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

/** @brief UBX-NAV-HPPOSECEF output payload. */
struct nav_hpposecef : detail::fixed_message<nav_hpposecef, UBX_CLASS_NAV, UBX_ID_NAV_HPPOSECEF, 28U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_INVALID_ECEF = 0x01U; ///< ECEF coordinates (incl. HP parts) invalid.
    /** @} */

    uint8_t version = 0U;  ///< Message version (0x00 for this layout).
    uint32_t i_tow = 0U;   ///< GPS time of week [ms].
    int32_t ecef_x = 0;    ///< ECEF X coordinate [cm].
    int32_t ecef_y = 0;    ///< ECEF Y coordinate [cm].
    int32_t ecef_z = 0;    ///< ECEF Z coordinate [cm].
    int8_t ecef_x_hp = 0;  ///< High-precision part of ECEF X [0.1 mm], -99..+99.
    int8_t ecef_y_hp = 0;  ///< High-precision part of ECEF Y [0.1 mm], -99..+99.
    int8_t ecef_z_hp = 0;  ///< High-precision part of ECEF Z [0.1 mm], -99..+99.
    uint8_t flags = 0U;    ///< Additional flags, see `FLAGS_*`.
    uint32_t p_acc = 0U;   ///< Position accuracy estimate [0.1 mm].

    CASTLE_NODISCARD bool invalid_ecef() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_INVALID_ECEF) != 0U;
    }

    /** @brief Precise ECEF X [m] = ecefX [cm] + ecefXHp [0.1 mm]. */
    CASTLE_NODISCARD double ecef_x_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return precise_m(ecef_x, ecef_x_hp);
    }

    /** @brief Precise ECEF Y [m] = ecefY [cm] + ecefYHp [0.1 mm]. */
    CASTLE_NODISCARD double ecef_y_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return precise_m(ecef_y, ecef_y_hp);
    }

    /** @brief Precise ECEF Z [m] = ecefZ [cm] + ecefZHp [0.1 mm]. */
    CASTLE_NODISCARD double ecef_z_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return precise_m(ecef_z, ecef_z_hp);
    }

    CASTLE_NODISCARD double p_acc_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(p_acc) * castle::math::pow10< -4, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.reserved(3U);
        stream.field(self.i_tow);
        stream.field(self.ecef_x);
        stream.field(self.ecef_y);
        stream.field(self.ecef_z);
        stream.field(self.ecef_x_hp);
        stream.field(self.ecef_y_hp);
        stream.field(self.ecef_z_hp);
        stream.field(self.flags);
        stream.field(self.p_acc);
    }

private:
    static double precise_m(int32_t cm, int8_t hp_0_1mm) CASTLE_NOEXCEPT
    {
        return (static_cast<double>(cm) * castle::math::pow10< -2, double>())
            + (static_cast<double>(hp_0_1mm) * castle::math::pow10< -4, double>());
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_HPPOSECEF_HPP
