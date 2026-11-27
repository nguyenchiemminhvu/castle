// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_timegps.hpp
 * @brief UBX-NAV-TIMEGPS (0x01 0x20): GPS time solution.
 *
 * Periodic/polled output. The 16-byte layout is shared bit-for-bit with
 * UBX-NAV2-TIMEGPS (0x29 0x20), so the model is templated on class and ID
 * (`basic_nav_timegps<Class, Id>`); see `nav2_timegps.hpp`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGPS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGPS_HPP

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

/** @brief GPS time solution payload, shared by UBX-NAV-TIMEGPS and UBX-NAV2-TIMEGPS. */
template <uint8_t MsgClass, uint8_t MsgId>
struct basic_nav_timegps
    : detail::fixed_message<basic_nav_timegps<MsgClass, MsgId>, MsgClass, MsgId, 16U>
{
    /** @name `valid` bits @{ */
    static CASTLE_CONSTEXPR uint8_t VALID_TOW = 0x01U;     ///< `towValid`: iTOW and fTOW are valid.
    static CASTLE_CONSTEXPR uint8_t VALID_WEEK = 0x02U;    ///< `weekValid`: week is valid.
    static CASTLE_CONSTEXPR uint8_t VALID_LEAP_S = 0x04U;  ///< `leapSValid`: leapS is valid.
    /** @} */

    uint32_t i_tow = 0U;  ///< GPS time of week of the navigation epoch [ms].
    int32_t f_tow = 0;    ///< Fractional part of `i_tow`, +/-500000 [ns].
    int16_t week = 0;     ///< GPS week number of the navigation epoch.
    int8_t leap_s = 0;    ///< GPS leap seconds (GPS - UTC) [s].
    uint8_t valid = 0U;   ///< Validity flags (`VALID_*`).
    uint32_t t_acc = 0U;  ///< Time accuracy estimate [ns].

    CASTLE_NODISCARD bool tow_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_TOW) != 0U;
    }

    CASTLE_NODISCARD bool week_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_WEEK) != 0U;
    }

    CASTLE_NODISCARD bool leap_s_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_LEAP_S) != 0U;
    }

    /** @brief Precise GPS time of week `i_tow * 1e-3 + f_tow * 1e-9` [s]. */
    CASTLE_NODISCARD double time_of_week_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (static_cast<double>(i_tow) * castle::math::pow10< -3, double>()) +
               (static_cast<double>(f_tow) * castle::math::pow10< -9, double>());
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.f_tow);
        stream.field(self.week);
        stream.field(self.leap_s);
        stream.field(self.valid);
        stream.field(self.t_acc);
    }
};

/** @brief UBX-NAV-TIMEGPS. */
using nav_timegps = basic_nav_timegps<UBX_CLASS_NAV, UBX_ID_NAV_TIMEGPS>;

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGPS_HPP
