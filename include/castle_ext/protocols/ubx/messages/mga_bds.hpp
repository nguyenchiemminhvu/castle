// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mga_bds.hpp
 * @brief UBX-MGA-BDS (0x13 0x03): BeiDou assistance data (ephemeris, almanac, health, UTC, ionosphere).
 *
 * Input messages sent by the host. One class/ID carries five layouts, told
 * apart by payload byte 0 (`type`), so each layout is its own model built on
 * `detail::typed_message`. Every model has `encode()` for transmission and a
 * `decode()` for loop-back / logging.
 *
 * @code
 * castle::protocols::ubx::messages::mga_bds_utc utc{};
 * utc.dt_ls = 4;
 * uint8_t frame[32U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(utc, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_BDS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_BDS_HPP

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

/** @brief `type` byte values of the UBX-MGA-BDS layouts. */
static CASTLE_CONSTEXPR uint8_t MGA_BDS_TYPE_EPH = 0x01U;
static CASTLE_CONSTEXPR uint8_t MGA_BDS_TYPE_ALM = 0x02U;
static CASTLE_CONSTEXPR uint8_t MGA_BDS_TYPE_HEALTH = 0x04U;
static CASTLE_CONSTEXPR uint8_t MGA_BDS_TYPE_UTC = 0x05U;
static CASTLE_CONSTEXPR uint8_t MGA_BDS_TYPE_IONO = 0x06U;

/** @brief Message version byte for every MGA-BDS layout in this revision. */
static CASTLE_CONSTEXPR uint8_t MGA_BDS_VERSION = 0x00U;

/** @brief UBX-MGA-BDS-EPH: BeiDou ephemeris assistance (88 bytes). */
struct mga_bds_eph
    : detail::typed_message<mga_bds_eph, UBX_CLASS_MGA, UBX_ID_MGA_BDS, MGA_BDS_TYPE_EPH, 88U>
{
    uint8_t sv_id = 0U;
    uint8_t sat_h1 = 0U;        ///< Autonomous satellite health flag.
    uint8_t iodc = 0U;          ///< Issue of data, clock.
    int16_t a2 = 0;             ///< Time polynomial coefficient 2 [2^-66 s/s^2].
    int32_t a1 = 0;             ///< Time polynomial coefficient 1 [2^-50 s/s].
    int32_t a0 = 0;             ///< Time polynomial coefficient 0 [2^-33 s].
    uint32_t toc = 0U;          ///< Clock data reference time [2^3 s].
    int16_t tgd1 = 0;           ///< Equipment group delay differential [0.1 ns].
    uint8_t urai = 0U;          ///< User range accuracy index.
    uint8_t iode = 0U;          ///< Issue of data, ephemeris.
    uint32_t toe = 0U;          ///< Ephemeris reference time [2^3 s].
    uint32_t sqrt_a = 0U;       ///< [2^-19 m^0.5].
    uint32_t e = 0U;            ///< Eccentricity [2^-33].
    int32_t omega = 0;          ///< Argument of perigee [2^-31 semi-circles].
    int16_t delta_n = 0;        ///< [2^-43 semi-circles/s].
    int16_t idot = 0;           ///< [2^-43 semi-circles/s].
    int32_t m0 = 0;             ///< [2^-31 semi-circles].
    int32_t omega0 = 0;         ///< [2^-31 semi-circles].
    int32_t omega_dot = 0;      ///< [2^-43 semi-circles/s].
    int32_t i0 = 0;             ///< [2^-31 semi-circles].
    int32_t cuc = 0;            ///< [2^-31 semi-circles].
    int32_t cus = 0;            ///< [2^-31 semi-circles].
    int32_t crc = 0;            ///< [2^-6 m].
    int32_t crs = 0;            ///< [2^-6 m].
    int32_t cic = 0;            ///< [2^-31 semi-circles].
    int32_t cis = 0;            ///< [2^-31 semi-circles].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_BDS_TYPE_EPH);
        stream.constant(MGA_BDS_VERSION);
        stream.field(self.sv_id);
        stream.reserved(1U);
        stream.field(self.sat_h1);
        stream.field(self.iodc);
        stream.field(self.a2);
        stream.field(self.a1);
        stream.field(self.a0);
        stream.field(self.toc);
        stream.field(self.tgd1);
        stream.field(self.urai);
        stream.field(self.iode);
        stream.field(self.toe);
        stream.field(self.sqrt_a);
        stream.field(self.e);
        stream.field(self.omega);
        stream.field(self.delta_n);
        stream.field(self.idot);
        stream.field(self.m0);
        stream.field(self.omega0);
        stream.field(self.omega_dot);
        stream.field(self.i0);
        stream.field(self.cuc);
        stream.field(self.cus);
        stream.field(self.crc);
        stream.field(self.crs);
        stream.field(self.cic);
        stream.field(self.cis);
        stream.reserved(4U);
    }
};

/** @brief UBX-MGA-BDS-ALM: BeiDou almanac assistance (40 bytes). */
struct mga_bds_alm
    : detail::typed_message<mga_bds_alm, UBX_CLASS_MGA, UBX_ID_MGA_BDS, MGA_BDS_TYPE_ALM, 40U>
{
    uint8_t sv_id = 0U;
    uint8_t wna = 0U;           ///< Almanac week number [week].
    uint8_t toa = 0U;           ///< Almanac reference time [2^12 s].
    int16_t delta_i = 0;        ///< [2^-19 semi-circles].
    uint32_t sqrt_a = 0U;       ///< [2^-11 m^0.5].
    uint32_t e = 0U;            ///< Eccentricity [2^-21].
    int32_t omega = 0;          ///< Argument of perigee [2^-23 semi-circles].
    int32_t m0 = 0;             ///< [2^-23 semi-circles].
    int32_t omega0 = 0;         ///< [2^-23 semi-circles].
    int32_t omega_dot = 0;      ///< [2^-38 semi-circles/s].
    int16_t a0 = 0;             ///< Satellite clock bias [2^-20 s].
    int16_t a1 = 0;             ///< Satellite clock rate [2^-38 s/s].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_BDS_TYPE_ALM);
        stream.constant(MGA_BDS_VERSION);
        stream.field(self.sv_id);
        stream.reserved(1U);
        stream.field(self.wna);
        stream.field(self.toa);
        stream.field(self.delta_i);
        stream.field(self.sqrt_a);
        stream.field(self.e);
        stream.field(self.omega);
        stream.field(self.m0);
        stream.field(self.omega0);
        stream.field(self.omega_dot);
        stream.field(self.a0);
        stream.field(self.a1);
        stream.reserved(4U);
    }
};

