// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_pvt.hpp
 * @brief UBX-NAV-PVT (0x01 0x07): navigation position velocity time solution.
 *
 * Periodic/polled output combining position, velocity and time with accuracy
 * figures. UBX-NAV2-PVT (0x29 0x07) carries the identical 92-byte payload, so
 * both are one template, `basic_nav_pvt<Class, Id>`; `nav_pvt` is the NAV
 * instance and `nav2_pvt` (nav2_pvt.hpp) the secondary-output instance.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_PVT_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_PVT_HPP

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

/** @brief GNSS fix type (`fixType`). */
enum class nav_pvt_fix_type : uint8_t
{
    no_fix = 0U,     ///< No fix.
    dead_reck = 1U,  ///< Dead reckoning only.
    fix_2d = 2U,     ///< 2D fix.
    fix_3d = 3U,     ///< 3D fix.
    gnss_dr = 4U,    ///< GNSS + dead reckoning combined.
    time_only = 5U   ///< Time only fix.
};

/** @brief Power save mode state (`flags.psmState`). */
enum class nav_pvt_psm_state : uint8_t
{
    not_active = 0U,                ///< PSM is not active.
    enabled = 1U,                   ///< Intermediate state before acquisition.
    acquisition = 2U,               ///< Acquisition.
    tracking = 3U,                  ///< Tracking.
    power_optimized_tracking = 4U,  ///< Power optimized tracking.
    inactive = 5U                   ///< Inactive.
};

/** @brief Carrier phase range solution status (`flags.carrSoln`). */
enum class nav_pvt_carr_soln : uint8_t
{
    none = 0U,      ///< No carrier phase range solution.
    floating = 1U,  ///< Carrier phase solution with floating ambiguities.
    fixed = 2U      ///< Carrier phase solution with fixed ambiguities.
};

/**
 * @brief NAV/NAV2-PVT payload, shared by both classes.
 * @tparam MsgClass `UBX_CLASS_NAV` or `UBX_CLASS_NAV2`.
 * @tparam MsgId `UBX_ID_NAV_PVT` or `UBX_ID_NAV2_PVT`.
 */
template <uint8_t MsgClass, uint8_t MsgId>
struct basic_nav_pvt : detail::fixed_message<basic_nav_pvt<MsgClass, MsgId>, MsgClass, MsgId, 92U>
{
    static CASTLE_CONSTEXPR uint8_t VALID_DATE = 0x01U;     ///< `valid.validDate`.
    static CASTLE_CONSTEXPR uint8_t VALID_TIME = 0x02U;     ///< `valid.validTime`.
    static CASTLE_CONSTEXPR uint8_t VALID_FULLY = 0x04U;    ///< `valid.fullyResolved`.
    static CASTLE_CONSTEXPR uint8_t VALID_MAG_DEC = 0x08U;  ///< `valid.validMag`.

    static CASTLE_CONSTEXPR uint8_t FLAGS_GNSS_FIX_OK = 0x01U;     ///< `flags.gnssFixOK`.
    static CASTLE_CONSTEXPR uint8_t FLAGS_DIFF_SOLN = 0x02U;       ///< `flags.diffSoln`.
    static CASTLE_CONSTEXPR uint8_t FLAGS_PSM_STATE_MASK = 0x1CU;  ///< `flags.psmState` (bits 4..2).
    static CASTLE_CONSTEXPR uint8_t FLAGS_PSM_STATE_SHIFT = 2U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_HEAD_VEH_VALID = 0x20U;  ///< `flags.headVehValid`.
    static CASTLE_CONSTEXPR uint8_t FLAGS_CARR_SOLN_MASK = 0xC0U;  ///< `flags.carrSoln` (bits 7..6).
    static CASTLE_CONSTEXPR uint8_t FLAGS_CARR_SOLN_SHIFT = 6U;

    static CASTLE_CONSTEXPR uint8_t FLAGS2_CONFIRMED_AVAI = 0x20U;  ///< `flags2.confirmedAvai`.
    static CASTLE_CONSTEXPR uint8_t FLAGS2_CONFIRMED_DATE = 0x40U;  ///< `flags2.confirmedDate`.
    static CASTLE_CONSTEXPR uint8_t FLAGS2_CONFIRMED_TIME = 0x80U;  ///< `flags2.confirmedTime`.

    static CASTLE_CONSTEXPR uint8_t FLAGS3_INVALID_LLH = 0x01U;  ///< `flags3.invalidLlh`.

