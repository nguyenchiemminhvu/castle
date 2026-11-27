// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_hpposllh.hpp
 * @brief UBX-NAV-HPPOSLLH (0x01 0x14): high-precision geodetic position solution.
 *
 * Periodic/polled output in the currently selected ellipsoid (WGS84 by default).
 * Angles are split into a 1e-7 deg part and a 1e-9 deg high-precision part
 * (-99..+99); heights into a mm part and a 0.1 mm part (-9..+9).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_HPPOSLLH_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_HPPOSLLH_HPP

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

/** @brief UBX-NAV-HPPOSLLH output payload. */
struct nav_hpposllh : detail::fixed_message<nav_hpposllh, UBX_CLASS_NAV, UBX_ID_NAV_HPPOSLLH, 36U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_INVALID_LLH = 0x01U; ///< lon/lat/height/hMSL (incl. HP parts) invalid.
    /** @} */

    uint8_t version = 0U;  ///< Message version (0x00 for this layout).
    uint8_t flags = 0U;    ///< Additional flags, see `FLAGS_*`.
    uint32_t i_tow = 0U;   ///< GPS time of week [ms].
    int32_t lon = 0;       ///< Longitude [1e-7 deg].
    int32_t lat = 0;       ///< Latitude [1e-7 deg].
    int32_t height = 0;    ///< Height above ellipsoid [mm].
    int32_t h_msl = 0;     ///< Height above mean sea level [mm].
    int8_t lon_hp = 0;     ///< High-precision part of longitude [1e-9 deg], -99..+99.
    int8_t lat_hp = 0;     ///< High-precision part of latitude [1e-9 deg], -99..+99.
    int8_t height_hp = 0;  ///< High-precision part of height [0.1 mm], -9..+9.
    int8_t h_msl_hp = 0;   ///< High-precision part of hMSL [0.1 mm], -9..+9.
    uint32_t h_acc = 0U;   ///< Horizontal accuracy estimate [0.1 mm].
    uint32_t v_acc = 0U;   ///< Vertical accuracy estimate [0.1 mm].

    CASTLE_NODISCARD bool invalid_llh() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_INVALID_LLH) != 0U;
    }

    /** @brief Precise longitude [deg] = lon [1e-7 deg] + lonHp [1e-9 deg]. */
    CASTLE_NODISCARD double longitude_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return precise_deg(lon, lon_hp);
    }

    /** @brief Precise latitude [deg] = lat [1e-7 deg] + latHp [1e-9 deg]. */
    CASTLE_NODISCARD double latitude_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return precise_deg(lat, lat_hp);
    }

    /** @brief Precise height above ellipsoid [m] = height [mm] + heightHp [0.1 mm]. */
    CASTLE_NODISCARD double height_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return precise_m(height, height_hp);
    }

    /** @brief Precise height above mean sea level [m] = hMSL [mm] + hMSLHp [0.1 mm]. */
    CASTLE_NODISCARD double h_msl_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return precise_m(h_msl, h_msl_hp);
    }

    CASTLE_NODISCARD double h_acc_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(h_acc) * castle::math::pow10< -4, double>();
    }

    CASTLE_NODISCARD double v_acc_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(v_acc) * castle::math::pow10< -4, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.reserved(2U);
        stream.field(self.flags);
        stream.field(self.i_tow);
        stream.field(self.lon);
        stream.field(self.lat);
        stream.field(self.height);
        stream.field(self.h_msl);
        stream.field(self.lon_hp);
        stream.field(self.lat_hp);
        stream.field(self.height_hp);
        stream.field(self.h_msl_hp);
        stream.field(self.h_acc);
        stream.field(self.v_acc);
    }

private:
    static double precise_deg(int32_t deg_1e7, int8_t hp_deg_1e9) CASTLE_NOEXCEPT
    {
        return (static_cast<double>(deg_1e7) * castle::math::pow10< -7, double>())
            + (static_cast<double>(hp_deg_1e9) * castle::math::pow10< -9, double>());
    }

    static double precise_m(int32_t mm, int8_t hp_0_1mm) CASTLE_NOEXCEPT
    {
        return (static_cast<double>(mm) * castle::math::pow10< -3, double>())
            + (static_cast<double>(hp_0_1mm) * castle::math::pow10< -4, double>());
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_HPPOSLLH_HPP
