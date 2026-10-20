// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_timegal.hpp
 * @brief UBX-NAV-TIMEGAL (0x01 0x25): Galileo time solution.
 *
 * Periodic/polled output. Shares its 20-byte layout with UBX-NAV-TIMEBDS and
 * UBX-NAV-TIMEQZSS through `detail::nav_gnss_time`; the spec names `galTow`,
 * `fGalTow` and `galWno` are exposed as `gal_tow()` / `f_gal_tow()` /
 * `gal_wno()` over the generic `tow` / `f_tow` / `week`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGAL_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGAL_HPP

#include "castle/core/compiler.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/detail/nav_gnss_time.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-NAV-TIMEGAL output payload. */
struct nav_timegal : detail::nav_gnss_time<nav_timegal, UBX_ID_NAV_TIMEGAL>
{
    /** @name `valid` bits (spec names) @{ */
    static CASTLE_CONSTEXPR uint8_t VALID_GAL_TOW = VALID_TOW;   ///< `galTowValid`: galTow and fGalTow are valid.
    static CASTLE_CONSTEXPR uint8_t VALID_GAL_WNO = VALID_WEEK;  ///< `galWnoValid`: galWno is valid.
    /** @} */

    /** @brief Galileo time of week `galTow`, rounded [s]. */
    CASTLE_NODISCARD uint32_t gal_tow() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tow;
    }

    /** @brief Fractional part of the Galileo time of week `fGalTow` [ns]. */
    CASTLE_NODISCARD int32_t f_gal_tow() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return f_tow;
    }

    /** @brief Galileo week number `galWno`. */
    CASTLE_NODISCARD int16_t gal_wno() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return week;
    }

    /** @brief `galTowValid`. */
    CASTLE_NODISCARD bool gal_tow_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tow_valid();
    }

    /** @brief `galWnoValid`. */
    CASTLE_NODISCARD bool gal_wno_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return week_valid();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGAL_HPP
