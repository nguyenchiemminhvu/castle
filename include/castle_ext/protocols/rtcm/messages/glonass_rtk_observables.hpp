// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file glonass_rtk_observables.hpp
 * @brief RTCM 3 messages 1009-1012: legacy GLONASS L1 / L1+L2 RTK observables.
 *
 * | Type | Content                            | Bits per satellite |
 * |------|------------------------------------|--------------------|
 * | 1009 | L1-only                            | 64                 |
 * | 1010 | L1-only, extended (ambiguity, CNR) | 79                 |
 * | 1011 | L1 and L2                          | 107                |
 * | 1012 | L1 and L2, extended                | 130                |
 *
 * Each message number is its own type, as for the MSM models: pick one of the
 * aliases `glonass_rtk_l1_obs` (1009), `glonass_rtk_l1_obs_ext` (1010),
 * `glonass_rtk_l1_l2_obs` (1011) or `glonass_rtk_l1_l2_obs_ext` (1012).
 * Satellite fields the message does not carry are ignored.
 */
#ifndef CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_GLONASS_RTK_OBSERVABLES_HPP
#define CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_GLONASS_RTK_OBSERVABLES_HPP

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

/** @brief One GLONASS satellite block of messages 1009-1012. */
struct glonass_rtk_satellite
{
    uint8_t satellite_id = 0U;                     ///< DF038: GLONASS slot number (1..24; 40..63 reserved for non-GLONASS).
    uint8_t l1_code = 0U;                          ///< DF039: 0 = C/A, 1 = P.
    uint8_t frequency_channel = 0U;                ///< DF040 (5 bit): frequency channel number + 7 (0..20).
    uint32_t l1_pseudorange = 0U;                  ///< DF041 (25 bit): L1 pseudorange modulo ambiguity, 0.02 m.
    int32_t l1_phase_minus_pseudorange = 0;        ///< DF042 (20 bit signed): L1 phaserange - pseudorange, 0.0005 m.
    uint8_t l1_lock_time = 0U;                     ///< DF043 (7 bit): L1 lock time indicator.
    uint8_t l1_ambiguity = 0U;                     ///< DF044 (1010/1012, 7 bit): integer L1 pseudorange ambiguity, 599584.916 m.
    uint8_t l1_cnr = 0U;                           ///< DF045 (1010/1012): L1 CNR, 0.25 dB-Hz.
    uint8_t l2_code = 0U;                          ///< DF046 (1011/1012, 2 bit): 0 = C/A, 1 = P.
    int16_t l2_pseudorange_difference = 0;         ///< DF047 (1011/1012, 14 bit signed): L2 - L1 pseudorange, 0.02 m.
    int32_t l2_phase_minus_l1_pseudorange = 0;     ///< DF048 (1011/1012, 20 bit signed): L2 phaserange - L1 pseudorange, 0.0005 m.
    uint8_t l2_lock_time = 0U;                     ///< DF049 (1011/1012, 7 bit): L2 lock time indicator.
    uint8_t l2_cnr = 0U;                           ///< DF050 (1012): L2 CNR, 0.25 dB-Hz.
};

/**
 * @brief One of the RTCM messages 1009, 1010, 1011 and 1012.
 *
 * `MESSAGE_TYPE`, `IS_EXTENDED`, `HAS_L2` and `accepts()` come from
 * `detail::rtk_observables_family`.
 *
 * @tparam Type Message number, `RTCM_TYPE_GLO_L1_OBS` .. `RTCM_TYPE_GLO_L1L2_OBS_EXT`.
 * @tparam MaxSatellites Satellite capacity, at most 31 (the count is a 5-bit field).
 */
template <uint16_t Type, castle::size_type MaxSatellites = 31U>
struct glonass_rtk_observables : detail::rtk_observables_family<RTCM_TYPE_GLO_L1_OBS, Type>
{
    using family = detail::rtk_observables_family<RTCM_TYPE_GLO_L1_OBS, Type>;
    using family::HAS_L2;
    using family::IS_EXTENDED;
    using family::MESSAGE_TYPE;

    static_assert((MaxSatellites > 0U) && (MaxSatellites <= 31U),
                  "the satellite count of messages 1009-1012 is a 5-bit field");

    /** @name Resolutions of the raw fields @{ */
    static CASTLE_CONSTEXPR double PSEUDORANGE_RESOLUTION_M = 0.02;
    static CASTLE_CONSTEXPR double PHASE_RESOLUTION_M = 0.0005;
    static CASTLE_CONSTEXPR double AMBIGUITY_M = 599584.916;
    static CASTLE_CONSTEXPR double CNR_RESOLUTION_DBHZ = 0.25;
    /** @} */

    uint16_t station_id = 0U;                           ///< DF003: reference station ID (12 bit).
    uint32_t time_of_day_ms = 0U;                       ///< DF034: GLONASS epoch time tk, 27 bit, ms.
    bool synchronous = false;                           ///< DF005: more messages follow for this epoch.
    bool divergence_free = false;                       ///< DF036: divergence-free smoothing indicator.
    uint8_t smoothing_interval = 0U;                    ///< DF037 (3 bit): smoothing interval.
    castle::container::vector<glonass_rtk_satellite, MaxSatellites> satellites{}; ///< DF035 is `satellites.size()`.

    /** @brief Writes the message payload; see `encode_message()`. */
    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.field(MESSAGE_TYPE, RTCM_MESSAGE_NUMBER_BITS);
        writer.field(station_id, RTCM_STATION_ID_BITS);
        writer.field(time_of_day_ms, 27U);
        writer.field(synchronous);
        writer.field(satellites.size(), 5U);
        writer.field(divergence_free);
        writer.field(smoothing_interval, 3U);

        for (CASTLE_CONST glonass_rtk_satellite& sat : satellites)
        {
            writer.field(sat.satellite_id, 6U);
            writer.field(sat.l1_code, 1U);
            writer.field(sat.frequency_channel, 5U);
            writer.field(sat.l1_pseudorange, 25U);
            writer.field(sat.l1_phase_minus_pseudorange, 20U);
            writer.field(sat.l1_lock_time, 7U);
            if (IS_EXTENDED)
            {
                writer.field(sat.l1_ambiguity, 7U);
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

/** @name Message 1009-1012 aliases @{ */
using glonass_rtk_l1_obs = glonass_rtk_observables<RTCM_TYPE_GLO_L1_OBS>;
using glonass_rtk_l1_obs_ext = glonass_rtk_observables<RTCM_TYPE_GLO_L1_OBS_EXT>;
using glonass_rtk_l1_l2_obs = glonass_rtk_observables<RTCM_TYPE_GLO_L1L2_OBS>;
using glonass_rtk_l1_l2_obs_ext = glonass_rtk_observables<RTCM_TYPE_GLO_L1L2_OBS_EXT>;
/** @} */

} // namespace messages
} // namespace rtcm
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_GLONASS_RTK_OBSERVABLES_HPP
