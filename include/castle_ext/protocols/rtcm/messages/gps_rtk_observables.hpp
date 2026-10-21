// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file gps_rtk_observables.hpp
 * @brief RTCM 3 messages 1001-1004: legacy GPS L1 / L1+L2 RTK observables.
 *
 * | Type | Content                         | Bits per satellite |
 * |------|---------------------------------|--------------------|
 * | 1001 | L1-only                         | 58                 |
 * | 1002 | L1-only, extended (ambiguity, CNR) | 74              |
 * | 1003 | L1 and L2                       | 101                |
 * | 1004 | L1 and L2, extended             | 125                |
 *
 * Each message number is its own type, as for the MSM models: pick one of the
 * aliases `gps_rtk_l1_obs` (1001), `gps_rtk_l1_obs_ext` (1002),
 * `gps_rtk_l1_l2_obs` (1003) or `gps_rtk_l1_l2_obs_ext` (1004). Satellite
 * fields the message does not carry are ignored.
 * Values are stored as the raw integers sent on the wire; the `*_RESOLUTION`
 * constants give their physical scale.
 */
#ifndef CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_GPS_RTK_OBSERVABLES_HPP
#define CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_GPS_RTK_OBSERVABLES_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"

#include "castle_ext/protocols/rtcm/bit_writer.hpp"
#include "castle_ext/protocols/rtcm/rtcm.hpp"
#include "castle_ext/protocols/rtcm/messages/detail/rtk_observables_family.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace rtcm
{
namespace messages
{

/** @brief One GPS satellite block of messages 1001-1004. */
struct gps_rtk_satellite
{
    uint8_t satellite_id = 0U;                     ///< DF009: GPS PRN (1..32).
    uint8_t l1_code = 0U;                          ///< DF010: 0 = C/A, 1 = P(Y).
    uint32_t l1_pseudorange = 0U;                  ///< DF011 (24 bit): L1 pseudorange modulo ambiguity, 0.02 m.
    int32_t l1_phase_minus_pseudorange = 0;        ///< DF012 (20 bit signed): L1 phaserange - pseudorange, 0.0005 m.
    uint8_t l1_lock_time = 0U;                     ///< DF013 (7 bit): L1 lock time indicator.
    uint8_t l1_ambiguity = 0U;                     ///< DF014 (1002/1004): integer L1 pseudorange ambiguity, 299792.458 m.
    uint8_t l1_cnr = 0U;                           ///< DF015 (1002/1004): L1 CNR, 0.25 dB-Hz.
    uint8_t l2_code = 0U;                          ///< DF016 (1003/1004, 2 bit): 0 = C/A, 1 = P direct, 2 = P cross-correlated, 3 = Z-tracking.
    int16_t l2_pseudorange_difference = 0;         ///< DF017 (1003/1004, 14 bit signed): L2 - L1 pseudorange, 0.02 m.
    int32_t l2_phase_minus_l1_pseudorange = 0;     ///< DF018 (1003/1004, 20 bit signed): L2 phaserange - L1 pseudorange, 0.0005 m.
    uint8_t l2_lock_time = 0U;                     ///< DF019 (1003/1004, 7 bit): L2 lock time indicator.
    uint8_t l2_cnr = 0U;                           ///< DF020 (1004): L2 CNR, 0.25 dB-Hz.
};

/**
 * @brief One of the RTCM messages 1001, 1002, 1003 and 1004.
 *
 * `MESSAGE_TYPE`, `IS_EXTENDED`, `HAS_L2` and `accepts()` come from
 * `detail::rtk_observables_family`.
 *
 * @tparam Type Message number, `RTCM_TYPE_GPS_L1_OBS` .. `RTCM_TYPE_GPS_L1L2_OBS_EXT`.
 * @tparam MaxSatellites Satellite capacity, at most 31 (the count is a 5-bit field).
 */
template <uint16_t Type, castle::size_type MaxSatellites = 31U>
struct gps_rtk_observables : detail::rtk_observables_family<RTCM_TYPE_GPS_L1_OBS, Type>
{
    using family = detail::rtk_observables_family<RTCM_TYPE_GPS_L1_OBS, Type>;
    using family::HAS_L2;
    using family::IS_EXTENDED;
    using family::MESSAGE_TYPE;

    static_assert((MaxSatellites > 0U) && (MaxSatellites <= 31U),
                  "the satellite count of messages 1001-1004 is a 5-bit field");

    /** @name Resolutions of the raw fields @{ */
    static CASTLE_CONSTEXPR double PSEUDORANGE_RESOLUTION_M = 0.02;
    static CASTLE_CONSTEXPR double PHASE_RESOLUTION_M = 0.0005;
    static CASTLE_CONSTEXPR double AMBIGUITY_M = 299792.458;
    static CASTLE_CONSTEXPR double CNR_RESOLUTION_DBHZ = 0.25;
    /** @} */

    uint16_t station_id = 0U;                           ///< DF003: reference station ID (12 bit).
    uint32_t tow_ms = 0U;                               ///< DF004: GPS epoch time (TOW), 30 bit, ms.
    bool synchronous = false;                           ///< DF005: more messages follow for this epoch.
    bool divergence_free = false;                       ///< DF007: divergence-free smoothing indicator.
    uint8_t smoothing_interval = 0U;                    ///< DF008 (3 bit): smoothing interval.
    castle::container::vector<gps_rtk_satellite, MaxSatellites> satellites{}; ///< DF006 is `satellites.size()`.

    /** @brief Writes the message payload; see `encode_message()`. */
    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.field(MESSAGE_TYPE, RTCM_MESSAGE_NUMBER_BITS);
        writer.field(station_id, RTCM_STATION_ID_BITS);
        writer.field(tow_ms, 30U);
        writer.field(synchronous);
        writer.field(satellites.size(), 5U);
        writer.field(divergence_free);
        writer.field(smoothing_interval, 3U);

        for (CASTLE_CONST gps_rtk_satellite& sat : satellites)
        {
            writer.field(sat.satellite_id, 6U);
            writer.field(sat.l1_code, 1U);
            writer.field(sat.l1_pseudorange, 24U);
            writer.field(sat.l1_phase_minus_pseudorange, 20U);
            writer.field(sat.l1_lock_time, 7U);
            if (IS_EXTENDED)
            {
                writer.field(sat.l1_ambiguity, 8U);
                writer.field(sat.l1_cnr, 8U);
            }
            if (HAS_L2)
            {
                writer.field(sat.l2_code, 2U);
                writer.field(sat.l2_pseudorange_difference, 14U);
                writer.field(sat.l2_phase_minus_l1_pseudorange, 20U);
                writer.field(sat.l2_lock_time, 7U);
                if (IS_EXTENDED)
                {
                    writer.field(sat.l2_cnr, 8U);
                }
            }
        }
    }
};

/** @name Message 1001-1004 aliases @{ */
using gps_rtk_l1_obs = gps_rtk_observables<RTCM_TYPE_GPS_L1_OBS>;
using gps_rtk_l1_obs_ext = gps_rtk_observables<RTCM_TYPE_GPS_L1_OBS_EXT>;
using gps_rtk_l1_l2_obs = gps_rtk_observables<RTCM_TYPE_GPS_L1L2_OBS>;
using gps_rtk_l1_l2_obs_ext = gps_rtk_observables<RTCM_TYPE_GPS_L1L2_OBS_EXT>;
/** @} */

} // namespace messages
} // namespace rtcm
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_GPS_RTK_OBSERVABLES_HPP
