// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mga_glo.hpp
 * @brief UBX-MGA-GLO (0x13 0x06): GLONASS assistance data (ephemeris, almanac, auxiliary time offsets).
 *
 * Input messages sent by the host. One class/ID carries three layouts, told
 * apart by payload byte 0 (`type`), so each layout is its own model built on
 * `detail::typed_message`. Every model has `encode()` for transmission and a
 * `decode()` for loop-back / logging.
 *
 * @code
 * castle::protocols::ubx::messages::mga_glo_timeoffset offsets{};
 * offsets.n = 1200U;
 * uint8_t frame[32U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(offsets, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_GLO_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_GLO_HPP

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

/** @brief `type` byte values of the UBX-MGA-GLO layouts. */
static CASTLE_CONSTEXPR uint8_t MGA_GLO_TYPE_EPH = 0x01U;
static CASTLE_CONSTEXPR uint8_t MGA_GLO_TYPE_ALM = 0x02U;
static CASTLE_CONSTEXPR uint8_t MGA_GLO_TYPE_TIMEOFFSET = 0x03U;

/** @brief Message version byte for every MGA-GLO layout in this revision. */
static CASTLE_CONSTEXPR uint8_t MGA_GLO_VERSION = 0x00U;

/** @brief `h` value meaning "frequency channel unknown" (ephemeris only). */
static CASTLE_CONSTEXPR int8_t MGA_GLO_FREQ_CHANNEL_UNKNOWN = -128;

/** @brief UBX-MGA-GLO-EPH: GLONASS ephemeris assistance (48 bytes). */
struct mga_glo_eph
    : detail::typed_message<mga_glo_eph, UBX_CLASS_MGA, UBX_ID_MGA_GLO, MGA_GLO_TYPE_EPH, 48U>
{
    uint8_t sv_id = 0U;
    uint8_t ft = 0U;            ///< User range accuracy.
    uint8_t b = 0U;             ///< Health flag from string 2.
    uint8_t m = 0U;             ///< Satellite type (1 = GLONASS-M).
    int8_t h = 0;               ///< Frequency channel number (-7..6, -128 = unknown).
    int32_t x = 0;              ///< PZ-90.02 position X [2^-11 km].
    int32_t y = 0;              ///< PZ-90.02 position Y [2^-11 km].
    int32_t z = 0;              ///< PZ-90.02 position Z [2^-11 km].
    int32_t dx = 0;             ///< PZ-90.02 velocity X [2^-20 km/s].
    int32_t dy = 0;             ///< PZ-90.02 velocity Y [2^-20 km/s].
    int32_t dz = 0;             ///< PZ-90.02 velocity Z [2^-20 km/s].
    int8_t ddx = 0;             ///< PZ-90.02 acceleration X [2^-30 km/s^2].
    int8_t ddy = 0;             ///< PZ-90.02 acceleration Y [2^-30 km/s^2].
    int8_t ddz = 0;             ///< PZ-90.02 acceleration Z [2^-30 km/s^2].
    uint8_t tb = 0U;            ///< Time-interval index within the UTC(SU) day [15 min].
    int16_t gamma = 0;          ///< Relative carrier frequency deviation [2^-40].
    uint8_t e = 0U;             ///< Ephemeris data age [day].
    int8_t delta_tau = 0;       ///< L2-L1 time difference [2^-30 s].
    int32_t tau = 0;            ///< SV clock bias [2^-30 s].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GLO_TYPE_EPH);
        stream.constant(MGA_GLO_VERSION);
        stream.field(self.sv_id);
        stream.reserved(1U);
        stream.field(self.ft);
        stream.field(self.b);
        stream.field(self.m);
        stream.field(self.h);
        stream.field(self.x);
        stream.field(self.y);
        stream.field(self.z);
        stream.field(self.dx);
        stream.field(self.dy);
        stream.field(self.dz);
        stream.field(self.ddx);
        stream.field(self.ddy);
        stream.field(self.ddz);
        stream.field(self.tb);
        stream.field(self.gamma);
        stream.field(self.e);
        stream.field(self.delta_tau);
        stream.field(self.tau);
        stream.reserved(4U);
    }
};

/** @brief UBX-MGA-GLO-ALM: GLONASS almanac assistance (36 bytes). */
struct mga_glo_alm
    : detail::typed_message<mga_glo_alm, UBX_CLASS_MGA, UBX_ID_MGA_GLO, MGA_GLO_TYPE_ALM, 36U>
{
    uint8_t sv_id = 0U;
    uint16_t n = 0U;            ///< Almanac calendar day within the four-year period [day].
    uint8_t m = 0U;             ///< Satellite type (1 = GLONASS-M).
    uint8_t c = 0U;             ///< Operability flag at almanac upload (1 = operable).
    int16_t tau = 0;            ///< Coarse time correction to GLONASS time [2^-18 s].
    uint16_t epsilon = 0U;      ///< Eccentricity [2^-20].
    int32_t lambda = 0;         ///< Longitude of first ascending node [2^-20 semi-circles].
    int32_t delta_i = 0;        ///< Inclination correction [2^-20 semi-circles].
    uint32_t t_lambda = 0U;     ///< Time of first ascending node passage [2^-5 s].
    int32_t delta_t = 0;        ///< Draconian period correction [2^-9 s/orbital-period].
    int8_t delta_dt = 0;        ///< Draconian period rate [2^-14 s/orbital-period^2].
    int8_t h = 0;               ///< Frequency channel number (-7..6).
    int16_t omega = 0;          ///< Argument of perigee (scale not given by the spec).

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GLO_TYPE_ALM);
        stream.constant(MGA_GLO_VERSION);
        stream.field(self.sv_id);
        stream.reserved(1U);
        stream.field(self.n);
        stream.field(self.m);
        stream.field(self.c);
        stream.field(self.tau);
        stream.field(self.epsilon);
        stream.field(self.lambda);
        stream.field(self.delta_i);
        stream.field(self.t_lambda);
        stream.field(self.delta_t);
        stream.field(self.delta_dt);
        stream.field(self.h);
        stream.field(self.omega);
        stream.reserved(4U);
    }
};

/** @brief UBX-MGA-GLO-TIMEOFFSET: GLONASS auxiliary time offsets (20 bytes). */
struct mga_glo_timeoffset
    : detail::typed_message<mga_glo_timeoffset, UBX_CLASS_MGA, UBX_ID_MGA_GLO, MGA_GLO_TYPE_TIMEOFFSET, 20U>
{
    uint16_t n = 0U;            ///< Almanac calendar day within the four-year period [day].
    int32_t tau_c = 0;          ///< Time scale correction to UTC(SU) [2^-27 s].
    int32_t tau_gps = 0;        ///< GPS time relative to GLONASS time [2^-31 s].
    int16_t b1 = 0;             ///< Delta UT1 coefficient [2^-10 s].
    int16_t b2 = 0;             ///< Delta UT1 rate [2^-16 s/msd].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GLO_TYPE_TIMEOFFSET);
        stream.constant(MGA_GLO_VERSION);
        stream.field(self.n);
        stream.field(self.tau_c);
        stream.field(self.tau_gps);
        stream.field(self.b1);
        stream.field(self.b2);
        stream.reserved(4U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_GLO_HPP
