/**
 * @file rtcm_input.cpp
 * @brief End-to-end RTCM 3.3 correction input to a u-blox ZED-F9R (UBX-19056845 chapter 4).
 *
 * The flow a host (base-station link, NTRIP client, radio modem bridge) follows:
 *
 *  1. Configure the receiver port with UBX-CFG-VALSET (UBX frames, `0xb5 0x62`):
 *     accept RTCM 3 input, optionally filter by reference station ID, and enable
 *     the UBX-RXM-RTCM status output.
 *  2. Write RTCM 3 frames (`0xd3 ... CRC-24Q`) to the same port, unchanged.
 *     The u-blox "Class/ID 0xf5 0xNN" of `RTCM-3X-TYPEnnnn` is not sent: it is
 *     the catalogue identifier u-blox gives every message (NMEA GGA is "0xf0
 *     0x00" in the same document), not a UBX header.
 *  3. Read back one UBX-RXM-RTCM (0x02 0x32) per RTCM frame the receiver parsed.
 *
 * Per-type construction of every message in the u-blox list is shown in the
 * sibling samples `rtcm_3x_gps_observables.cpp`, `rtcm_3x_glonass_observables.cpp`,
 * `rtcm_3x_station.cpp` and `rtcm_3x_msm.cpp`.
 */
#include "rtcm_input_support.hpp"

#include "castle_ext/protocols/rtcm/messages/msm.hpp"
#include "castle_ext/protocols/rtcm/messages/station_arp.hpp"
#include "castle_ext/protocols/ubx/messages/cfg_valset.hpp"
#include "castle_ext/protocols/ubx/messages/rxm_rtcm.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"

