/**
 * @file rtcm_input_support.hpp
 * @brief Shared plumbing of the u-blox RTCM input samples (samples only, not a library header).
 *
 * UBX-19056845 chapter 4 lists the RTCM 3.3 messages a ZED-F9R accepts as
 * input under u-blox names such as `RTCM-3X-TYPE1002` with a "Class/ID" of
 * `0xf5 0x02`. That Class/ID is u-blox's catalogue identifier only: the
 * payload table of every entry starts with the RTCM preamble `0xd3` and ends
 * with the 3-byte CRC-24Q, i.e. it *is* a plain RTCM 3 frame. The host writes
 * that frame to a receiver port unchanged; there is no `0xb5 0x62` UBX header,
 * no UBX length and no Fletcher checksum around it.
 *
 * This header provides what every per-type sample shares:
 *  - `receiver_port`: the bytes a host writes to one receiver port, encoded in
 *    place (zero copy) by `rtcm::encode_message()` or `ubx::encode_message()`;
 *  - `check_rtcm_frame()`: proves the last frame is a bare RTCM 3 frame;
 *  - the configuration keys involved in RTCM input (UBX-19056845 section 5.9);
 *  - one realistic epoch of measurements per constellation, and the two
 *    conversions from physical units to the raw RTCM data fields that the
 *    legacy (1001-1012) and MSM (1074-1127) models store.
 */
#ifndef CASTLE_SAMPLE_UBX_RTCM_INPUT_SUPPORT_HPP
#define CASTLE_SAMPLE_UBX_RTCM_INPUT_SUPPORT_HPP

#include "sample_support.hpp"

#include "castle/container/array_view.hpp"
#include "castle/math/round.hpp"

#include "castle_ext/checksums/crc24q.hpp"
#include "castle_ext/protocols/rtcm/bit_writer.hpp"
#include "castle_ext/protocols/rtcm/messages/msm.hpp"
#include "castle_ext/protocols/rtcm/rtcm.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"

#include <stdint.h>

namespace rtcm_sample
{

// -----------------------------------------------------------------------------
// UBX configuration keys involved in RTCM input (UBX-19056845 section 5.9)
// -----------------------------------------------------------------------------

/** @name `CFG-<port>INPROT-RTCM3X` (type L): accept RTCM 3 input on a port. Default true on every port. @{ */
CASTLE_CONSTEXPR uint32_t CFG_UART1INPROT_RTCM3X = 0x10730004U;
CASTLE_CONSTEXPR uint32_t CFG_UART2INPROT_RTCM3X = 0x10750004U;
CASTLE_CONSTEXPR uint32_t CFG_USBINPROT_RTCM3X = 0x10770004U;
CASTLE_CONSTEXPR uint32_t CFG_I2CINPROT_RTCM3X = 0x10710004U;
CASTLE_CONSTEXPR uint32_t CFG_SPIINPROT_RTCM3X = 0x10790004U;
/** @} */

/** @brief `CFG-RTCM-DF003_IN` (U2): reference station ID used by the input filter. Default 0. */
CASTLE_CONSTEXPR uint32_t CFG_RTCM_DF003_IN = 0x30090008U;
/** @brief `CFG-RTCM-DF003_IN_FILTER` (E1): 0 DISABLED (default), 1 RELAXED, 2 STRICT. */
CASTLE_CONSTEXPR uint32_t CFG_RTCM_DF003_IN_FILTER = 0x20090009U;
CASTLE_CONSTEXPR uint8_t DF003_FILTER_RELAXED = 1U;
/** @brief `CFG-MSGOUT-UBX_RXM_RTCM_UART1` (U1): UBX-RXM-RTCM output rate on UART1. Default 0 (off). */
CASTLE_CONSTEXPR uint32_t CFG_MSGOUT_UBX_RXM_RTCM_UART1 = 0x20910269U;

// -----------------------------------------------------------------------------
// Receiver port
// -----------------------------------------------------------------------------

/**
 * @brief Bytes the host writes to one receiver port (UART, USB, I2C or SPI).
 *
 * Frames are encoded directly into the transmit buffer; a real driver would
 * hand `bytes[0 .. size)` to the UART DMA. UBX and RTCM frames share the port:
 * the receiver tells them apart by their first byte (`0xb5` or `0xd3`).
 */
struct receiver_port
{
    static CASTLE_CONSTEXPR castle::size_type CAPACITY = 8192U;

