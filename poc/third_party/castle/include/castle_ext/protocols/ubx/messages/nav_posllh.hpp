// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_posllh.hpp
 * @brief UBX-NAV-POSLLH (0x01 0x02): geodetic position solution.
 *
 * Periodic/polled output in the currently selected ellipsoid (WGS84 by default).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_POSLLH_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_POSLLH_HPP

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

/** @brief UBX-NAV-POSLLH output payload. */
struct nav_posllh : detail::fixed_message<nav_posllh, UBX_CLASS_NAV, UBX_ID_NAV_POSLLH, 28U>
{
    uint32_t i_tow = 0U;  ///< GPS time of week [ms].
    int32_t lon = 0;      ///< Longitude [1e-7 deg].
    int32_t lat = 0;      ///< Latitude [1e-7 deg].
    int32_t height = 0;   ///< Height above ellipsoid [mm].
    int32_t h_msl = 0;    ///< Height above mean sea level [mm].
    uint32_t h_acc = 0U;  ///< Horizontal accuracy estimate [mm].
    uint32_t v_acc = 0U;  ///< Vertical accuracy estimate [mm].

    CASTLE_NODISCARD double longitude_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(lon) * castle::math::pow10< -7, double>();
    }

    CASTLE_NODISCARD double latitude_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(lat) * castle::math::pow10< -7, double>();
    }

    CASTLE_NODISCARD double height_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(height) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double h_msl_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(h_msl) * castle::math::pow10< -3, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.lon);
        stream.field(self.lat);
        stream.field(self.height);
        stream.field(self.h_msl);
        stream.field(self.h_acc);
        stream.field(self.v_acc);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_POSLLH_HPP