/** @brief UBX-MGA-BDS-HEALTH: health codes for BeiDou SVs 1-30 (68 bytes). */
struct mga_bds_health
    : detail::typed_message<mga_bds_health, UBX_CLASS_MGA, UBX_ID_MGA_BDS, MGA_BDS_TYPE_HEALTH, 68U>
{
    static CASTLE_CONSTEXPR castle::size_type NUM_SVS = 30U;

    /** @brief The 9 LSBs of each `health_code` entry carry the D1 health code. */
    static CASTLE_CONSTEXPR uint16_t HEALTH_CODE_MASK = 0x01FFU;

    /** @brief `health_code[i]` is the health word of SV `i + 1`. */
    castle::container::array<uint16_t, NUM_SVS> health_code{};

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_BDS_TYPE_HEALTH);
        stream.constant(MGA_BDS_VERSION);
        stream.reserved(2U);
        stream.field(self.health_code);
        stream.reserved(4U);
    }
};

/** @brief UBX-MGA-BDS-UTC: BDT-to-UTC model (20 bytes). */
struct mga_bds_utc
    : detail::typed_message<mga_bds_utc, UBX_CLASS_MGA, UBX_ID_MGA_BDS, MGA_BDS_TYPE_UTC, 20U>
{
    int32_t a0_utc = 0;          ///< BDT clock bias relative to UTC [2^-30 s].
    int32_t a1_utc = 0;          ///< BDT clock rate relative to UTC [2^-50 s/s].
    int8_t dt_ls = 0;            ///< Leap seconds before the new leap second [s].
    uint8_t wn_rec = 0U;         ///< Week of reception of this parameter set (8-bit) [week].
    uint8_t wn_lsf = 0U;         ///< Week of the new leap second [week].
    uint8_t dn = 0U;             ///< Day number of the new leap second [day].
    int8_t dt_lsf = 0;           ///< Leap seconds after the new leap second [s].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_BDS_TYPE_UTC);
        stream.constant(MGA_BDS_VERSION);
        stream.reserved(2U);
        stream.field(self.a0_utc);
        stream.field(self.a1_utc);
        stream.field(self.dt_ls);
        stream.reserved(1U);
        stream.field(self.wn_rec);
        stream.field(self.wn_lsf);
        stream.field(self.dn);
        stream.field(self.dt_lsf);
        stream.reserved(2U);
    }
};

/** @brief UBX-MGA-BDS-IONO: Klobuchar ionosphere parameters (16 bytes). */
struct mga_bds_iono
    : detail::typed_message<mga_bds_iono, UBX_CLASS_MGA, UBX_ID_MGA_BDS, MGA_BDS_TYPE_IONO, 16U>
{
    castle::container::array<int8_t, 4U> alpha{}; ///< alpha0..3 [2^-30, -27, -24, -24].
    castle::container::array<int8_t, 4U> beta{};  ///< beta0..3 [2^11, 14, 16, 16].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_BDS_TYPE_IONO);
        stream.constant(MGA_BDS_VERSION);
        stream.reserved(2U);
        stream.field(self.alpha);
        stream.field(self.beta);
        stream.reserved(4U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_BDS_HPP