    uint32_t i_tow = 0U;    ///< GPS time of week [ms].
    uint16_t year = 0U;     ///< Year (UTC) [y].
    uint8_t month = 0U;     ///< Month, 1..12 (UTC).
    uint8_t day = 0U;       ///< Day of month, 1..31 (UTC).
    uint8_t hour = 0U;      ///< Hour of day, 0..23 (UTC).
    uint8_t minute = 0U;    ///< Minute of hour, 0..59 (UTC) (`min`).
    uint8_t second = 0U;    ///< Seconds of minute, 0..60 (UTC) (`sec`).
    uint8_t valid = 0U;     ///< Validity flags, see `VALID_*`.
    uint32_t t_acc = 0U;    ///< Time accuracy estimate (UTC) [ns].
    int32_t nano = 0;       ///< Fraction of second, -1e9..1e9 (UTC) [ns].
    nav_pvt_fix_type fix_type = nav_pvt_fix_type::no_fix;  ///< GNSS fix type.
    uint8_t flags = 0U;     ///< Fix status flags, see `FLAGS_*`.
    uint8_t flags2 = 0U;    ///< Additional flags, see `FLAGS2_*`.
    uint8_t num_sv = 0U;    ///< Satellites used in the navigation solution.
    int32_t lon = 0;        ///< Longitude [1e-7 deg].
    int32_t lat = 0;        ///< Latitude [1e-7 deg].
    int32_t height = 0;     ///< Height above ellipsoid [mm].
    int32_t h_msl = 0;      ///< Height above mean sea level [mm].
    uint32_t h_acc = 0U;    ///< Horizontal accuracy estimate [mm].
    uint32_t v_acc = 0U;    ///< Vertical accuracy estimate [mm].
    int32_t vel_n = 0;      ///< NED north velocity [mm/s].
    int32_t vel_e = 0;      ///< NED east velocity [mm/s].
    int32_t vel_d = 0;      ///< NED down velocity [mm/s].
    int32_t g_speed = 0;    ///< Ground speed (2-D) [mm/s].
    int32_t head_mot = 0;   ///< Heading of motion (2-D) [1e-5 deg].
    uint32_t s_acc = 0U;    ///< Speed accuracy estimate [mm/s].
    uint32_t head_acc = 0U; ///< Heading accuracy estimate (motion and vehicle) [1e-5 deg].
    uint16_t p_dop = 0U;    ///< Position DOP [0.01].
    uint8_t flags3 = 0U;    ///< Additional flags, see `FLAGS3_*`.
    int32_t head_veh = 0;   ///< Heading of vehicle (2-D) [1e-5 deg].
    int16_t mag_dec = 0;    ///< Magnetic declination [1e-2 deg].
    uint16_t mag_acc = 0U;  ///< Magnetic declination accuracy [1e-2 deg].

    CASTLE_NODISCARD bool valid_date() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_DATE) != 0U;
    }

    CASTLE_NODISCARD bool valid_time() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_TIME) != 0U;
    }

    CASTLE_NODISCARD bool fully_resolved() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_FULLY) != 0U;
    }

    CASTLE_NODISCARD bool valid_mag() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_MAG_DEC) != 0U;
    }

    CASTLE_NODISCARD bool fix_ok() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_GNSS_FIX_OK) != 0U;
    }

    CASTLE_NODISCARD bool diff_soln() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_DIFF_SOLN) != 0U;
    }

    CASTLE_NODISCARD bool head_veh_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_HEAD_VEH_VALID) != 0U;
    }

    CASTLE_NODISCARD nav_pvt_psm_state psm_state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_pvt_psm_state>((flags & FLAGS_PSM_STATE_MASK) >> FLAGS_PSM_STATE_SHIFT);
    }

    CASTLE_NODISCARD nav_pvt_carr_soln carr_soln() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_pvt_carr_soln>((flags & FLAGS_CARR_SOLN_MASK) >> FLAGS_CARR_SOLN_SHIFT);
    }

    CASTLE_NODISCARD bool confirmed_avai() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags2 & FLAGS2_CONFIRMED_AVAI) != 0U;
    }

    CASTLE_NODISCARD bool confirmed_date() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags2 & FLAGS2_CONFIRMED_DATE) != 0U;
    }

    CASTLE_NODISCARD bool confirmed_time() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags2 & FLAGS2_CONFIRMED_TIME) != 0U;
    }

    CASTLE_NODISCARD bool invalid_llh() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags3 & FLAGS3_INVALID_LLH) != 0U;
    }

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

    CASTLE_NODISCARD double h_acc_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(h_acc) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double v_acc_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(v_acc) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double vel_n_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(vel_n) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double vel_e_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(vel_e) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double vel_d_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(vel_d) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double g_speed_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(g_speed) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double s_acc_m_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(s_acc) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double head_mot_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(head_mot) * castle::math::pow10< -5, double>();
    }

    CASTLE_NODISCARD double head_acc_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(head_acc) * castle::math::pow10< -5, double>();
    }

    CASTLE_NODISCARD double head_veh_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(head_veh) * castle::math::pow10< -5, double>();
    }

    CASTLE_NODISCARD double pdop_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(p_dop) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double mag_dec_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(mag_dec) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double mag_acc_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(mag_acc) * castle::math::pow10< -2, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.year);
        stream.field(self.month);
        stream.field(self.day);
        stream.field(self.hour);
        stream.field(self.minute);
        stream.field(self.second);
        stream.field(self.valid);
        stream.field(self.t_acc);
        stream.field(self.nano);
        stream.field(self.fix_type);
        stream.field(self.flags);
        stream.field(self.flags2);
        stream.field(self.num_sv);
        stream.field(self.lon);
        stream.field(self.lat);
        stream.field(self.height);
        stream.field(self.h_msl);
        stream.field(self.h_acc);
        stream.field(self.v_acc);
        stream.field(self.vel_n);
        stream.field(self.vel_e);
        stream.field(self.vel_d);
        stream.field(self.g_speed);
        stream.field(self.head_mot);
        stream.field(self.s_acc);
        stream.field(self.head_acc);
        stream.field(self.p_dop);
        stream.field(self.flags3);
        stream.reserved(5U);
        stream.field(self.head_veh);
        stream.field(self.mag_dec);
        stream.field(self.mag_acc);
    }
};

/** @brief UBX-NAV-PVT (0x01 0x07). */
using nav_pvt = basic_nav_pvt<UBX_CLASS_NAV, UBX_ID_NAV_PVT>;

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_PVT_HPP