int main()
{
    namespace rtcm = castle::protocols::rtcm;
    namespace ubx = castle::protocols::ubx;
    using castle::container::array_view;
    using ubx::config::config_entry;
    using ubx::config::config_value;
    using namespace rtcm_sample;

    // -------------------------------------------------------------------------
    // 1. Configuration (UBX). RTCM3X input is enabled by default on UART1, UART2,
    //    USB, I2C and SPI; setting it explicitly documents the dependency.
    // -------------------------------------------------------------------------
    ubx::messages::cfg_valset<4U> config{};
    config.layers = ubx::config::config_layer::ram | ubx::config::config_layer::bbr;
    CASTLE_SAMPLE_CHECK(castle::succeeded(config.entries.push_back(config_entry{CFG_UART1INPROT_RTCM3X, config_value(true)})));
    // Relaxed DF003 filter: drop RTCM messages from any other base station.
    CASTLE_SAMPLE_CHECK(castle::succeeded(config.entries.push_back(config_entry{CFG_RTCM_DF003_IN, config_value(STATION_ID)})));
    CASTLE_SAMPLE_CHECK(castle::succeeded(
        config.entries.push_back(config_entry{CFG_RTCM_DF003_IN_FILTER, config_value(DF003_FILTER_RELAXED)})));
    // One UBX-RXM-RTCM per parsed RTCM input message on UART1.
    CASTLE_SAMPLE_CHECK(castle::succeeded(
        config.entries.push_back(config_entry{CFG_MSGOUT_UBX_RXM_RTCM_UART1, config_value(static_cast<uint8_t>(1U))})));

    CASTLE_SAMPLE_CHECK(castle::succeeded(uart1.send_ubx(config)));
    const array_view<const uint8_t> config_frame = uart1.last_frame();
    CASTLE_SAMPLE_CHECK(config_frame[0U] == ubx::UBX_SYNC_CHAR_1); // a real UBX frame: 0xb5 0x62 0x06 0x8a ...
    CASTLE_SAMPLE_CHECK(config_frame[1U] == ubx::UBX_SYNC_CHAR_2);
    CASTLE_SAMPLE_CHECK(config_frame[2U] == ubx::UBX_CLASS_CFG);
    CASTLE_SAMPLE_CHECK(config_frame[3U] == ubx::UBX_ID_CFG_VALSET);
    // The receiver answers with UBX-ACK-ACK (see the ack message model); omitted here.

    // -------------------------------------------------------------------------
    // 2. Corrections (RTCM 3). The base position first, then the observations.
    // -------------------------------------------------------------------------
    rtcm::messages::station_arp_height arp{}; // RTCM-3X-TYPE1006
    arp.station_id = STATION_ID;
    arp.gps = true;
    arp.glonass = true;
    arp.galileo = true;
    arp.set_position_m(STATION_ECEF_M[0U], STATION_ECEF_M[1U], STATION_ECEF_M[2U]);
    arp.set_antenna_height_m(1.5);
    CASTLE_SAMPLE_CHECK(castle::succeeded(uart1.send_rtcm(arp)));
    check_rtcm_frame(uart1, rtcm::RTCM_TYPE_STATION_ARP_HEIGHT, STATION_ID);
    const castle::size_type arp_start = uart1.last_start;

    rtcm::messages::gps_msm7 gps{}; // RTCM-3X-TYPE1077
    gps.station_id = STATION_ID;
    gps.epoch_time = GPS_TOW_MS;
    gps.multiple_message = false; // the only MSM of this epoch
    fill_msm(gps, GPS_EPOCH, GPS_SIGNALS);
    CASTLE_SAMPLE_CHECK(castle::succeeded(uart1.send_rtcm(gps)));
    check_rtcm_frame(uart1, rtcm::messages::gps_msm7::MESSAGE_TYPE, STATION_ID);

    // The RTCM frame is exactly the u-blox payload table: preamble 0xd3, numData, data, crc.
    // No 0xf5 byte and no UBX header precede it, so it is no UBX frame.
    const array_view<const uint8_t> arp_frame(&uart1.bytes[arp_start], uart1.last_start - arp_start);
    CASTLE_SAMPLE_CHECK(arp_frame[0U] == rtcm::RTCM_PREAMBLE);
    CASTLE_SAMPLE_CHECK(arp_frame.size() == rtcm::frame_size(21U)); // "Message Size: 6 + numData"
    ubx::message_view not_ubx{};
    CASTLE_SAMPLE_CHECK(!ubx::decode_frame(arp_frame, not_ubx));

    // One UART transmit buffer, three frames back to back: UBX, RTCM, RTCM.
    CASTLE_SAMPLE_CHECK(uart1.size == config_frame.size() + arp_frame.size() + uart1.last_frame().size());

    // -------------------------------------------------------------------------
    // 3. Feedback (UBX). The receiver emits UBX-RXM-RTCM for each parsed RTCM frame.
    //    Its reply to the 1077 above is simulated here with the same message model.
    // -------------------------------------------------------------------------
    ubx::messages::rxm_rtcm reply{};
    reply.version = 0x02U;
    reply.flags = 0x04U; // crcFailed = 0, msgUsed = 2 (used)
    reply.ref_station = STATION_ID;
    reply.msg_type = rtcm::messages::gps_msm7::MESSAGE_TYPE;
    uint8_t rx[32U] = {};
    castle::size_type received = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(ubx::encode_message(reply, rx, sizeof(rx), received)));

    ubx::message_view view{};
    CASTLE_SAMPLE_CHECK(ubx::decode_frame(array_view<const uint8_t>(rx, received), view));
    ubx::messages::rxm_rtcm status{};
    CASTLE_SAMPLE_CHECK(ubx::messages::rxm_rtcm::decode(view, status));
    CASTLE_SAMPLE_CHECK(!status.crc_failed());
    CASTLE_SAMPLE_CHECK(status.msg_used() == ubx::messages::rxm_rtcm_msg_used::used);
    CASTLE_SAMPLE_CHECK(status.msg_type == 1077U);     // DF002 of the frame we sent
    CASTLE_SAMPLE_CHECK(status.ref_station == STATION_ID); // DF003 of the frame we sent
    return 0;
}
