// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mga_ini.hpp
 * @brief UBX-MGA-INI (0x13 0x40): initial position, time, clock drift and frequency assistance.
 *
 * Input messages sent by the host. One class/ID carries six layouts, told
 * apart by payload byte 0 (`type`), so each layout is its own model built on
 * `detail::typed_message`. Layouts that share a length (POS_XYZ/POS_LLH,
 * TIME_UTC/TIME_GNSS, CLKD/FREQ) are still dispatched exclusively by `type`.
 *
 * @code
 * castle::protocols::ubx::messages::mga_ini_pos_llh pos{};
 * pos.lat = 473977000;
 * pos.lon = 85456000;
 * pos.pos_acc = 10000U;
 * uint8_t frame[32U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(pos, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_INI_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_INI_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/detail/typed_message.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief `type` byte values of the UBX-MGA-INI layouts. */
static CASTLE_CONSTEXPR uint8_t MGA_INI_TYPE_POS_XYZ = 0x00U;
static CASTLE_CONSTEXPR uint8_t MGA_INI_TYPE_POS_LLH = 0x01U;
static CASTLE_CONSTEXPR uint8_t MGA_INI_TYPE_TIME_UTC = 0x10U;
static CASTLE_CONSTEXPR uint8_t MGA_INI_TYPE_TIME_GNSS = 0x11U;
static CASTLE_CONSTEXPR uint8_t MGA_INI_TYPE_CLKD = 0x20U;
static CASTLE_CONSTEXPR uint8_t MGA_INI_TYPE_FREQ = 0x21U;

/** @brief Message version byte for every MGA-INI layout in this revision. */
static CASTLE_CONSTEXPR uint8_t MGA_INI_VERSION = 0x00U;

/** @brief `ref.source` of the TIME_UTC / TIME_GNSS layouts (3-15 reserved). */
enum class mga_ini_time_source : uint8_t
{
    none = 0U,    ///< On receipt of the message (inaccurate).
    extint0 = 1U, ///< Relative to the pulse sent to EXTINT0.
    extint1 = 2U  ///< Relative to the pulse sent to EXTINT1.
};

/** @brief `flags.source` of the FREQ layout (2-15 reserved). */
enum class mga_ini_freq_source : uint8_t
{
    extint0 = 0U, ///< Frequency available on EXTINT0.
    extint1 = 1U  ///< Frequency available on EXTINT1.
};

/** @brief `gnssId` time bases supported by TIME_GNSS. */
enum class mga_ini_gnss_id : uint8_t
{
    gps = 0U,
    galileo = 2U,
    beidou = 3U,
    glonass = 6U
};

/** @brief UBX-MGA-INI-POS_XYZ: initial WGS84 ECEF position (20 bytes). */
struct mga_ini_pos_xyz
    : detail::typed_message<mga_ini_pos_xyz, UBX_CLASS_MGA, UBX_ID_MGA_INI, MGA_INI_TYPE_POS_XYZ, 20U>
{
    int32_t ecef_x = 0;     ///< [cm].
    int32_t ecef_y = 0;     ///< [cm].
    int32_t ecef_z = 0;     ///< [cm].
    uint32_t pos_acc = 0U;  ///< Position accuracy (stddev) [cm].

    CASTLE_NODISCARD double ecef_x_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(ecef_x) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double ecef_y_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(ecef_y) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double ecef_z_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(ecef_z) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double pos_acc_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(pos_acc) * castle::math::pow10< -2, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_INI_TYPE_POS_XYZ);
        stream.constant(MGA_INI_VERSION);
        stream.reserved(2U);
        stream.field(self.ecef_x);
        stream.field(self.ecef_y);
        stream.field(self.ecef_z);
        stream.field(self.pos_acc);
    }
};

