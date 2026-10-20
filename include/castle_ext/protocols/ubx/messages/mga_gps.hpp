// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mga_gps.hpp
 * @brief UBX-MGA-GPS (0x13 0x00): GPS assistance data (ephemeris, almanac, health, UTC, ionosphere).
 *
 * Input messages sent by the host. One class/ID carries five layouts, told
 * apart by payload byte 0 (`type`), so each layout is its own model built on
 * `detail::typed_message`. Every model has `encode()` for transmission and a
 * `decode()` for loop-back / logging.
 *
 * The ephemeris and almanac layouts are shared bit-for-bit with UBX-MGA-QZSS
 * and are therefore templated on the message ID (`basic_mga_gps_eph<Id>`,
 * `basic_mga_gps_alm<Id>`); see `mga_qzss.hpp`.
 *
 * @code
 * castle::protocols::ubx::messages::mga_gps_utc utc{};
 * utc.utc_dt_ls = 18;
 * uint8_t frame[32U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(utc, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_GPS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_GPS_HPP

#include "castle/core/compiler.hpp"
#include "castle/container/array.hpp"
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

/** @brief `type` byte values shared by UBX-MGA-GPS and UBX-MGA-QZSS layouts. */
static CASTLE_CONSTEXPR uint8_t MGA_GPS_TYPE_EPH = 0x01U;
static CASTLE_CONSTEXPR uint8_t MGA_GPS_TYPE_ALM = 0x02U;
static CASTLE_CONSTEXPR uint8_t MGA_GPS_TYPE_HEALTH = 0x04U;
static CASTLE_CONSTEXPR uint8_t MGA_GPS_TYPE_UTC = 0x05U;
static CASTLE_CONSTEXPR uint8_t MGA_GPS_TYPE_IONO = 0x06U;

/** @brief Message version byte for every MGA-GPS/QZSS layout in this revision. */
static CASTLE_CONSTEXPR uint8_t MGA_GPS_VERSION = 0x00U;

/** @brief GPS-style ephemeris assistance (68 bytes), shared by MGA-GPS-EPH and MGA-QZSS-EPH. */
template <uint8_t MsgId>
struct basic_mga_gps_eph
    : detail::typed_message<basic_mga_gps_eph<MsgId>, UBX_CLASS_MGA, MsgId, MGA_GPS_TYPE_EPH, 68U>
{
    uint8_t sv_id = 0U;
    uint8_t fit_interval = 0U;  ///< Fit interval flag.
    uint8_t ura_index = 0U;     ///< User range accuracy.
    uint8_t sv_health = 0U;
    int8_t tgd = 0;             ///< Group delay differential [2^-31 s].
    uint16_t iodc = 0U;
    uint16_t toc = 0U;          ///< Clock data reference time [2^4 s].
    int8_t af2 = 0;             ///< [2^-55 s/s^2].
    int16_t af1 = 0;            ///< [2^-43 s/s].
    int32_t af0 = 0;            ///< [2^-31 s].
    int16_t crs = 0;            ///< [2^-5 m].
    int16_t delta_n = 0;        ///< [2^-43 semi-circles/s].
    int32_t m0 = 0;             ///< [2^-31 semi-circles].
    int16_t cuc = 0;            ///< [2^-29 rad].
    int16_t cus = 0;            ///< [2^-29 rad].
    uint32_t e = 0U;            ///< Eccentricity [2^-33].
    uint32_t sqrt_a = 0U;       ///< [2^-19 m^0.5].
    uint16_t toe = 0U;          ///< Ephemeris reference time [2^4 s].
    int16_t cic = 0;            ///< [2^-29 rad].
    int32_t omega0 = 0;         ///< [2^-31 semi-circles].
    int16_t cis = 0;            ///< [2^-29 rad].
    int16_t crc = 0;            ///< [2^-5 m].
    int32_t i0 = 0;             ///< [2^-31 semi-circles].
    int32_t omega = 0;          ///< [2^-31 semi-circles].
    int32_t omega_dot = 0;      ///< [2^-43 semi-circles/s].
    int16_t idot = 0;           ///< [2^-43 semi-circles/s].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GPS_TYPE_EPH);
        stream.constant(MGA_GPS_VERSION);
        stream.field(self.sv_id);
        stream.reserved(1U);
        stream.field(self.fit_interval);
        stream.field(self.ura_index);
        stream.field(self.sv_health);
        stream.field(self.tgd);
        stream.field(self.iodc);
        stream.field(self.toc);
        stream.reserved(1U);
        stream.field(self.af2);
        stream.field(self.af1);
        stream.field(self.af0);
        stream.field(self.crs);
        stream.field(self.delta_n);
        stream.field(self.m0);
        stream.field(self.cuc);
        stream.field(self.cus);
        stream.field(self.e);
        stream.field(self.sqrt_a);
        stream.field(self.toe);
        stream.field(self.cic);
        stream.field(self.omega0);
        stream.field(self.cis);
        stream.field(self.crc);
        stream.field(self.i0);
        stream.field(self.omega);
        stream.field(self.omega_dot);
        stream.field(self.idot);
        stream.reserved(2U);
    }
};

/** @brief GPS-style almanac assistance (36 bytes), shared by MGA-GPS-ALM and MGA-QZSS-ALM. */
template <uint8_t MsgId>
struct basic_mga_gps_alm
    : detail::typed_message<basic_mga_gps_alm<MsgId>, UBX_CLASS_MGA, MsgId, MGA_GPS_TYPE_ALM, 36U>
{
    uint8_t sv_id = 0U;
    uint8_t sv_health = 0U;
    uint16_t e = 0U;            ///< Eccentricity [2^-21].
    uint8_t alm_wn_a = 0U;      ///< Almanac reference week (8-bit).
    uint8_t toa = 0U;           ///< Almanac reference time [2^12 s].
    int16_t delta_i = 0;        ///< [2^-19 semi-circles].
    int16_t omega_dot = 0;      ///< [2^-38 semi-circles/s].
    uint32_t sqrt_a = 0U;       ///< [2^-11 m^0.5].
    int32_t omega0 = 0;         ///< [2^-23 semi-circles].
    int32_t omega = 0;          ///< [2^-23 semi-circles].
    int32_t m0 = 0;             ///< [2^-23 semi-circles].
    int16_t af0 = 0;            ///< [2^-20 s].
    int16_t af1 = 0;            ///< [2^-38 s/s].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GPS_TYPE_ALM);
        stream.constant(MGA_GPS_VERSION);
        stream.field(self.sv_id);
        stream.field(self.sv_health);
        stream.field(self.e);
        stream.field(self.alm_wn_a);
        stream.field(self.toa);
        stream.field(self.delta_i);
        stream.field(self.omega_dot);
        stream.field(self.sqrt_a);
        stream.field(self.omega0);
        stream.field(self.omega);
        stream.field(self.m0);
        stream.field(self.af0);
        stream.field(self.af1);
        stream.reserved(4U);
    }
};

