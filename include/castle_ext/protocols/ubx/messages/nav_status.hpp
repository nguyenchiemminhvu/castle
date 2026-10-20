// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_status.hpp
 * @brief UBX-NAV-STATUS (0x01 0x03): receiver navigation status.
 *
 * Periodic/polled output. `gps_fix` alone does not qualify a fix as valid;
 * check `fix_ok()` (`gpsFixOk`) as well.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_STATUS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_STATUS_HPP

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

/** @brief GPS fix type (`gpsFix`); 0x06..0xFF are reserved. */
enum class nav_status_gps_fix : uint8_t
{
    no_fix = 0U,     ///< No fix.
    dead_reck = 1U,  ///< Dead reckoning only.
    fix_2d = 2U,     ///< 2D fix.
    fix_3d = 3U,     ///< 3D fix.
    gnss_dr = 4U,    ///< GPS + dead reckoning combined.
    time_only = 5U   ///< Time only fix.
};

/** @brief Map matching status (`fixStat.mapMatching`). */
enum class nav_status_map_matching : uint8_t
{
    none = 0U,            ///< No map matching.
    valid_not_used = 1U,  ///< Map matching data received but too old.
    valid_used = 2U,      ///< Map matching data applied.
    valid_used_dr = 3U    ///< Applied; enables dead reckoning when sensors are unavailable.
};

/** @brief Power save mode state (`flags2.psmState`). */
enum class nav_status_psm_state : uint8_t
{
    acquisition = 0U,               ///< Acquisition, or PSM disabled.
    tracking = 1U,                  ///< Tracking.
    power_optimized_tracking = 2U,  ///< Power optimized tracking.
    inactive = 3U                   ///< Inactive.
};

/** @brief Spoofing detection state (`flags2.spoofDetState`). */
enum class nav_status_spoof_det_state : uint8_t
{
    unknown = 0U,       ///< Unknown or deactivated.
    no_spoofing = 1U,   ///< No spoofing indicated.
    spoofing = 2U,      ///< Spoofing indicated.
    multiple = 3U       ///< Multiple spoofing indications.
};

/** @brief Carrier phase range solution status (`flags2.carrSoln`). */
enum class nav_status_carr_soln : uint8_t
{
    none = 0U,      ///< No carrier phase range solution.
    floating = 1U,  ///< Carrier phase solution with floating ambiguities.
    fixed = 2U      ///< Carrier phase solution with fixed ambiguities.
};

/** @brief UBX-NAV-STATUS output payload. */
struct nav_status : detail::fixed_message<nav_status, UBX_CLASS_NAV, UBX_ID_NAV_STATUS, 16U>
{
    static CASTLE_CONSTEXPR uint8_t FLAGS_GPS_FIX_OK = 0x01U;  ///< `flags.gpsFixOk`.
    static CASTLE_CONSTEXPR uint8_t FLAGS_DIFF_SOLN = 0x02U;   ///< `flags.diffSoln`.
    static CASTLE_CONSTEXPR uint8_t FLAGS_WKN_SET = 0x04U;     ///< `flags.wknSet`.
    static CASTLE_CONSTEXPR uint8_t FLAGS_TOW_SET = 0x08U;     ///< `flags.towSet`.

    static CASTLE_CONSTEXPR uint8_t FIX_STAT_DIFF_CORR = 0x01U;          ///< `fixStat.diffCorr`.
    static CASTLE_CONSTEXPR uint8_t FIX_STAT_CARR_SOLN_VALID = 0x02U;    ///< `fixStat.carrSolnValid`.
    static CASTLE_CONSTEXPR uint8_t FIX_STAT_MAP_MATCHING_MASK = 0xC0U;  ///< `fixStat.mapMatching` (bits 7..6).
    static CASTLE_CONSTEXPR uint8_t FIX_STAT_MAP_MATCHING_SHIFT = 6U;

    static CASTLE_CONSTEXPR uint8_t FLAGS2_PSM_STATE_MASK = 0x03U;        ///< `flags2.psmState` (bits 1..0).
    static CASTLE_CONSTEXPR uint8_t FLAGS2_PSM_STATE_SHIFT = 0U;
    static CASTLE_CONSTEXPR uint8_t FLAGS2_SPOOF_DET_STATE_MASK = 0x18U;  ///< `flags2.spoofDetState` (bits 4..3).
    static CASTLE_CONSTEXPR uint8_t FLAGS2_SPOOF_DET_STATE_SHIFT = 3U;
    static CASTLE_CONSTEXPR uint8_t FLAGS2_CARR_SOLN_MASK = 0xC0U;        ///< `flags2.carrSoln` (bits 7..6).
    static CASTLE_CONSTEXPR uint8_t FLAGS2_CARR_SOLN_SHIFT = 6U;

    uint32_t i_tow = 0U;    ///< GPS time of week [ms].
    nav_status_gps_fix gps_fix = nav_status_gps_fix::no_fix;  ///< GPS fix type.
    uint8_t flags = 0U;     ///< Navigation status flags, see `FLAGS_*`.
    uint8_t fix_stat = 0U;  ///< Fix status information, see `FIX_STAT_*`.
    uint8_t flags2 = 0U;    ///< Further navigation output information, see `FLAGS2_*`.
    uint32_t ttff = 0U;     ///< Time to first fix [ms].
    uint32_t msss = 0U;     ///< Milliseconds since startup/reset [ms].

    CASTLE_NODISCARD bool fix_ok() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_GPS_FIX_OK) != 0U;
    }

    CASTLE_NODISCARD bool diff_soln() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_DIFF_SOLN) != 0U;
    }

    CASTLE_NODISCARD bool wkn_set() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_WKN_SET) != 0U;
    }

    CASTLE_NODISCARD bool tow_set() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_TOW_SET) != 0U;
    }

    CASTLE_NODISCARD bool diff_corr() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (fix_stat & FIX_STAT_DIFF_CORR) != 0U;
    }

    CASTLE_NODISCARD bool carr_soln_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (fix_stat & FIX_STAT_CARR_SOLN_VALID) != 0U;
    }

    CASTLE_NODISCARD nav_status_map_matching map_matching() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_status_map_matching>(
            (fix_stat & FIX_STAT_MAP_MATCHING_MASK) >> FIX_STAT_MAP_MATCHING_SHIFT);
    }

    CASTLE_NODISCARD nav_status_psm_state psm_state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_status_psm_state>((flags2 & FLAGS2_PSM_STATE_MASK) >> FLAGS2_PSM_STATE_SHIFT);
    }

    CASTLE_NODISCARD nav_status_spoof_det_state spoof_det_state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_status_spoof_det_state>(
            (flags2 & FLAGS2_SPOOF_DET_STATE_MASK) >> FLAGS2_SPOOF_DET_STATE_SHIFT);
    }

    CASTLE_NODISCARD nav_status_carr_soln carr_soln() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_status_carr_soln>((flags2 & FLAGS2_CARR_SOLN_MASK) >> FLAGS2_CARR_SOLN_SHIFT);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.gps_fix);
        stream.field(self.flags);
        stream.field(self.fix_stat);
        stream.field(self.flags2);
        stream.field(self.ttff);
        stream.field(self.msss);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_STATUS_HPP