/** @brief UBX-MGA-INI-POS_LLH: initial WGS84 geodetic position (20 bytes). */
struct mga_ini_pos_llh
    : detail::typed_message<mga_ini_pos_llh, UBX_CLASS_MGA, UBX_ID_MGA_INI, MGA_INI_TYPE_POS_LLH, 20U>
{
    int32_t lat = 0;        ///< Latitude [1e-7 deg].
    int32_t lon = 0;        ///< Longitude [1e-7 deg].
    int32_t alt = 0;        ///< Altitude [cm].
    uint32_t pos_acc = 0U;  ///< Position accuracy (stddev) [cm].

    CASTLE_NODISCARD double latitude_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(lat) * castle::math::pow10< -7, double>();
    }

    CASTLE_NODISCARD double longitude_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(lon) * castle::math::pow10< -7, double>();
    }

    CASTLE_NODISCARD double alt_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(alt) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double pos_acc_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(pos_acc) * castle::math::pow10< -2, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_INI_TYPE_POS_LLH);
        stream.constant(MGA_INI_VERSION);
        stream.reserved(2U);
        stream.field(self.lat);
        stream.field(self.lon);
        stream.field(self.alt);
        stream.field(self.pos_acc);
    }
};

/** @brief UBX-MGA-INI-TIME_UTC: initial UTC time (24 bytes). */
struct mga_ini_time_utc
    : detail::typed_message<mga_ini_time_utc, UBX_CLASS_MGA, UBX_ID_MGA_INI, MGA_INI_TYPE_TIME_UTC, 24U>
{
    /** @name `ref` bits @{ */
    static CASTLE_CONSTEXPR uint8_t REF_SOURCE = 0x0FU;
    static CASTLE_CONSTEXPR uint8_t REF_FALL = 0x10U;
    static CASTLE_CONSTEXPR uint8_t REF_LAST = 0x20U;
    /** @} */

    /** @brief `leap_secs` value meaning "unknown". */
    static CASTLE_CONSTEXPR int8_t LEAP_SECS_UNKNOWN = -128;

    uint8_t ref = 0U;           ///< Time reference (`REF_*` bits).
    int8_t leap_secs = 0;       ///< Leap seconds since 1980, or `LEAP_SECS_UNKNOWN` [s].
    uint16_t year = 0U;
    uint8_t month = 0U;         ///< 1..12.
    uint8_t day = 0U;           ///< 1..31.
    uint8_t hour = 0U;          ///< 0..23.
    uint8_t minute = 0U;        ///< 0..59.
    uint8_t second = 0U;        ///< 0..59 [s].
    uint32_t ns = 0U;           ///< 0..999 999 999 [ns].
    uint16_t t_acc_s = 0U;      ///< Seconds part of time accuracy [s].
    uint32_t t_acc_ns = 0U;     ///< Nanoseconds part of time accuracy [ns].

    CASTLE_NODISCARD mga_ini_time_source source() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<mga_ini_time_source>(ref & REF_SOURCE);
    }

    /** @brief Use the falling EXTINT edge (default rising). */
    CASTLE_NODISCARD bool fall() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (ref & REF_FALL) != 0U;
    }

    /** @brief Use the last EXTINT pulse (default next pulse). */
    CASTLE_NODISCARD bool last() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (ref & REF_LAST) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_INI_TYPE_TIME_UTC);
        stream.constant(MGA_INI_VERSION);
        stream.field(self.ref);
        stream.field(self.leap_secs);
        stream.field(self.year);
        stream.field(self.month);
        stream.field(self.day);
        stream.field(self.hour);
        stream.field(self.minute);
        stream.field(self.second);
        stream.reserved(1U);
        stream.field(self.ns);
        stream.field(self.t_acc_s);
        stream.reserved(2U);
        stream.field(self.t_acc_ns);
    }
};