/** @brief UBX-MGA-GPS-EPH. */
using mga_gps_eph = basic_mga_gps_eph<UBX_ID_MGA_GPS>;

/** @brief UBX-MGA-GPS-ALM. */
using mga_gps_alm = basic_mga_gps_alm<UBX_ID_MGA_GPS>;

/** @brief UBX-MGA-GPS-HEALTH: health codes for all 32 GPS SVs (40 bytes). */
struct mga_gps_health
    : detail::typed_message<mga_gps_health, UBX_CLASS_MGA, UBX_ID_MGA_GPS, MGA_GPS_TYPE_HEALTH, 40U>
{
    static CASTLE_CONSTEXPR castle::size_type NUM_SVS = 32U;

    /** @brief `health_code[i]` is the 6-bit health of PRN `i + 1`. */
    castle::container::array<uint8_t, NUM_SVS> health_code{};

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GPS_TYPE_HEALTH);
        stream.constant(MGA_GPS_VERSION);
        stream.reserved(2U);
        stream.field(self.health_code);
        stream.reserved(4U);
    }
};

/** @brief UBX-MGA-GPS-UTC: GPS UTC model (20 bytes). */
struct mga_gps_utc
    : detail::typed_message<mga_gps_utc, UBX_CLASS_MGA, UBX_ID_MGA_GPS, MGA_GPS_TYPE_UTC, 20U>
{
    int32_t utc_a0 = 0;          ///< [2^-30 s].
    int32_t utc_a1 = 0;          ///< [2^-50 s/s].
    int8_t utc_dt_ls = 0;        ///< Leap seconds before the event [s].
    uint8_t utc_tot = 0U;        ///< Reference time of week [2^12 s].
    uint8_t utc_wn_t = 0U;       ///< Reference week (8-bit).
    uint8_t utc_wn_lsf = 0U;     ///< Leap second event week (8-bit).
    uint8_t utc_dn = 0U;         ///< Leap second event day of week.
    int8_t utc_dt_lsf = 0;       ///< Leap seconds after the event [s].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GPS_TYPE_UTC);
        stream.constant(MGA_GPS_VERSION);
        stream.reserved(2U);
        stream.field(self.utc_a0);
        stream.field(self.utc_a1);
        stream.field(self.utc_dt_ls);
        stream.field(self.utc_tot);
        stream.field(self.utc_wn_t);
        stream.field(self.utc_wn_lsf);
        stream.field(self.utc_dn);
        stream.field(self.utc_dt_lsf);
        stream.reserved(2U);
    }
};

/** @brief UBX-MGA-GPS-IONO: Klobuchar ionosphere parameters (16 bytes). */
struct mga_gps_iono
    : detail::typed_message<mga_gps_iono, UBX_CLASS_MGA, UBX_ID_MGA_GPS, MGA_GPS_TYPE_IONO, 16U>
{
    castle::container::array<int8_t, 4U> iono_alpha{}; ///< alpha0..3 [2^-30, -27, -24, -24].
    castle::container::array<int8_t, 4U> iono_beta{};  ///< beta0..3 [2^11, 14, 16, 16].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GPS_TYPE_IONO);
        stream.constant(MGA_GPS_VERSION);
        stream.reserved(2U);
        stream.field(self.iono_alpha);
        stream.field(self.iono_beta);
        stream.reserved(4U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_GPS_HPP
