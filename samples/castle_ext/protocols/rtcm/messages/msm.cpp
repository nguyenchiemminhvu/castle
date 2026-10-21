#include "sample_support.hpp"

#include "castle_ext/checksums/crc24q.hpp"
#include "castle_ext/protocols/rtcm/messages/msm.hpp"

#include <math.h>

namespace
{

/** @brief Speed of light in m/s; dividing a range in metres by it gives light-milliseconds * 1000. */
CASTLE_CONSTEXPR double LIGHT_MS = 299792.458;

/** @brief Converts a range in metres to light-milliseconds, the unit of every MSM range field. */
double metres_to_ms(double metres)
{
    return metres / LIGHT_MS;
}

/** @brief One satellite as measured on two signals. */
struct tracked_satellite
{
    castle::size_type prn;
    double pseudorange_m[2U];
    double phase_range_m[2U];
    double rate_m_s[2U];
    double cnr_dbhz[2U];
};

} // namespace

int main()
{
    using namespace castle::protocols::rtcm;
    using namespace castle::protocols::rtcm::messages;

    // -----------------------------------------------------------------------
    // 1. A realistic GPS MSM7 epoch: 3 satellites x 2 signals from physical ranges.
    // -----------------------------------------------------------------------
    static const tracked_satellite satellites[3U] = {
        {5U, {21345678.123, 21345680.456}, {21345677.900, 21345679.800}, {-412.350, -412.338}, {47.25, 41.50}},
        {12U, {23098765.432, 23098768.010}, {23098764.100, 23098766.700}, {120.500, 120.511}, {44.00, 38.75}},
        {25U, {20876543.210, 20876545.600}, {20876542.800, 20876545.100}, {-2250.700, -2250.688}, {51.50, 45.0625}},
    };

    // Signal IDs from RTCM 10403.3 Table 3.5-91: 2 = L1 C/A, 16 = L2C(L).
    CASTLE_CONSTEXPR castle::size_type SIGNAL_L1_CA = 2U;
    CASTLE_CONSTEXPR castle::size_type SIGNAL_L2C_L = 16U;

    gps_msm7 epoch{};
    CASTLE_SAMPLE_CHECK(gps_msm7::MESSAGE_TYPE == 1077U);
    CASTLE_SAMPLE_CHECK(gps_msm7::accepts(1077U));
    CASTLE_SAMPLE_CHECK(!gps_msm7::accepts(1074U));

    epoch.station_id = 1234U;
    epoch.epoch_time = 345600000U + 1000U; // GPS time of week in milliseconds
    epoch.multiple_message = false;
    epoch.iods = 0U;
    epoch.clock_steering = 1U;   // not applied
    epoch.external_clock = 0U;
    epoch.divergence_free = false;
    epoch.smoothing_interval = 0U;

    // Masks: satellite and signal bits are MSB-first; the cell mask is satellite-major over the
    // *selected* satellites and signals, here all 3 x 2 cells.
    for (const tracked_satellite& tracked : satellites)
    {
        epoch.satellite_mask |= gps_msm7::satellite_bit(tracked.prn);
    }
    epoch.signal_mask = gps_msm7::signal_bit(SIGNAL_L1_CA) | gps_msm7::signal_bit(SIGNAL_L2C_L);
    for (castle::size_type sat_index = 0U; sat_index < 3U; ++sat_index)
    {
        for (castle::size_type sig_index = 0U; sig_index < 2U; ++sig_index)
        {
            epoch.cell_mask |= gps_msm7::cell_bit(sat_index, sig_index, 3U, 2U);
        }
    }
    CASTLE_SAMPLE_CHECK(epoch.satellite_mask == (gps_msm7::satellite_bit(5U) | gps_msm7::satellite_bit(12U)
                                                 | gps_msm7::satellite_bit(25U)));
    CASTLE_SAMPLE_CHECK(epoch.signal_mask == 0x40010000U);
    CASTLE_SAMPLE_CHECK(epoch.cell_mask == 0x3FU); // 6 cells, all present

    // Satellites: rough range (from the first signal) and rough phase-range rate.
    for (const tracked_satellite& tracked : satellites)
    {
        msm_satellite sat{};
        gps_msm7::set_rough_range(sat, metres_to_ms(tracked.pseudorange_m[0U]), tracked.rate_m_s[0U]);
        CASTLE_SAMPLE_CHECK(sat.rough_range_ms != gps_msm7::INVALID_ROUGH_RANGE);
        CASTLE_SAMPLE_CHECK(sat.rough_phase_range_rate != gps_msm7::INVALID_ROUGH_RATE);
        CASTLE_SAMPLE_CHECK(castle::succeeded(epoch.satellites.push_back(sat)));
    }
    CASTLE_SAMPLE_CHECK(epoch.satellites[0U].rough_range_ms == 71U); // 21345678.123 m is 71.2 ms
    CASTLE_SAMPLE_CHECK(epoch.satellites[0U].rough_phase_range_rate == -412);

    // Cells, in cell-mask order: satellite 0 L1, satellite 0 L2, satellite 1 L1, ...
    for (castle::size_type sat_index = 0U; sat_index < 3U; ++sat_index)
    {
        const msm_satellite& sat = epoch.satellites[sat_index];
        const tracked_satellite& tracked = satellites[sat_index];
        for (castle::size_type sig_index = 0U; sig_index < 2U; ++sig_index)
        {
            msm_signal cell{};
            gps_msm7::set_pseudorange(sat, cell, metres_to_ms(tracked.pseudorange_m[sig_index]));
            gps_msm7::set_phase_range(sat, cell, metres_to_ms(tracked.phase_range_m[sig_index]));
            gps_msm7::set_phase_range_rate(sat, cell, tracked.rate_m_s[sig_index]);
            gps_msm7::set_cnr(cell, tracked.cnr_dbhz[sig_index]);
            cell.lock_time = 600U; // DF407 lock time indicator
            cell.half_cycle = false;

            CASTLE_SAMPLE_CHECK(cell.fine_pseudorange != gps_msm7::INVALID_FINE_PSEUDORANGE);
            CASTLE_SAMPLE_CHECK(cell.fine_phase_range != gps_msm7::INVALID_FINE_PHASE_RANGE);
            CASTLE_SAMPLE_CHECK(cell.fine_phase_range_rate != gps_msm7::INVALID_FINE_RATE);

            // Rebuilding the range from the wire integers lands within one LSB (~0.6 mm).
            const double rough_ms = static_cast<double>(sat.rough_range_ms)
                                  + static_cast<double>(sat.rough_range_mod) / gps_msm7::ROUGH_RANGE_STEPS_PER_MS;
            const double rebuilt_m =
                (rough_ms + static_cast<double>(cell.fine_pseudorange) / gps_msm7::PSEUDORANGE_STEPS_PER_MS) * LIGHT_MS;
            CASTLE_SAMPLE_CHECK(fabs(rebuilt_m - tracked.pseudorange_m[sig_index]) < 0.001);
            const double rebuilt_phase_m =
                (rough_ms + static_cast<double>(cell.fine_phase_range) / gps_msm7::PHASE_RANGE_STEPS_PER_MS) * LIGHT_MS;
            CASTLE_SAMPLE_CHECK(fabs(rebuilt_phase_m - tracked.phase_range_m[sig_index]) < 0.001);

            CASTLE_SAMPLE_CHECK(castle::succeeded(epoch.signals.push_back(cell)));
        }
    }
    CASTLE_SAMPLE_CHECK(epoch.satellites.size() == 3U);
    CASTLE_SAMPLE_CHECK(epoch.signals.size() == 6U);
    CASTLE_SAMPLE_CHECK(epoch.signals[0U].cnr == 756U); // 47.25 dB-Hz * 16

    uint8_t frame[RTCM_MAX_FRAME_SIZE] = {};
    castle::size_type written = 0U;
    castle::status result = encode_message(epoch, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));

    // Payload: 169 header bits + 6 cell-mask bits + 3 * 36 satellite bits + 6 * 80 cell bits = 763 bits.
    CASTLE_SAMPLE_CHECK(((169U + 6U + (3U * 36U) + (6U * 80U) + 7U) / 8U) == 96U);
    const castle::size_type payload_bytes = 96U;
    CASTLE_SAMPLE_CHECK(written == frame_size(payload_bytes));
    CASTLE_SAMPLE_CHECK(written == 102U);

    // Frame header: preamble, 6 reserved zero bits + 10-bit length.
    CASTLE_SAMPLE_CHECK(frame[0U] == RTCM_PREAMBLE);
    CASTLE_SAMPLE_CHECK(frame[1U] == 0U);
    CASTLE_SAMPLE_CHECK(frame[2U] == payload_bytes);

    // Payload starts with DF002 = 1077 (12 bits) and DF003 = 1234 (12 bits).
    CASTLE_SAMPLE_CHECK(frame[3U] == 0x43U);
    CASTLE_SAMPLE_CHECK((frame[4U] >> 4U) == 0x5U);
    CASTLE_SAMPLE_CHECK(((static_cast<uint16_t>(frame[4U] & 0x0FU) << 8U) | frame[5U]) == 1234U);
    CASTLE_SAMPLE_CHECK(((frame[4U] & 0x0FU) == 0x4U) && (frame[5U] == 0xD2U));

    // The satellite mask starts at payload bit 73; satellite ID 5 is its 5th bit from the MSB (bit 77).
    CASTLE_SAMPLE_CHECK(frame[RTCM_FRAME_INDEX_PAYLOAD + 9U] == 0x04U);

    // Trailing CRC-24Q covers preamble, length and payload.
    const castle::size_type covered = RTCM_FRAME_INDEX_PAYLOAD + payload_bytes;
    const uint32_t crc = castle::checksums::crc24q::calculate(frame, covered);
    CASTLE_SAMPLE_CHECK(frame[covered] == static_cast<uint8_t>(crc >> 16U));
    CASTLE_SAMPLE_CHECK(frame[covered + 1U] == static_cast<uint8_t>(crc >> 8U));
    CASTLE_SAMPLE_CHECK(frame[covered + 2U] == static_cast<uint8_t>(crc));
    CASTLE_SAMPLE_CHECK(castle::checksums::crc24q::calculate(frame, written) == 0U); // CRC over frame+CRC is zero

    // -----------------------------------------------------------------------
    // 2. Encode-time validation and buffer handling.
    // -----------------------------------------------------------------------
    uint8_t exact[102U] = {};
    CASTLE_SAMPLE_CHECK(encode_message(epoch, exact, sizeof(exact), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == 102U);
    CASTLE_SAMPLE_CHECK(encode_message(epoch, exact, sizeof(exact) - 1U, written) == castle::status::full);
    CASTLE_SAMPLE_CHECK(written == 0U);
    CASTLE_SAMPLE_CHECK(encode_message(epoch, nullptr, 0U, written) == castle::status::invalid_argument);

    // The masks and the vectors must agree: one satellite entry per set mask bit.
    gps_msm7 broken = epoch;
    broken.satellites.pop_back();
    CASTLE_SAMPLE_CHECK(encode_message(broken, frame, sizeof(frame), written) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(written == 0U);

    // Too many cells: 13 satellites x 5 signals = 65 > 64.
    gps_msm7 oversize{};
    for (castle::size_type id = 1U; id <= 13U; ++id)
    {
        oversize.satellite_mask |= gps_msm7::satellite_bit(id);
        CASTLE_SAMPLE_CHECK(castle::succeeded(oversize.satellites.push_back(msm_satellite{})));
    }
    for (castle::size_type id = 1U; id <= 5U; ++id)
    {
        oversize.signal_mask |= gps_msm7::signal_bit(id);
    }
    CASTLE_SAMPLE_CHECK(gps_msm7::cell_bit(0U, 0U, 13U, 5U) == 0U); // the helper refuses such a grid as well
    CASTLE_SAMPLE_CHECK(encode_message(oversize, frame, sizeof(frame), written) == castle::status::invalid_argument);

    // A field that does not fit its wire width is reported, never truncated.
    gps_msm7 wide = epoch;
    wide.station_id = 4096U; // DF003 has 12 bits
    CASTLE_SAMPLE_CHECK(encode_message(wide, frame, sizeof(frame), written) == castle::status::out_of_range);

    // Out-of-range indices and ids yield an empty bit, never a wrong one.
    CASTLE_SAMPLE_CHECK(gps_msm7::satellite_bit(0U) == 0U);
    CASTLE_SAMPLE_CHECK(gps_msm7::satellite_bit(65U) == 0U);
    CASTLE_SAMPLE_CHECK(gps_msm7::signal_bit(33U) == 0U);
    CASTLE_SAMPLE_CHECK(gps_msm7::cell_bit(3U, 0U, 3U, 2U) == 0U);

    // Values outside the representable range become the documented invalid markers.
    msm_satellite far_away{};
    gps_msm7::set_rough_range(far_away, 300.0, 9000.0); // > 255 ms, > 8191 m/s
    CASTLE_SAMPLE_CHECK(far_away.rough_range_ms == gps_msm7::INVALID_ROUGH_RANGE);
    CASTLE_SAMPLE_CHECK(far_away.rough_phase_range_rate == gps_msm7::INVALID_ROUGH_RATE);
    msm_signal lost{};
    gps_msm7::set_pseudorange(epoch.satellites[0U], lost, 5.0); // cannot be within 1/1024 ms of the rough range
    CASTLE_SAMPLE_CHECK(lost.fine_pseudorange == gps_msm7::INVALID_FINE_PSEUDORANGE);
    gps_msm7::set_cnr(lost, 200.0); // saturates at 1023 / 16 dB-Hz
    CASTLE_SAMPLE_CHECK(lost.cnr == 1023U);

    // -----------------------------------------------------------------------
    // 3. GLONASS MSM4: day-of-week epoch, no rate fields, coarser quantisation.
    // -----------------------------------------------------------------------
    glonass_msm4 glonass{};
    CASTLE_SAMPLE_CHECK(glonass_msm4::MESSAGE_TYPE == 1084U);
    CASTLE_SAMPLE_CHECK(!glonass_msm4::HAS_RATE);
    glonass.station_id = 1234U;
    glonass.set_glonass_epoch(2U, 43200000U); // Tuesday, 12:00:00 in GLONASS time
    CASTLE_SAMPLE_CHECK(glonass.epoch_time == ((2U << 27U) | 43200000U));
    glonass.satellite_mask = glonass_msm4::satellite_bit(3U);
    glonass.signal_mask = glonass_msm4::signal_bit(2U);
    glonass.cell_mask = glonass_msm4::cell_bit(0U, 0U, 1U, 1U);

    msm_satellite glo_sat{};
    glonass_msm4::set_rough_range(glo_sat, metres_to_ms(19876543.21));
    msm_signal glo_cell{};
    glonass_msm4::set_pseudorange(glo_sat, glo_cell, metres_to_ms(19876543.21));
    glonass_msm4::set_phase_range(glo_sat, glo_cell, metres_to_ms(19876542.00));
    glonass_msm4::set_cnr(glo_cell, 42.4); // 1 dB-Hz resolution
    glo_cell.lock_time = 7U;
    glo_cell.half_cycle = true;
    CASTLE_SAMPLE_CHECK(glo_cell.cnr == 42U);
    CASTLE_SAMPLE_CHECK(castle::succeeded(glonass.satellites.push_back(glo_sat)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(glonass.signals.push_back(glo_cell)));

    result = encode_message(glonass, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    // 169 + 1 + 18 + 48 = 236 bits -> 30 payload bytes.
    CASTLE_SAMPLE_CHECK(written == frame_size(30U));
    CASTLE_SAMPLE_CHECK(frame[2U] == 30U);
    CASTLE_SAMPLE_CHECK((frame[RTCM_FRAME_INDEX_PAYLOAD + 3U] >> 5U) == 2U); // day of week sits in the epoch's top 3 bits

    // The other systems only differ in the message number.
    CASTLE_SAMPLE_CHECK(galileo_msm5::MESSAGE_TYPE == 1095U);
    CASTLE_SAMPLE_CHECK(beidou_msm7::MESSAGE_TYPE == 1127U);

    return 0;
}