/** @brief UBX-MGA-INI-TIME_GNSS: initial time in a GNSS time base (24 bytes). */
struct mga_ini_time_gnss
    : detail::typed_message<mga_ini_time_gnss, UBX_CLASS_MGA, UBX_ID_MGA_INI, MGA_INI_TYPE_TIME_GNSS, 24U>
{
    /** @name `ref` bits @{ */
    static CASTLE_CONSTEXPR uint8_t REF_SOURCE = 0x0FU;
    static CASTLE_CONSTEXPR uint8_t REF_FALL = 0x10U;
    static CASTLE_CONSTEXPR uint8_t REF_LAST = 0x20U;
    /** @} */

    uint8_t ref = 0U;                           ///< Time reference (`REF_*` bits).
    mga_ini_gnss_id gnss_id = mga_ini_gnss_id::gps; ///< Time base.
    uint16_t week = 0U;                         ///< GNSS week number.
    uint32_t tow = 0U;                          ///< GNSS time of week [s].
    uint32_t ns = 0U;                           ///< Sub-second part, 0..999 999 999 [ns].
    uint16_t t_acc_s = 0U;                      ///< Seconds part of time accuracy [s].
    uint32_t t_acc_ns = 0U;                     ///< Nanoseconds part of time accuracy [ns].

    CASTLE_NODISCARD mga_ini_time_source source() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<mga_ini_time_source>(ref & REF_SOURCE);
    }

    /** @brief Use the falling EXTINT edge (default rising). */
    CASTLE_NODISCARD bool fall() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (ref & REF_FALL) != 0U;
    }

    /** @brief Use the last EXTINT pulse (default next pulse). */
    CASTLE_NODISCARD bool last() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (ref & REF_LAST) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_INI_TYPE_TIME_GNSS);
        stream.constant(MGA_INI_VERSION);
        stream.field(self.ref);
        stream.field(self.gnss_id);
        stream.reserved(2U);
        stream.field(self.week);
        stream.field(self.tow);
        stream.field(self.ns);
        stream.field(self.t_acc_s);
        stream.reserved(2U);
        stream.field(self.t_acc_ns);
    }
};

/** @brief UBX-MGA-INI-CLKD: initial clock drift (12 bytes). */
struct mga_ini_clkd
    : detail::typed_message<mga_ini_clkd, UBX_CLASS_MGA, UBX_ID_MGA_INI, MGA_INI_TYPE_CLKD, 12U>
{
    int32_t clk_d = 0;          ///< Clock drift [ns/s].
    uint32_t clk_d_acc = 0U;    ///< Clock drift accuracy [ns/s].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_INI_TYPE_CLKD);
        stream.constant(MGA_INI_VERSION);
        stream.reserved(2U);
        stream.field(self.clk_d);
        stream.field(self.clk_d_acc);
    }
};

/** @brief UBX-MGA-INI-FREQ: external frequency assistance (12 bytes). */
struct mga_ini_freq
    : detail::typed_message<mga_ini_freq, UBX_CLASS_MGA, UBX_ID_MGA_INI, MGA_INI_TYPE_FREQ, 12U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_SOURCE = 0x0FU;
    static CASTLE_CONSTEXPR uint8_t FLAGS_FALL = 0x10U;
    /** @} */

    uint8_t flags = 0U;         ///< Frequency reference (`FLAGS_*` bits).
    int32_t freq = 0;           ///< Frequency [1e-2 Hz].
    uint32_t freq_acc = 0U;     ///< Frequency accuracy [ppb].

    CASTLE_NODISCARD mga_ini_freq_source source() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<mga_ini_freq_source>(flags & FLAGS_SOURCE);
    }

    /** @brief Use the falling EXTINT edge (default rising). */
    CASTLE_NODISCARD bool fall() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_FALL) != 0U;
    }

    CASTLE_NODISCARD double freq_hz() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(freq) * castle::math::pow10< -2, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_INI_TYPE_FREQ);
        stream.constant(MGA_INI_VERSION);
        stream.reserved(1U);
        stream.field(self.flags);
        stream.field(self.freq);
        stream.field(self.freq_acc);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_INI_HPP