    uint8_t bytes[CAPACITY] = {};
    castle::size_type size = 0U;       ///< Bytes queued so far.
    castle::size_type last_start = 0U; ///< Offset of the most recent frame.

    /** @brief Appends one RTCM 3 frame built from an RTCM message model. */
    template <typename Message>
    castle::status send_rtcm(const Message& message) noexcept
    {
        castle::size_type written = 0U;
        const castle::status result =
            castle::protocols::rtcm::encode_message(message, &bytes[size], CAPACITY - size, written);
        commit(written);
        return result;
    }

    /** @brief Appends one UBX frame built from a UBX message model (for example `cfg_valset`). */
    template <typename Message>
    castle::status send_ubx(const Message& message) noexcept
    {
        castle::size_type written = 0U;
        const castle::status result =
            castle::protocols::ubx::encode_message(message, &bytes[size], CAPACITY - size, written);
        commit(written);
        return result;
    }

    /** @brief The most recent frame. */
    castle::container::array_view<const uint8_t> last_frame() const noexcept
    {
        return castle::container::array_view<const uint8_t>(&bytes[last_start], size - last_start);
    }

private:
    void commit(castle::size_type written) noexcept
    {
        last_start = size;
        size += written; // written is 0 on failure, so nothing is queued.
    }
};

/** @brief The host's connection to the receiver: UART1 accepts UBX, NMEA and RTCM 3 input by default. */
inline receiver_port uart1{};

/** @brief Reads `width` (<= 32) payload bits of `frame` starting at payload bit `position`, MSB first. */
inline uint32_t payload_bits(castle::container::array_view<const uint8_t> frame,
                             castle::size_type position,
                             castle::size_type width) noexcept
{
    uint32_t value = 0U;
    for (castle::size_type index = 0U; index < width; ++index)
    {
        const castle::size_type bit = position + index;
        const uint8_t byte = frame[castle::protocols::rtcm::RTCM_FRAME_INDEX_PAYLOAD + (bit / 8U)];
        value = (value << 1U) | ((byte >> (7U - (bit % 8U))) & 1U);
    }
    return value;
}

/**
 * @brief Checks that the last frame on `port` is a bare RTCM 3 frame of message `type` from station `station_id`.
 *
 * This is exactly the u-blox payload table of `RTCM-3X-TYPE<type>`:
 * `preamble 0xd3 | res1:6 numData:10 | data[numData] | crc[3]`.
 */
inline void check_rtcm_frame(const receiver_port& port, uint16_t type, uint16_t station_id) noexcept
{
    using namespace castle::protocols::rtcm;

    const castle::container::array_view<const uint8_t> frame = port.last_frame();
    CASTLE_SAMPLE_CHECK(frame.size() > RTCM_FRAME_OVERHEAD);
    CASTLE_SAMPLE_CHECK(frame[0U] == RTCM_PREAMBLE);                              // preamble, not UBX 0xb5 0x62
    CASTLE_SAMPLE_CHECK((frame[1U] & 0xFCU) == 0U);                               // res1: 6 reserved zero bits
    const castle::size_type num_data = (static_cast<castle::size_type>(frame[1U] & 0x03U) << 8U) | frame[2U];
    CASTLE_SAMPLE_CHECK(frame.size() == frame_size(num_data));                    // "Message Size: 6 + numData"
    CASTLE_SAMPLE_CHECK(payload_bits(frame, 0U, RTCM_MESSAGE_NUMBER_BITS) == type); // DF002
    CASTLE_SAMPLE_CHECK(payload_bits(frame, RTCM_MESSAGE_NUMBER_BITS, RTCM_STATION_ID_BITS) == station_id); // DF003
    CASTLE_SAMPLE_CHECK(castle::checksums::crc24q::calculate(frame.data(), frame.size()) == 0U); // CRC-24Q residue
}

// -----------------------------------------------------------------------------
// Reference station and one epoch of measurements per constellation
// -----------------------------------------------------------------------------

CASTLE_CONSTEXPR uint16_t STATION_ID = 1234U; ///< DF003 of every sample frame.

/** @brief Base station antenna reference point, ECEF (ITRF) metres. */
CASTLE_CONSTEXPR double STATION_ECEF_M[3U] = {4027894.0060, 307045.6000, 4919474.9100};

/** @brief Speed of light, m/ms: dividing metres by it gives the light-milliseconds of the MSM range fields. */
CASTLE_CONSTEXPR double LIGHT_M_PER_MS = 299792.458;

/**
 * @brief One satellite observed by the base station on two signals.
 *
 * Signal 0 / 1 are L1 C/A / L2C (GPS), L1 C/A / L2 C/A (GLONASS), E1 / E5b
 * (Galileo) and B1I / B2I (BeiDou), the signals a ZED-F9R tracks.
 */
struct observation
{
    uint8_t id;               ///< GPS PRN, GLONASS slot, Galileo PRN or BeiDou PRN.
    int8_t frequency_channel; ///< GLONASS frequency channel number k (-7..+6); 0 elsewhere.
    double pseudorange_m[2U];
    double phase_range_m[2U];  ///< Carrier phase in metres (cycles x wavelength).
    double range_rate_m_s[2U]; ///< Phase-range rate (-Doppler x wavelength).
    double cnr_dbhz[2U];
};

CASTLE_CONSTEXPR uint32_t GPS_TOW_MS = 345601000U; ///< GPS time of week, ms.
inline CASTLE_CONSTEXPR observation GPS_EPOCH[3U] = {
    {5U, 0, {21345678.123, 21345680.456}, {21345677.902, 21345680.214}, {-412.350, -412.338}, {47.25, 41.50}},
    {12U, 0, {23098765.432, 23098768.010}, {23098764.118, 23098766.721}, {120.500, 120.511}, {44.00, 38.75}},
    {25U, 0, {20876543.210, 20876545.600}, {20876542.833, 20876545.128}, {-2250.700, -2250.688}, {51.50, 45.25}},
};

CASTLE_CONSTEXPR uint8_t GLONASS_DAY_OF_WEEK = 3U;           ///< DF416: 0 = Sunday.
CASTLE_CONSTEXPR uint32_t GLONASS_TIME_OF_DAY_MS = 43218000U; ///< DF034: GLONASS (Moscow) time of day, ms.
inline CASTLE_CONSTEXPR observation GLONASS_EPOCH[3U] = {
    {3U, 5, {19876543.210, 19876545.870}, {19876542.650, 19876545.010}, {310.250, 310.270}, {45.00, 39.50}},
    {10U, -7, {22123456.780, 22123459.020}, {22123455.930, 22123458.440}, {-1520.400, -1520.370}, {42.75, 37.00}},
    {17U, 4, {24012345.670, 24012348.110}, {24012344.920, 24012347.360}, {870.600, 870.620}, {38.50, 33.25}},
};

CASTLE_CONSTEXPR uint32_t GALILEO_TOW_MS = 345601000U; ///< DF248: Galileo system time of week, ms.
inline CASTLE_CONSTEXPR observation GALILEO_EPOCH[3U] = {
    {4U, 0, {23567890.120, 23567892.750}, {23567889.400, 23567891.980}, {-650.300, -650.280}, {48.00, 45.50}},
    {11U, 0, {25789012.340, 25789014.810}, {25789011.720, 25789014.050}, {215.750, 215.770}, {44.25, 42.00}},
    {19U, 0, {27012345.680, 27012348.020}, {27012344.910, 27012347.330}, {1789.100, 1789.120}, {40.50, 38.25}},
};

CASTLE_CONSTEXPR uint32_t BEIDOU_TOW_MS = 345587000U; ///< DF427: BeiDou time of week, ms (GPS time - 14 s).
inline CASTLE_CONSTEXPR observation BEIDOU_EPOCH[3U] = {
    {3U, 0, {37912345.670, 37912348.210}, {37912344.880, 37912347.300}, {1.250, 1.270}, {43.00, 41.25}}, // GEO
    {19U, 0, {21876543.210, 21876545.730}, {21876542.470, 21876544.950}, {-980.600, -980.580}, {47.50, 44.00}},
    {27U, 0, {24456789.010, 24456791.660}, {24456788.300, 24456790.820}, {455.200, 455.220}, {41.75, 39.50}},
};

/** @name RTCM 10403.3 MSM signal IDs (DF395 bit numbers) of the two tracked signals. @{ */
CASTLE_CONSTEXPR castle::size_type GPS_SIGNALS[2U] = {2U, 16U};      ///< L1 C/A (1C), L2C L (2L).
CASTLE_CONSTEXPR castle::size_type GLONASS_SIGNALS[2U] = {2U, 8U};   ///< G1 C/A (1C), G2 C/A (2C).
CASTLE_CONSTEXPR castle::size_type GALILEO_SIGNALS[2U] = {2U, 14U};  ///< E1 C (1C), E5b I (7I).
CASTLE_CONSTEXPR castle::size_type BEIDOU_SIGNALS[2U] = {2U, 14U};   ///< B1I (2I), B2I (7I).
/** @} */

// -----------------------------------------------------------------------------
// Physical units -> raw data fields
// -----------------------------------------------------------------------------

/** @brief Rounds `value / step` to the nearest integer step. */
template <typename Int>
Int to_steps(double value, double step) noexcept
{
    return castle::math::round_half_away<Int>(value / step);
}

/** @brief Lock time indicator for "tracked continuously for a long time" (DF013, DF019, DF043, DF049). */
CASTLE_CONSTEXPR uint8_t LEGACY_LOCK_TIME_LONG = 127U;

/** @brief The full L1 pseudorange a rover rebuilds from DF014 / DF044 and DF011 / DF041. */
template <typename Observables, typename Satellite>
double l1_pseudorange_m(const Satellite& sat) noexcept
{
    return (static_cast<double>(sat.l1_ambiguity) * Observables::AMBIGUITY_M)
         + (static_cast<double>(sat.l1_pseudorange) * Observables::PSEUDORANGE_RESOLUTION_M);
}

/**
 * @brief Fills the L1 fields of a legacy observables satellite (1001-1004, 1009-1012).
 *
 * The pseudorange is split into the integer ambiguity (DF014 / DF044, in units
 * of `Observables::AMBIGUITY_M`) and the remainder (DF011 / DF041, 0.02 m);
 * the phase is sent relative to the pseudorange the rover reconstructs.
 *
 * @tparam Observables A legacy observables model such as `gps_rtk_l1_l2_obs_ext` or `glonass_rtk_l1_obs`,
 * providing the resolutions.
 * @note The phase must lie within +-262 m of the pseudorange (20-bit DF012 / DF042); a larger offset is
 * reported as `out_of_range` by `encode_message()`.
 */
template <typename Observables, typename Satellite>
void set_legacy_l1(Satellite& sat, const observation& obs) noexcept
{
    const double pseudorange_m = obs.pseudorange_m[0U];
    sat.l1_ambiguity = static_cast<uint8_t>(pseudorange_m / Observables::AMBIGUITY_M);
    const double ambiguity_m = static_cast<double>(sat.l1_ambiguity) * Observables::AMBIGUITY_M;
    sat.l1_pseudorange = to_steps<uint32_t>(pseudorange_m - ambiguity_m, Observables::PSEUDORANGE_RESOLUTION_M);
    sat.l1_phase_minus_pseudorange = to_steps<int32_t>(obs.phase_range_m[0U] - l1_pseudorange_m<Observables>(sat),
                                                       Observables::PHASE_RESOLUTION_M);
    sat.l1_lock_time = LEGACY_LOCK_TIME_LONG;
    sat.l1_cnr = to_steps<uint8_t>(obs.cnr_dbhz[0U], Observables::CNR_RESOLUTION_DBHZ);
}

/** @brief Fills the L2 fields (1003/1004, 1011/1012) relative to the L1 pseudorange set by `set_legacy_l1()`. */
template <typename Observables, typename Satellite>
void set_legacy_l2(Satellite& sat, const observation& obs) noexcept
{
    const double l1_m = l1_pseudorange_m<Observables>(sat);
    sat.l2_pseudorange_difference = to_steps<int16_t>(obs.pseudorange_m[1U] - l1_m, Observables::PSEUDORANGE_RESOLUTION_M);
    sat.l2_phase_minus_l1_pseudorange = to_steps<int32_t>(obs.phase_range_m[1U] - l1_m, Observables::PHASE_RESOLUTION_M);
    sat.l2_lock_time = LEGACY_LOCK_TIME_LONG;
    sat.l2_cnr = to_steps<uint8_t>(obs.cnr_dbhz[1U], Observables::CNR_RESOLUTION_DBHZ);
}

/**
 * @brief Fills the masks, satellites and cells of an MSM4/5/7 message from one epoch.
 *
 * Every satellite is tracked on both `signal_ids`, so the cell mask is full.
 * `epoch` must be sorted by satellite ID and `signal_ids` ascending: MSM orders
 * satellites and cells by mask bit, i.e. by ID.
 *
 * Constellation specifics (epoch time, GLONASS extended satellite info) are set by the caller.
 */
template <typename Msm, castle::size_type N>
void fill_msm(Msm& msm, const observation (&epoch)[N], const castle::size_type (&signal_ids)[2U]) noexcept
{
    CASTLE_SAMPLE_CHECK(signal_ids[0U] < signal_ids[1U]);
    msm.signal_mask = Msm::signal_bit(signal_ids[0U]) | Msm::signal_bit(signal_ids[1U]);

    // DF402 (MSM4/5, 4 bit) / DF407 (MSM7, 10 bit): lock time indicator for a long continuous lock.
    const uint16_t lock_time = (Msm::LOCK_TIME_BITS == 10U) ? 500U : 12U;

    for (castle::size_type sat_index = 0U; sat_index < N; ++sat_index)
    {
        const observation& obs = epoch[sat_index];
        CASTLE_SAMPLE_CHECK((sat_index == 0U) || (epoch[sat_index - 1U].id < obs.id));
        msm.satellite_mask |= Msm::satellite_bit(obs.id);

        // The rough range (and rate) comes from the first signal; the cells carry the residuals.
        castle::protocols::rtcm::messages::msm_satellite sat{};
        CASTLE_IF_CONSTEXPR (Msm::HAS_RATE)
        {
            Msm::set_rough_range(sat, obs.pseudorange_m[0U] / LIGHT_M_PER_MS, obs.range_rate_m_s[0U]);
            CASTLE_SAMPLE_CHECK(sat.rough_phase_range_rate != Msm::INVALID_ROUGH_RATE);
        }
        else
        {
            Msm::set_rough_range(sat, obs.pseudorange_m[0U] / LIGHT_M_PER_MS);
        }
        CASTLE_SAMPLE_CHECK(sat.rough_range_ms != Msm::INVALID_ROUGH_RANGE);
        CASTLE_SAMPLE_CHECK(castle::succeeded(msm.satellites.push_back(sat)));

        for (castle::size_type sig_index = 0U; sig_index < 2U; ++sig_index)
        {
            msm.cell_mask |= Msm::cell_bit(sat_index, sig_index, N, 2U);

            castle::protocols::rtcm::messages::msm_signal cell{};
            Msm::set_pseudorange(sat, cell, obs.pseudorange_m[sig_index] / LIGHT_M_PER_MS);
            Msm::set_phase_range(sat, cell, obs.phase_range_m[sig_index] / LIGHT_M_PER_MS);
            CASTLE_IF_CONSTEXPR (Msm::HAS_RATE)
            {
                Msm::set_phase_range_rate(sat, cell, obs.range_rate_m_s[sig_index]);
                CASTLE_SAMPLE_CHECK(cell.fine_phase_range_rate != Msm::INVALID_FINE_RATE);
            }
            Msm::set_cnr(cell, obs.cnr_dbhz[sig_index]);
            cell.lock_time = lock_time;
            CASTLE_SAMPLE_CHECK(cell.fine_pseudorange != Msm::INVALID_FINE_PSEUDORANGE);
            CASTLE_SAMPLE_CHECK(cell.fine_phase_range != Msm::INVALID_FINE_PHASE_RANGE);
            CASTLE_SAMPLE_CHECK(castle::succeeded(msm.signals.push_back(cell)));
        }
    }
}

/** @brief Payload bytes of an MSM message with `satellites` x 2 signals, all cells present. */
template <typename Msm>
CASTLE_CONSTEXPR castle::size_type msm_payload_bytes(castle::size_type satellites) noexcept
{
    // 169 header bits + cell mask + per-satellite + per-cell bits (see msm.md).
    return (169U + (satellites * 2U)
            + (satellites * (Msm::HAS_RATE ? 36U : 18U))
            + (satellites * 2U * (1U + Msm::PSEUDORANGE_BITS + Msm::PHASE_RANGE_BITS + Msm::LOCK_TIME_BITS
                                  + Msm::CNR_BITS + (Msm::HAS_RATE ? 15U : 0U)))
            + 7U)
         / 8U;
}

} // namespace rtcm_sample

#endif // CASTLE_SAMPLE_UBX_RTCM_INPUT_SUPPORT_HPP
