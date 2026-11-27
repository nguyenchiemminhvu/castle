// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mga_gal.hpp
 * @brief UBX-MGA-GAL (0x13 0x02): Galileo assistance data (ephemeris, almanac, GPS time offset, UTC).
 *
 * Input messages sent by the host. One class/ID carries four layouts, told
 * apart by payload byte 0 (`type`), so each layout is its own model built on
 * `detail::typed_message`. Every model has `encode()` for transmission and a
 * `decode()` for loop-back / logging.
 *
 * @code
 * castle::protocols::ubx::messages::mga_gal_timeoffset ggto{};
 * ggto.a0_g = -12;
 * uint8_t frame[24U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(ggto, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_GAL_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_GAL_HPP

#include "castle/core/compiler.hpp"
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

/** @brief `type` byte values of the UBX-MGA-GAL layouts. */
static CASTLE_CONSTEXPR uint8_t MGA_GAL_TYPE_EPH = 0x01U;
static CASTLE_CONSTEXPR uint8_t MGA_GAL_TYPE_ALM = 0x02U;
static CASTLE_CONSTEXPR uint8_t MGA_GAL_TYPE_TIMEOFFSET = 0x03U;
static CASTLE_CONSTEXPR uint8_t MGA_GAL_TYPE_UTC = 0x05U;

/** @brief Message version byte for every MGA-GAL layout in this revision. */
static CASTLE_CONSTEXPR uint8_t MGA_GAL_VERSION = 0x00U;

/** @brief UBX-MGA-GAL-EPH: Galileo ephemeris assistance (76 bytes). */
struct mga_gal_eph
    : detail::typed_message<mga_gal_eph, UBX_CLASS_MGA, UBX_ID_MGA_GAL, MGA_GAL_TYPE_EPH, 76U>
{
    uint8_t sv_id = 0U;
    uint16_t iod_nav = 0U;          ///< Ephemeris and clock correction issue of data.
    int16_t delta_n = 0;            ///< [2^-43 semi-circles/s].
    int32_t m0 = 0;                 ///< [2^-31 semi-circles].
    uint32_t e = 0U;                ///< Eccentricity [2^-33].
    uint32_t sqrt_a = 0U;           ///< [2^-19 m^0.5].
    int32_t omega0 = 0;             ///< [2^-31 semi-circles].
    int32_t i0 = 0;                 ///< [2^-31 semi-circles].
    int32_t omega = 0;              ///< [2^-31 semi-circles].
    int32_t omega_dot = 0;          ///< [2^-43 semi-circles/s].
    int16_t i_dot = 0;              ///< [2^-43 semi-circles/s].
    int16_t cuc = 0;                ///< [2^-29 rad].
    int16_t cus = 0;                ///< [2^-29 rad].
    int16_t crc = 0;                ///< [2^-5 m] (the spec table prints "radians").
    int16_t crs = 0;                ///< [2^-5 m] (the spec table prints "radians").
    int16_t cic = 0;                ///< [2^-29 rad].
    int16_t cis = 0;                ///< [2^-29 rad].
    uint16_t toe = 0U;              ///< Ephemeris reference time [60 s].
    int32_t af0 = 0;                ///< [2^-34 s].
    int32_t af1 = 0;                ///< [2^-46 s/s].
    int8_t af2 = 0;                 ///< [2^-59 s/s^2].
    uint8_t sisa_index_e1_e5b = 0U; ///< Signal-in-space accuracy index (E1-E5b).
    uint16_t toc = 0U;              ///< Clock correction reference time of week [60 s].
    int16_t bgd_e1_e5b = 0;         ///< E1-E5b broadcast group delay.
    uint8_t health_e1b = 0U;        ///< E1-B signal health status.
    uint8_t data_validity_e1b = 0U; ///< E1-B data validity status.
    uint8_t health_e5b = 0U;        ///< E5b signal health status.
    uint8_t data_validity_e5b = 0U; ///< E5b data validity status.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GAL_TYPE_EPH);
        stream.constant(MGA_GAL_VERSION);
        stream.field(self.sv_id);
        stream.reserved(1U);
        stream.field(self.iod_nav);
        stream.field(self.delta_n);
        stream.field(self.m0);
        stream.field(self.e);
        stream.field(self.sqrt_a);
        stream.field(self.omega0);
        stream.field(self.i0);
        stream.field(self.omega);
        stream.field(self.omega_dot);
        stream.field(self.i_dot);
        stream.field(self.cuc);
        stream.field(self.cus);
        stream.field(self.crc);
        stream.field(self.crs);
        stream.field(self.cic);
        stream.field(self.cis);
        stream.field(self.toe);
        stream.field(self.af0);
        stream.field(self.af1);
        stream.field(self.af2);
        stream.field(self.sisa_index_e1_e5b);
        stream.field(self.toc);
        stream.field(self.bgd_e1_e5b);
        stream.reserved(2U);
        stream.field(self.health_e1b);
        stream.field(self.data_validity_e1b);
        stream.field(self.health_e5b);
        stream.field(self.data_validity_e5b);
        stream.reserved(4U);
    }
};

/** @brief UBX-MGA-GAL-ALM: Galileo almanac assistance (32 bytes). */
struct mga_gal_alm
    : detail::typed_message<mga_gal_alm, UBX_CLASS_MGA, UBX_ID_MGA_GAL, MGA_GAL_TYPE_ALM, 32U>
{
    uint8_t sv_id = 0U;
    uint8_t ioda = 0U;              ///< Almanac issue of data.
    uint8_t alm_wn_a = 0U;          ///< Almanac reference week number [week].
    uint16_t toa = 0U;              ///< Almanac reference time [600 s].
    int16_t delta_sqrt_a = 0;       ///< Delta to sqrt of nominal semi-major axis (29 600 km) [2^-9 m^0.5].
    uint16_t e = 0U;                ///< Eccentricity [2^-16].
    int16_t delta_i = 0;            ///< Inclination relative to 56 deg [2^-14 semi-circles].
    int16_t omega0 = 0;             ///< [2^-15 semi-circles].
    int16_t omega_dot = 0;          ///< [2^-33 semi-circles/s].
    int16_t omega = 0;              ///< [2^-15 semi-circles].
    int16_t m0 = 0;                 ///< [2^-15 semi-circles].
    int16_t af0 = 0;                ///< [2^-19 s].
    int16_t af1 = 0;                ///< [2^-38 s/s].
    uint8_t health_e1b = 0U;        ///< E1-B signal health status.
    uint8_t health_e5b = 0U;        ///< E5b signal health status.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GAL_TYPE_ALM);
        stream.constant(MGA_GAL_VERSION);
        stream.field(self.sv_id);
        stream.reserved(1U);
        stream.field(self.ioda);
        stream.field(self.alm_wn_a);
        stream.field(self.toa);
        stream.field(self.delta_sqrt_a);
        stream.field(self.e);
        stream.field(self.delta_i);
        stream.field(self.omega0);
        stream.field(self.omega_dot);
        stream.field(self.omega);
        stream.field(self.m0);
        stream.field(self.af0);
        stream.field(self.af1);
        stream.field(self.health_e1b);
        stream.field(self.health_e5b);
        stream.reserved(4U);
    }
};

