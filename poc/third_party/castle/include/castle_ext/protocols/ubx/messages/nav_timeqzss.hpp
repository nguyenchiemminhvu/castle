// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_timeqzss.hpp
 * @brief UBX-NAV-TIMEQZSS (0x01 0x27): QZSS time solution.
 *
 * Periodic/polled output. Shares its 20-byte layout with UBX-NAV-TIMEBDS and
 * UBX-NAV-TIMEGAL through `detail::nav_gnss_time`; the spec names `qzssTow`,
 * `fQzssTow` and `qzssWno` are exposed as `qzss_tow()` / `f_qzss_tow()` /
 * `qzss_wno()` over the generic `tow` / `f_tow` / `week`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEQZSS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEQZSS_HPP

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

/** @brief UBX-NAV-TIMEQZSS output payload. */
struct nav_timeqzss : detail::nav_gnss_time<nav_timeqzss, UBX_ID_NAV_TIMEQZSS>
{
    /** @name `valid` bits (spec names) @{ */
    static CASTLE_CONSTEXPR uint8_t VALID_QZSS_TOW = VALID_TOW;   ///< `qzssTowValid`: qzssTow and fQzssTow are valid.
    static CASTLE_CONSTEXPR uint8_t VALID_QZSS_WNO = VALID_WEEK;  ///< `qzssWnoValid`: qzssWno is valid.
    /** @} */

    /** @brief QZSS time of week `qzssTow`, rounded [s]. */
    CASTLE_NODISCARD uint32_t qzss_tow() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tow;
    }

    /** @brief Fractional part of the QZSS time of week `fQzssTow` [ns]. */
    CASTLE_NODISCARD int32_t f_qzss_tow() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return f_tow;
    }

    /** @brief QZSS week number `qzssWno`. */
    CASTLE_NODISCARD int16_t qzss_wno() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return week;
    }

    /** @brief `qzssTowValid`. */
    CASTLE_NODISCARD bool qzss_tow_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tow_valid();
    }

    /** @brief `qzssWnoValid`. */
    CASTLE_NODISCARD bool qzss_wno_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return week_valid();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEQZSS_HPP
