#include "sample_support.hpp"

#include "castle_ext/protocols/rtcm/messages/messages.hpp"

namespace
{

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

/** @brief Encodes `model` at `stream[used]`, checks the appended frame and advances `used`. */
template <typename Model>
void append(const Model& model, uint16_t type, uint8_t* stream, castle::size_type capacity, castle::size_type& used)
{
    castle::size_type written = 0U;
    const castle::status result = encode_message(model, &stream[used], capacity - used, written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    CASTLE_SAMPLE_CHECK(written > RTCM_FRAME_OVERHEAD);
    CASTLE_SAMPLE_CHECK(stream[used] == RTCM_PREAMBLE);
    // The 12 bit message number leads every payload.
    CASTLE_SAMPLE_CHECK(((stream[used + RTCM_FRAME_INDEX_PAYLOAD] << 4U) | (stream[used + RTCM_FRAME_INDEX_PAYLOAD + 1U] >> 4U)) == type);
    // A frame including its own CRC-24Q has residue 0.
    CASTLE_SAMPLE_CHECK(castle::checksums::crc24q::calculate(&stream[used], written) == 0U);
    used += written;
}

/** @brief Appends one legacy observables message per alias, all carrying the satellites of `source`. */
template <typename Message, typename Source>
void append_as(const Source& source, uint8_t* stream, castle::size_type capacity, castle::size_type& used)
{
    Message message;
    message.station_id = source.station_id;
    message.satellites = source.satellites; // Same capacity, so the vectors are one type.
    append(message, Message::MESSAGE_TYPE, stream, capacity, used);
}

} // namespace

int main()
{
    // One buffer receives a complete correction epoch: every model of the umbrella once.
    uint8_t stream[4096U] = {};
    castle::size_type used = 0U;

    // Station information (1005, 1006, 1007, 1033, 1230).
    station_arp arp;
    arp.station_id = 1234U;
    arp.set_position_m(4027894.0060, 307045.6000, 4919474.9100);
    append(arp, RTCM_TYPE_STATION_ARP, stream, sizeof(stream), used);
    station_arp_height arp_height;
    arp_height.station_id = arp.station_id;
    arp_height.set_position_m(4027894.0060, 307045.6000, 4919474.9100);
    arp_height.set_antenna_height_m(1.5);
    append(arp_height, RTCM_TYPE_STATION_ARP_HEIGHT, stream, sizeof(stream), used);

    antenna_descriptor antenna;
    antenna.station_id = 1234U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(antenna.descriptor.assign("TRM59800.00     NONE")));
    append(antenna, RTCM_TYPE_ANTENNA_DESCRIPTOR, stream, sizeof(stream), used);

    receiver_antenna_descriptor receiver;
    receiver.station_id = 1234U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(receiver.antenna.assign("ANT")));
    CASTLE_SAMPLE_CHECK(castle::succeeded(receiver.receiver_type.assign("UBLOX ZED-F9P")));
    append(receiver, RTCM_TYPE_RECEIVER_ANTENNA, stream, sizeof(stream), used);

    glonass_code_phase_biases biases;
    biases.station_id = 1234U;
    biases.signals_mask = glonass_code_phase_biases::MASK_L1_CA | glonass_code_phase_biases::MASK_L2_P;
    biases.l1_ca_bias = -100;
    biases.l2_p_bias = 50;
    append(biases, RTCM_TYPE_GLO_BIASES, stream, sizeof(stream), used);

    // Legacy RTK observables (1001-1004, 1009-1012).
    gps_rtk_l1_obs gps;
    gps_rtk_satellite gps_satellite;
    gps_satellite.satellite_id = 5U;
    gps_satellite.l1_pseudorange = 12345678U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(gps.satellites.push_back(gps_satellite)));
    append(gps, RTCM_TYPE_GPS_L1_OBS, stream, sizeof(stream), used);
    append_as<gps_rtk_l1_obs_ext>(gps, stream, sizeof(stream), used);
    append_as<gps_rtk_l1_l2_obs>(gps, stream, sizeof(stream), used);
    append_as<gps_rtk_l1_l2_obs_ext>(gps, stream, sizeof(stream), used);

    glonass_rtk_l1_obs glonass;
    glonass_rtk_satellite glonass_satellite;
    glonass_satellite.satellite_id = 5U;
    glonass_satellite.frequency_channel = 8U;
    glonass_satellite.l1_pseudorange = 22345678U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(glonass.satellites.push_back(glonass_satellite)));
    append(glonass, RTCM_TYPE_GLO_L1_OBS, stream, sizeof(stream), used);
    append_as<glonass_rtk_l1_obs_ext>(glonass, stream, sizeof(stream), used);
    append_as<glonass_rtk_l1_l2_obs>(glonass, stream, sizeof(stream), used);
    append_as<glonass_rtk_l1_l2_obs_ext>(glonass, stream, sizeof(stream), used);

    // Multiple Signal Messages: one satellite, one signal, one cell.
    gps_msm7 msm;
    msm.station_id = 1234U;
    msm.satellite_mask = gps_msm7::satellite_bit(3U);
    msm.signal_mask = gps_msm7::signal_bit(2U);
    msm.cell_mask = gps_msm7::cell_bit(0U, 0U, 1U, 1U);
    msm_satellite msm_sat;
    gps_msm7::set_rough_range(msm_sat, 70.123456, -123.4);
    CASTLE_SAMPLE_CHECK(castle::succeeded(msm.satellites.push_back(msm_sat)));
    msm_signal cell;
    gps_msm7::set_pseudorange(msm_sat, cell, 70.1235);
    gps_msm7::set_phase_range(msm_sat, cell, 70.12352);
    gps_msm7::set_cnr(cell, 42.0);
    gps_msm7::set_phase_range_rate(msm_sat, cell, -123.39);
    CASTLE_SAMPLE_CHECK(castle::succeeded(msm.signals.push_back(cell)));
    append(msm, gps_msm7::MESSAGE_TYPE, stream, sizeof(stream), used);

    // Each model accepts exactly its own message number.
    CASTLE_SAMPLE_CHECK(gps_rtk_l1_obs_ext::accepts(1002U) && !gps_rtk_l1_obs_ext::accepts(1003U));
    CASTLE_SAMPLE_CHECK(!gps_rtk_l1_obs_ext::accepts(1010U));
    CASTLE_SAMPLE_CHECK(glonass_rtk_l1_obs_ext::accepts(1010U) && !glonass_rtk_l1_obs_ext::accepts(1002U));
    CASTLE_SAMPLE_CHECK(station_arp_height::accepts(1006U) && !station_arp::accepts(1006U));
    CASTLE_SAMPLE_CHECK(!station_arp_height::accepts(1007U));
    CASTLE_SAMPLE_CHECK(antenna_descriptor::accepts(1007U));
    CASTLE_SAMPLE_CHECK(receiver_antenna_descriptor::accepts(1033U));
    CASTLE_SAMPLE_CHECK(glonass_code_phase_biases::accepts(1230U));
    CASTLE_SAMPLE_CHECK(gps_msm7::accepts(1077U) && !gps_msm7::accepts(1074U));

    CASTLE_SAMPLE_CHECK(used > 0U);
    CASTLE_SAMPLE_CHECK(used < sizeof(stream));
    return 0;
}
