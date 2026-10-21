// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file msm.hpp
 * @brief RTCM 3 Multiple Signal Messages MSM4, MSM5 and MSM7 for every GNSS.
 *
 * MSM is the current RTCM 3 observation format (messages 1074/1075/1077 GPS,
 * 1084/1085/1087 GLONASS, 1094/1095/1097 Galileo, 1124/1125/1127 BeiDou). One
 * message carries the observations of all satellites and signals of one epoch:
 *
 *  - a header with three masks: satellites (64 bit), signals (32 bit) and the
 *    satellite x signal cell mask (`satellites * signals` bits, at most 64);
 *  - one `msm_satellite` per masked satellite (rough range, and for MSM5/7
 *    extended info and rough phase-range rate);
 *  - one `msm_signal` per set cell (fine pseudorange, fine phase-range, lock
 *    time, half-cycle flag, CNR and, for MSM5/7, fine phase-range rate).
 *
 * | Level | Pseudorange | Phase-range | CNR         | Rate fields |
 * |-------|-------------|-------------|-------------|-------------|
 * | MSM4  | 2^-24 ms    | 2^-29 ms    | 1 dB-Hz     | no          |
 * | MSM5  | 2^-24 ms    | 2^-29 ms    | 1 dB-Hz     | yes         |
 * | MSM7  | 2^-29 ms    | 2^-31 ms    | 1/16 dB-Hz  | yes         |
 *
 * All values are stored as raw wire integers. The `set_*` helpers quantise
 * physical measurements into them, honouring the invalid-value conventions
 * (a value that does not fit becomes the field's "invalid" marker).
 *
 * @code
 * using msm = castle::protocols::rtcm::messages::gps_msm7;
 *
 * msm epoch{};
 * epoch.station_id = 1U;
 * epoch.epoch_time = 345600000U;
 * epoch.satellite_mask = msm::satellite_bit(5U) | msm::satellite_bit(12U);
 * epoch.signal_mask = msm::signal_bit(2U);          // GPS L1 C/A
 * epoch.cell_mask = 0x3U;                           // 2 satellites x 1 signal, both present
 *
 * castle::protocols::rtcm::messages::msm_satellite sat{};
 * msm::set_rough_range(sat, 70.123456, -123.0);
 * static_cast<void>(epoch.satellites.push_back(sat));
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_MSM_HPP
#define CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_MSM_HPP

#include "castle/bit/bit_count.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"
#include "castle/error/status.hpp"
#include "castle/math/round.hpp"

#include "castle_ext/protocols/rtcm/bit_writer.hpp"
#include "castle_ext/protocols/rtcm/rtcm.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace rtcm
{
namespace messages
{

/** @brief GNSS served by an MSM message; the value is the MSM1 message number minus one. */
enum class msm_system : uint16_t
{
    gps = 1070U,
    glonass = 1080U,
    galileo = 1090U,
    sbas = 1100U,
    qzss = 1110U,
    beidou = 1120U
};

/** @brief Per-satellite data of an MSM4/5/7 message. */
struct msm_satellite
{
    uint8_t rough_range_ms = 0U;           ///< DF397 (8 bit): integer milliseconds of the rough range; 255 = invalid.
    uint8_t extended_info = 0U;            ///< (MSM5/7, 4 bit): GLONASS frequency channel number + 7, otherwise reserved.
    uint16_t rough_range_mod = 0U;         ///< DF398 (10 bit): rough range modulo 1 ms, 2^-10 ms.
    int16_t rough_phase_range_rate = 0;    ///< DF399 (MSM5/7, 14 bit signed): rough phase-range rate, 1 m/s; -8192 = invalid.
};

/** @brief Per-cell (satellite x signal) data of an MSM4/5/7 message. */
struct msm_signal
{
    int32_t fine_pseudorange = 0;          ///< DF400 (MSM4/5, 15 bit) / DF405 (MSM7, 20 bit): signed fine pseudorange.
    int32_t fine_phase_range = 0;          ///< DF401 (MSM4/5, 22 bit) / DF406 (MSM7, 24 bit): signed fine phase-range.
    uint16_t lock_time = 0U;               ///< DF402 (MSM4/5, 4 bit) / DF407 (MSM7, 10 bit): lock time indicator.
    bool half_cycle = false;               ///< DF420: half-cycle ambiguity present.
    uint16_t cnr = 0U;                     ///< DF403 (MSM4/5, 6 bit, 1 dB-Hz) / DF408 (MSM7, 10 bit, 1/16 dB-Hz).
    int16_t fine_phase_range_rate = 0;     ///< DF404 (MSM5/7, 15 bit signed): fine phase-range rate, 0.0001 m/s; -16384 = invalid.
};

/**
 * @brief One MSM4, MSM5 or MSM7 message.
 *
 * @tparam System Constellation, selecting the message number.
 * @tparam Level  MSM level: 4, 5 or 7.
 * @tparam MaxSatellites Capacity of `satellites` (at most 64).
 * @tparam MaxCells Capacity of `signals` (at most 64).
 */
template <msm_system System,
          uint8_t Level,
          castle::size_type MaxSatellites = 64U,
          castle::size_type MaxCells = 64U>
struct msm_message
{
    static_assert((Level == 4U) || (Level == 5U) || (Level == 7U), "only MSM4, MSM5 and MSM7 are supported");
    static_assert((MaxSatellites > 0U) && (MaxSatellites <= 64U), "an MSM message has at most 64 satellites");
    static_assert((MaxCells > 0U) && (MaxCells <= 64U), "an MSM message has at most 64 cells");

    /** @brief Message number (DF002) of this system and level. */
    static CASTLE_CONSTEXPR uint16_t MESSAGE_TYPE = static_cast<uint16_t>(static_cast<uint16_t>(System) + Level);

    /** @brief Largest satellite x signal product allowed by the cell mask. */
    static CASTLE_CONSTEXPR castle::size_type MAX_CELLS = 64U;

    /** @name Level-dependent field layout @{ */
    static CASTLE_CONSTEXPR bool HAS_RATE = (Level >= 5U);                    ///< Carries extended info and phase-range rates.
    static CASTLE_CONSTEXPR castle::size_type PSEUDORANGE_BITS = (Level == 7U) ? 20U : 15U;
    static CASTLE_CONSTEXPR castle::size_type PHASE_RANGE_BITS = (Level == 7U) ? 24U : 22U;
    static CASTLE_CONSTEXPR castle::size_type LOCK_TIME_BITS = (Level == 7U) ? 10U : 4U;
    static CASTLE_CONSTEXPR castle::size_type CNR_BITS = (Level == 7U) ? 10U : 6U;
    /** @} */

    /** @name Quantisation steps
     *  Fine pseudorange / phase-range steps are in light-milliseconds. @{ */
    static CASTLE_CONSTEXPR double PSEUDORANGE_STEPS_PER_MS = (Level == 7U) ? 536870912.0 : 16777216.0;   ///< 2^29 or 2^24.
    static CASTLE_CONSTEXPR double PHASE_RANGE_STEPS_PER_MS = (Level == 7U) ? 2147483648.0 : 536870912.0; ///< 2^31 or 2^29.
    static CASTLE_CONSTEXPR double CNR_STEPS_PER_DBHZ = (Level == 7U) ? 16.0 : 1.0;
    static CASTLE_CONSTEXPR double ROUGH_RANGE_STEPS_PER_MS = 1024.0;
    static CASTLE_CONSTEXPR double FINE_RATE_STEPS_PER_M_S = 10000.0;
    /** @} */

    /** @name Invalid-value markers @{ */
    static CASTLE_CONSTEXPR uint8_t INVALID_ROUGH_RANGE = 255U;
    static CASTLE_CONSTEXPR int16_t INVALID_ROUGH_RATE = -8192;
    static CASTLE_CONSTEXPR int16_t INVALID_FINE_RATE = -16384;
    static CASTLE_CONSTEXPR int32_t INVALID_FINE_PSEUDORANGE = (Level == 7U) ? -524288 : -16384;
    static CASTLE_CONSTEXPR int32_t INVALID_FINE_PHASE_RANGE = (Level == 7U) ? -8388608 : -2097152;
    /** @} */

    uint16_t station_id = 0U;           ///< DF003: reference station ID (12 bit).
    uint32_t epoch_time = 0U;           ///< 30 bit epoch: TOW in ms (GPS, Galileo, BeiDou) or day of week (3 bit) + time of day in ms (27 bit) for GLONASS.
    bool multiple_message = false;      ///< DF393: more MSM messages follow for this epoch.
    uint8_t iods = 0U;                  ///< DF409 (3 bit): issue of data station.
    uint8_t clock_steering = 0U;        ///< DF411 (2 bit): 0 unknown, 1 not applied, 2 applied.
    uint8_t external_clock = 0U;        ///< DF412 (2 bit): external clock indicator.
    bool divergence_free = false;       ///< DF417: divergence-free smoothing indicator.
    uint8_t smoothing_interval = 0U;    ///< DF418 (3 bit): smoothing interval.
    uint64_t satellite_mask = 0U;       ///< DF394: bit for satellite ID n is `satellite_bit(n)`.
    uint32_t signal_mask = 0U;          ///< DF395: bit for signal ID n is `signal_bit(n)`.
    uint64_t cell_mask = 0U;            ///< DF396: `popcount(satellite_mask) * popcount(signal_mask)` bits, satellite-major.
    castle::container::vector<msm_satellite, MaxSatellites> satellites{}; ///< One entry per set bit of `satellite_mask`, in ID order.
    castle::container::vector<msm_signal, MaxCells> signals{};            ///< One entry per set bit of `cell_mask`, in cell order.

    /** @brief Reports whether `type` is this message's number. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR bool accepts(uint16_t type) CASTLE_NOEXCEPT
    {
        return type == MESSAGE_TYPE;
    }

    /** @brief Satellite-mask bit of satellite ID `id` (1..64). */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR uint64_t satellite_bit(castle::size_type id) CASTLE_NOEXCEPT
    {
        return (id >= 1U && id <= 64U) ? (static_cast<uint64_t>(1) << (64U - id)) : 0U;
    }

    /** @brief Signal-mask bit of signal ID `id` (1..32). */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR uint32_t signal_bit(castle::size_type id) CASTLE_NOEXCEPT
    {
        return (id >= 1U && id <= 32U) ? (static_cast<uint32_t>(1) << (32U - id)) : 0U;
    }

    /**
     * @brief Cell-mask bit of the cell (`satellite_index`, `signal_index`).
     *
     * @param satellite_index Position of the satellite among the set bits of `satellite_mask` (0-based).
     * @param signal_index Position of the signal among the set bits of `signal_mask` (0-based).
     * @param satellite_count Number of satellites in the message.
     * @param signal_count Number of signals in the message.
     * @return The bit to OR into `cell_mask`, or 0 when an index is out of range.
     */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR uint64_t cell_bit(castle::size_type satellite_index,
                                                               castle::size_type signal_index,
                                                               castle::size_type satellite_count,
                                                               castle::size_type signal_count) CASTLE_NOEXCEPT
    {
        return ((satellite_index < satellite_count) && (signal_index < signal_count)
                && ((satellite_count * signal_count) <= MAX_CELLS))
            ? (static_cast<uint64_t>(1)
               << ((satellite_count * signal_count) - 1U - (satellite_index * signal_count + signal_index)))
            : 0U;
    }

    /** @brief Stores a GLONASS epoch: day of week (0 = Sunday .. 6, 7 = unknown) and time of day in ms. */
    void set_glonass_epoch(uint8_t day_of_week, uint32_t time_of_day_ms) CASTLE_NOEXCEPT
    {
        epoch_time = (static_cast<uint32_t>(day_of_week & 0x07U) << 27U) | (time_of_day_ms & 0x07FFFFFFU);
    }

    /**
     * @brief Quantises a rough range into `sat`.
     * @param sat Satellite entry to fill.
     * @param range_ms Range in light-milliseconds (metres / 299792.458); 0 <= range_ms < 255.
     * @note A range outside 0..255 ms sets the invalid marker.
     */
    static void set_rough_range(msm_satellite& sat, double range_ms) CASTLE_NOEXCEPT
    {
        if (!(range_ms >= 0.0) || (range_ms >= 255.0))
        {
            sat.rough_range_ms = INVALID_ROUGH_RANGE;
            sat.rough_range_mod = 0U;
            return;
        }
        CASTLE_CONST uint32_t steps = castle::math::round_half_away<uint32_t>(range_ms * ROUGH_RANGE_STEPS_PER_MS);
        sat.rough_range_ms = static_cast<uint8_t>(steps >> 10U);
        sat.rough_range_mod = (sat.rough_range_ms == INVALID_ROUGH_RANGE) ? 0U : static_cast<uint16_t>(steps & 0x3FFU);
    }

    /**
     * @brief Quantises a rough range and rough phase-range rate into `sat` (MSM5/7).
     * @param rate_m_s Rough phase-range rate in m/s; outside -8191..8191 sets the invalid marker.
     */
    static void set_rough_range(msm_satellite& sat, double range_ms, double rate_m_s) CASTLE_NOEXCEPT
    {
        static_assert(HAS_RATE, "MSM4 carries no phase-range rate");
        set_rough_range(sat, range_ms);
        sat.rough_phase_range_rate = (rate_m_s > -8191.5 && rate_m_s < 8191.5)
            ? static_cast<int16_t>(castle::math::round_half_away<int32_t>(rate_m_s))
            : INVALID_ROUGH_RATE;
    }

    /**
     * @brief Sets the fine pseudorange of `cell` from a full range.
     * @param sat The satellite the cell belongs to (already holding its rough range).
     * @param range_ms Pseudorange in light-milliseconds.
     * @note The residual to the quantised rough range must fit the field, otherwise the invalid marker is stored.
     */
    static void set_pseudorange(CASTLE_CONST msm_satellite& sat, msm_signal& cell, double range_ms) CASTLE_NOEXCEPT
    {
        cell.fine_pseudorange = quantise_residual(range_ms - rough_range_ms_of(sat),
                                                  PSEUDORANGE_STEPS_PER_MS,
                                                  PSEUDORANGE_BITS);
    }

    /** @brief Sets the fine phase-range of `cell` from a full phase-range in light-milliseconds. */
    static void set_phase_range(CASTLE_CONST msm_satellite& sat, msm_signal& cell, double range_ms) CASTLE_NOEXCEPT
    {
        cell.fine_phase_range = quantise_residual(range_ms - rough_range_ms_of(sat),
                                                  PHASE_RANGE_STEPS_PER_MS,
                                                  PHASE_RANGE_BITS);
    }

    /** @brief Sets the fine phase-range rate of `cell` from a full rate in m/s (MSM5/7). */
    static void set_phase_range_rate(CASTLE_CONST msm_satellite& sat, msm_signal& cell, double rate_m_s) CASTLE_NOEXCEPT
    {
        static_assert(HAS_RATE, "MSM4 carries no phase-range rate");
        cell.fine_phase_range_rate = static_cast<int16_t>(quantise_residual(
            rate_m_s - static_cast<double>(sat.rough_phase_range_rate), FINE_RATE_STEPS_PER_M_S, 15U));
    }

    /** @brief Sets the CNR of `cell` from dB-Hz, clamped to the field range (NaN gives 0). */
    static void set_cnr(msm_signal& cell, double dbhz) CASTLE_NOEXCEPT
    {
        CASTLE_CONST double steps = dbhz * CNR_STEPS_PER_DBHZ;
        CASTLE_CONST double maximum = static_cast<double>((static_cast<uint32_t>(1) << CNR_BITS) - 1U);
        cell.cnr = static_cast<uint16_t>(castle::math::round_half_away<uint32_t>(!(steps > 0.0) ? 0.0 : ((steps > maximum) ? maximum : steps)));
    }

    /** @brief Writes the message payload; see `encode_message()`. */
    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST castle::size_type satellite_count = castle::bit::popcount(satellite_mask);
        CASTLE_CONST castle::size_type signal_count = castle::bit::popcount(signal_mask);
        CASTLE_CONST castle::size_type cell_bits = satellite_count * signal_count;
        if ((cell_bits > MAX_CELLS)
            || (satellites.size() != satellite_count)
            || (signals.size() != castle::bit::popcount(cell_mask)))
        {
            writer.fail(castle::status::invalid_argument);
            return;
        }

        writer.field(MESSAGE_TYPE, RTCM_MESSAGE_NUMBER_BITS);
        writer.field(station_id, RTCM_STATION_ID_BITS);
        writer.field(epoch_time, 30U);
        writer.field(multiple_message);
        writer.field(iods, 3U);
        writer.reserved(7U);
        writer.field(clock_steering, 2U);
        writer.field(external_clock, 2U);
        writer.field(divergence_free);
        writer.field(smoothing_interval, 3U);
        writer.field(satellite_mask, 64U);
        writer.field(signal_mask, 32U);
        writer.field(cell_mask, cell_bits);

        // Satellite data and signal data are field-major: each field is written for all entries before the next.
        for (CASTLE_CONST msm_satellite& sat : satellites)
        {
            writer.field(sat.rough_range_ms, 8U);
        }
        if (HAS_RATE)
        {
            for (CASTLE_CONST msm_satellite& sat : satellites)
            {
                writer.field(sat.extended_info, 4U);
            }
        }
        for (CASTLE_CONST msm_satellite& sat : satellites)
        {
            writer.field(sat.rough_range_mod, 10U);
        }
        if (HAS_RATE)
        {
            for (CASTLE_CONST msm_satellite& sat : satellites)
            {
                writer.field(sat.rough_phase_range_rate, 14U);
            }
        }

        for (CASTLE_CONST msm_signal& cell : signals)
        {
            writer.field(cell.fine_pseudorange, PSEUDORANGE_BITS);
        }
        for (CASTLE_CONST msm_signal& cell : signals)
        {
            writer.field(cell.fine_phase_range, PHASE_RANGE_BITS);
        }
        for (CASTLE_CONST msm_signal& cell : signals)
        {
            writer.field(cell.lock_time, LOCK_TIME_BITS);
        }
        for (CASTLE_CONST msm_signal& cell : signals)
        {
            writer.field(cell.half_cycle);
        }
        for (CASTLE_CONST msm_signal& cell : signals)
        {
            writer.field(cell.cnr, CNR_BITS);
        }
        if (HAS_RATE)
        {
            for (CASTLE_CONST msm_signal& cell : signals)
            {
                writer.field(cell.fine_phase_range_rate, 15U);
            }
        }
    }

private:
    /** @brief Quantised rough range of `sat` in milliseconds. */
    static double rough_range_ms_of(CASTLE_CONST msm_satellite& sat) CASTLE_NOEXCEPT
    {
        return static_cast<double>(sat.rough_range_ms) + (static_cast<double>(sat.rough_range_mod) / ROUGH_RANGE_STEPS_PER_MS);
    }

    /**
     * @brief Converts a residual to a signed `bits`-wide step count.
     * @return The rounded step count, or the field's invalid marker (-2^(bits-1)) when it does not fit.
     */
    static int32_t quantise_residual(double residual, double steps_per_unit, castle::size_type bits) CASTLE_NOEXCEPT
    {
        CASTLE_CONST int32_t invalid = -static_cast<int32_t>(static_cast<uint32_t>(1) << (bits - 1U));
        CASTLE_CONST double scaled = residual * steps_per_unit;
        CASTLE_CONST double limit = static_cast<double>(-invalid);
        if (!(scaled > -limit) || !(scaled < limit))
        {
            return invalid;
        }
        CASTLE_CONST int32_t steps = castle::math::round_half_away<int32_t>(scaled);
        return ((steps <= invalid) || (steps >= -invalid)) ? invalid : steps;
    }
};

/** @name MSM aliases of the messages u-blox receivers accept @{ */
using gps_msm4 = msm_message<msm_system::gps, 4U>;
using gps_msm5 = msm_message<msm_system::gps, 5U>;
using gps_msm7 = msm_message<msm_system::gps, 7U>;
using glonass_msm4 = msm_message<msm_system::glonass, 4U>;
using glonass_msm5 = msm_message<msm_system::glonass, 5U>;
using glonass_msm7 = msm_message<msm_system::glonass, 7U>;
using galileo_msm4 = msm_message<msm_system::galileo, 4U>;
using galileo_msm5 = msm_message<msm_system::galileo, 5U>;
using galileo_msm7 = msm_message<msm_system::galileo, 7U>;
using beidou_msm4 = msm_message<msm_system::beidou, 4U>;
using beidou_msm5 = msm_message<msm_system::beidou, 5U>;
using beidou_msm7 = msm_message<msm_system::beidou, 7U>;
/** @} */

} // namespace messages
} // namespace rtcm
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_MSM_HPP
