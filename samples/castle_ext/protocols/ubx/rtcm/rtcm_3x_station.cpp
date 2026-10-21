/**
 * @file rtcm_3x_station.cpp
 * @brief u-blox RTCM-3X-TYPE1005, 1006, 1007, 1033 and 1230: reference station information.
 *
 * | u-blox message      | Class/ID  | Castle model                                        |
 * |---------------------|-----------|-----------------------------------------------------|
 * | RTCM-3X-TYPE1005    | 0xf5 0x05 | `station_arp`                                       |
 * | RTCM-3X-TYPE1006    | 0xf5 0x06 | `station_arp_height`                                |
 * | RTCM-3X-TYPE1007    | 0xf5 0x07 | `antenna_descriptor`                                |
 * | RTCM-3X-TYPE1033    | 0xf5 0x21 | `receiver_antenna_descriptor`                       |
 * | RTCM-3X-TYPE1230    | 0xf5 0xe6 | `glonass_code_phase_biases`                         |
 *
 * A rover cannot use observations without the base position, so 1005 or 1006
 * goes out first and is repeated regularly (typically every 5 to 10 s).
 */
#include "rtcm_input_support.hpp"

#include "castle_ext/protocols/rtcm/messages/antenna_descriptor.hpp"
#include "castle_ext/protocols/rtcm/messages/glonass_code_phase_biases.hpp"
#include "castle_ext/protocols/rtcm/messages/receiver_antenna_descriptor.hpp"
#include "castle_ext/protocols/rtcm/messages/station_arp.hpp"

namespace
{

/** @brief The base station ARP as `Message` (`station_arp` or `station_arp_height`). */
template <typename Message>
Message make_arp()
{
    using namespace rtcm_sample;

    Message arp{};
    arp.station_id = STATION_ID;
    arp.itrf_year = 0U;            // DF021: 0 = no specific realization
    arp.gps = true;                // DF022..DF024: services provided by the station
    arp.glonass = true;
    arp.galileo = true;
    arp.reference_station_indicator = false; // DF141: physical station
    arp.single_receiver_oscillator = true;   // DF142
    arp.set_position_m(STATION_ECEF_M[0U], STATION_ECEF_M[1U], STATION_ECEF_M[2U]);
    return arp;
}

} // namespace
int main()
{
    using namespace castle::protocols::rtcm;
    using namespace castle::protocols::rtcm::messages;
    using namespace rtcm_sample;

    // -------------------------------------------------------------------------
    // RTCM-3X-TYPE1005 (0xf5 0x05): stationary RTK reference station ARP.
    // -------------------------------------------------------------------------
    const station_arp arp = make_arp<station_arp>();
    CASTLE_SAMPLE_CHECK(arp.ecef_x == 40278940060);  // DF025 in 0.0001 m

    CASTLE_SAMPLE_CHECK(castle::succeeded(uart1.send_rtcm(arp)));
    check_rtcm_frame(uart1, RTCM_TYPE_STATION_ARP, STATION_ID);
    CASTLE_SAMPLE_CHECK(uart1.last_frame().size() == frame_size(19U)); // 152 bits

    // -------------------------------------------------------------------------
    // RTCM-3X-TYPE1006 (0xf5 0x06): the same ARP plus antenna height (DF028).
    // -------------------------------------------------------------------------
    station_arp_height arp_height = make_arp<station_arp_height>();
    arp_height.set_antenna_height_m(1.5);
    CASTLE_SAMPLE_CHECK(castle::succeeded(uart1.send_rtcm(arp_height)));
    check_rtcm_frame(uart1, RTCM_TYPE_STATION_ARP_HEIGHT, STATION_ID);
    CASTLE_SAMPLE_CHECK(uart1.last_frame().size() == frame_size(21U)); // 168 bits

    // -------------------------------------------------------------------------
    // RTCM-3X-TYPE1007 (0xf5 0x07): antenna descriptor (IGS antenna name + radome).
    // -------------------------------------------------------------------------
    antenna_descriptor antenna{};
    antenna.station_id = STATION_ID;
    CASTLE_SAMPLE_CHECK(castle::succeeded(antenna.descriptor.assign("TRM59800.00     NONE"))); // DF029/DF030
    antenna.setup_id = 0U;                                                                    // DF031
    CASTLE_SAMPLE_CHECK(castle::succeeded(uart1.send_rtcm(antenna)));
    check_rtcm_frame(uart1, RTCM_TYPE_ANTENNA_DESCRIPTOR, STATION_ID);
    CASTLE_SAMPLE_CHECK(uart1.last_frame().size() == frame_size(5U + 20U));

    // -------------------------------------------------------------------------
    // RTCM-3X-TYPE1033 (0xf5 0x21): receiver and antenna descriptors.
    // -------------------------------------------------------------------------
    receiver_antenna_descriptor receiver{};
    receiver.station_id = STATION_ID;
    CASTLE_SAMPLE_CHECK(castle::succeeded(receiver.antenna.assign("TRM59800.00     NONE")));
    receiver.antenna_setup_id = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(receiver.antenna_serial.assign("5000118716")));
    CASTLE_SAMPLE_CHECK(castle::succeeded(receiver.receiver_type.assign("U-BLOX ZED-F9P")));
    CASTLE_SAMPLE_CHECK(castle::succeeded(receiver.firmware.assign("HPG 1.32")));
    CASTLE_SAMPLE_CHECK(castle::succeeded(receiver.receiver_serial.assign("")));
    CASTLE_SAMPLE_CHECK(castle::succeeded(uart1.send_rtcm(receiver)));
    check_rtcm_frame(uart1, RTCM_TYPE_RECEIVER_ANTENNA, STATION_ID);

    // -------------------------------------------------------------------------
    // RTCM-3X-TYPE1230 (0xf5 0xe6): GLONASS L1 and L2 code-phase biases (0.02 m).
    // Needed when base and rover receivers come from different manufacturers.
    // -------------------------------------------------------------------------
    glonass_code_phase_biases biases{};
    biases.station_id = STATION_ID;
    biases.code_phase_bias_indicator = false; // DF421: observations not aligned
    biases.signals_mask = glonass_code_phase_biases::MASK_L1_CA | glonass_code_phase_biases::MASK_L2_CA;
    biases.l1_ca_bias = to_steps<int16_t>(-1.96, glonass_code_phase_biases::RESOLUTION_M); // DF423
    biases.l2_ca_bias = to_steps<int16_t>(-2.10, glonass_code_phase_biases::RESOLUTION_M); // DF425
    CASTLE_SAMPLE_CHECK(biases.l1_ca_bias == -98);
    CASTLE_SAMPLE_CHECK(castle::succeeded(uart1.send_rtcm(biases)));
    check_rtcm_frame(uart1, RTCM_TYPE_GLO_BIASES, STATION_ID);
    CASTLE_SAMPLE_CHECK(uart1.last_frame().size() == frame_size(4U + 4U)); // 32-bit header + 2 biases

    // A string longer than the 31 characters RTCM allows is refused when it is assigned.
    antenna_descriptor too_long{};
    CASTLE_SAMPLE_CHECK(too_long.descriptor.assign("0123456789012345678901234567890123") == castle::status::full);
    return 0;
}
