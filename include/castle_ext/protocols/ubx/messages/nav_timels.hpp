// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_timels.hpp
 * @brief UBX-NAV-TIMELS (0x01 0x26): leap second event information.
 *
 * Periodic/polled output describing the current leap second count and the
 * upcoming (or most recent) leap second event.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMELS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMELS_HPP

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

/** @brief `srcOfCurrLs`: information source for the current number of leap seconds. */
enum class nav_timels_src_of_curr_ls : uint8_t
{
    firmware_default = 0U,  ///< Hardcoded in the firmware, can be outdated.
    gps_glonass = 1U,       ///< Derived from the GPS-GLONASS time difference.
    gps = 2U,
    sbas = 3U,
    beidou = 4U,
    galileo = 5U,
    aided_data = 6U,
    configured = 7U,
    unknown = 255U
};

/** @brief `srcOfLsChange`: information source for the future leap second event. */
enum class nav_timels_src_of_ls_change : uint8_t
{
    no_source = 0U,
    gps = 2U,
    sbas = 3U,
    beidou = 4U,
    galileo = 5U,
    glonass = 6U
};

/** @brief UBX-NAV-TIMELS output payload. */
struct nav_timels : detail::fixed_message<nav_timels, UBX_CLASS_NAV, UBX_ID_NAV_TIMELS, 24U>
{
    /** @name `valid` bits @{ */
    static CASTLE_CONSTEXPR uint8_t VALID_CURR_LS = 0x01U;           ///< `validCurrLs`.
    static CASTLE_CONSTEXPR uint8_t VALID_TIME_TO_LS_EVENT = 0x02U;  ///< `validTimeToLsEvent`.
    /** @} */

    uint32_t i_tow = 0U;    ///< GPS time of week of the navigation epoch [ms].
    uint8_t version = 0U;   ///< Message version (0x00 for this version).
    nav_timels_src_of_curr_ls src_of_curr_ls = nav_timels_src_of_curr_ls::firmware_default;
    int8_t curr_ls = 0;     ///< Leap seconds since GPS epoch (GPS - UTC) [s].
    nav_timels_src_of_ls_change src_of_ls_change = nav_timels_src_of_ls_change::no_source;
    int8_t ls_change = 0;   ///< Scheduled change: +1, -1 or 0 (none / unknown) [s].
    int32_t time_to_ls_event = 0;     ///< Seconds to (> 0) or since (< 0) the event [s].
    uint16_t date_of_ls_gps_wn = 0U;  ///< GPS week number of the event.
    uint16_t date_of_ls_gps_dn = 0U;  ///< GPS day of week of the event (GPS/GAL 1 = Sun..7 = Sat).
    uint8_t valid = 0U;     ///< Validity flags (`VALID_*`).

    CASTLE_NODISCARD bool valid_curr_ls() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_CURR_LS) != 0U;
    }

    /** @brief `time_to_ls_event`, `date_of_ls_gps_wn` and `date_of_ls_gps_dn` are valid. */
    CASTLE_NODISCARD bool valid_time_to_ls_event() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_TIME_TO_LS_EVENT) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.version);
        stream.reserved(3U);
        stream.field(self.src_of_curr_ls);
        stream.field(self.curr_ls);
        stream.field(self.src_of_ls_change);
        stream.field(self.ls_change);
        stream.field(self.time_to_ls_event);
        stream.field(self.date_of_ls_gps_wn);
        stream.field(self.date_of_ls_gps_dn);
        stream.reserved(3U);
        stream.field(self.valid);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMELS_HPP