/** @brief UBX-MGA-GAL-TIMEOFFSET: Galileo-to-GPS time offset (GGTO, 12 bytes). */
struct mga_gal_timeoffset
    : detail::typed_message<mga_gal_timeoffset, UBX_CLASS_MGA, UBX_ID_MGA_GAL, MGA_GAL_TYPE_TIMEOFFSET, 12U>
{
    int16_t a0_g = 0;               ///< Constant term of the offset polynomial [2^-35 s].
    int16_t a1_g = 0;               ///< Rate of change of the offset [2^-51 s/s].
    uint8_t t0_g = 0U;              ///< GGTO reference time [3600 s].
    uint8_t wn0_g = 0U;             ///< GGTO reference week number [week].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GAL_TYPE_TIMEOFFSET);
        stream.constant(MGA_GAL_VERSION);
        stream.reserved(2U);
        stream.field(self.a0_g);
        stream.field(self.a1_g);
        stream.field(self.t0_g);
        stream.field(self.wn0_g);
        stream.reserved(2U);
    }
};

/** @brief UBX-MGA-GAL-UTC: Galileo UTC model (20 bytes). */
struct mga_gal_utc
    : detail::typed_message<mga_gal_utc, UBX_CLASS_MGA, UBX_ID_MGA_GAL, MGA_GAL_TYPE_UTC, 20U>
{
    int32_t a0 = 0;                 ///< First UTC polynomial parameter [2^-30 s].
    int32_t a1 = 0;                 ///< Second UTC polynomial parameter [2^-50 s/s].
    int8_t dt_ls = 0;               ///< Current leap seconds [s].
    uint8_t tot = 0U;               ///< UTC reference time of week (Galileo time) [3600 s].
    uint8_t wnt = 0U;               ///< UTC reference week number (8-bit) [week].
    uint8_t wn_lsf = 0U;            ///< Week of the future leap second (8-bit) [week].
    uint8_t dn = 0U;                ///< Day of the future leap second [day].
    int8_t dt_lsf = 0;              ///< Future leap seconds (spec `dTLSF`) [s].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GAL_TYPE_UTC);
        stream.constant(MGA_GAL_VERSION);
        stream.reserved(2U);
        stream.field(self.a0);
        stream.field(self.a1);
        stream.field(self.dt_ls);
        stream.field(self.tot);
        stream.field(self.wnt);
        stream.field(self.wn_lsf);
        stream.field(self.dn);
        stream.field(self.dt_lsf);
        stream.reserved(2U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_GAL_HPP
