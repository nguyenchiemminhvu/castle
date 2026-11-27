// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_timebds.hpp
 * @brief UBX-NAV-TIMEBDS (0x01 0x24): BeiDou time solution.
 *
 * Periodic/polled output. Shares its 20-byte layout with UBX-NAV-TIMEGAL and
 * UBX-NAV-TIMEQZSS through `detail::nav_gnss_time`; the spec names `SOW` and
 * `fSOW` are exposed as `sow()` / `f_sow()` over the generic `tow` / `f_tow`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEBDS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEBDS_HPP

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

/** @brief UBX-NAV-TIMEBDS output payload. */
struct nav_timebds : detail::nav_gnss_time<nav_timebds, UBX_ID_NAV_TIMEBDS>
{
    /** @name `valid` bits (spec names) @{ */
    static CASTLE_CONSTEXPR uint8_t VALID_SOW = VALID_TOW;  ///< `sowValid`: SOW and fSOW are valid.
    /** @} */

    /** @brief BDS time of week `SOW`, rounded [s]. */
    CASTLE_NODISCARD uint32_t sow() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tow;
    }

    /** @brief Fractional part of SOW `fSOW` [ns]. */
    CASTLE_NODISCARD int32_t f_sow() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return f_tow;
    }

    /** @brief `sowValid`. */
    CASTLE_NODISCARD bool sow_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tow_valid();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEBDS_HPP
