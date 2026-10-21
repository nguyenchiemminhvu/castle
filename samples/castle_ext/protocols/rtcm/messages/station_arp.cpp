#include "sample_support.hpp"

#include "castle_ext/protocols/rtcm/messages/station_arp.hpp"

namespace
{

/** @brief Describes the base station; shared by 1005 (`station_arp`) and 1006 (`station_arp_height`). */
template <typename Message>
Message make_station()
{
    Message station{};
    station.station_id = 1234U;
    station.itrf_year = 14U;
    station.gps = true;
    station.glonass = true;
    station.galileo = false;
    station.reference_station_indicator = false;
    station.single_receiver_oscillator = true;
    station.quarter_cycle_indicator = 2U;
    station.set_position_m(4027894.0060, 307045.6000, 4919474.9100);
    return station;
}

} // namespace

int main()
{
    using namespace castle::protocols::rtcm;
    using namespace castle::protocols::rtcm::messages;

    // 1. One type per message number: station_arp is 1005, station_arp_height is 1006.
    static_assert(station_arp::MESSAGE_TYPE == RTCM_TYPE_STATION_ARP, "station_arp is message 1005");
    static_assert(station_arp_height::MESSAGE_TYPE == RTCM_TYPE_STATION_ARP_HEIGHT, "station_arp_height is message 1006");
    static_assert(!station_arp::HAS_ANTENNA_HEIGHT && station_arp_height::HAS_ANTENNA_HEIGHT, "1006 adds DF028");
    CASTLE_SAMPLE_CHECK(station_arp::accepts(RTCM_TYPE_STATION_ARP));
    CASTLE_SAMPLE_CHECK(!station_arp::accepts(RTCM_TYPE_STATION_ARP_HEIGHT));
    CASTLE_SAMPLE_CHECK(station_arp_height::accepts(RTCM_TYPE_STATION_ARP_HEIGHT));
    CASTLE_SAMPLE_CHECK(!station_arp_height::accepts(RTCM_TYPE_ANTENNA_DESCRIPTOR));

    // 2. Describe the base station: positions are given in metres and stored as 0.0001 m units.
    station_arp station = make_station<station_arp>();
    CASTLE_SAMPLE_CHECK(station.ecef_x == 40278940060);
    CASTLE_SAMPLE_CHECK(station.ecef_y == 3070456000);
    CASTLE_SAMPLE_CHECK(station.ecef_z == 49194749100);
    CASTLE_SAMPLE_CHECK(station.x_m() > 4027894.0059 && station.x_m() < 4027894.0061);
    CASTLE_SAMPLE_CHECK(station.y_m() > 307045.5999 && station.y_m() < 307045.6001);
    CASTLE_SAMPLE_CHECK(station.z_m() > 4919474.9099 && station.z_m() < 4919474.9101);

    // 3. Message 1005: 19 byte payload, 25 byte frame (preamble 0xD3, 10-bit length, CRC-24Q).
    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    castle::status result = encode_message(station, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(19U));
    CASTLE_SAMPLE_CHECK(frame[0U] == RTCM_PREAMBLE);
    CASTLE_SAMPLE_CHECK(frame[2U] == 19U);
    CASTLE_SAMPLE_CHECK(frame[3U] == 0x3EU && frame[4U] == 0xD4U); // 1005 = 0x3ED, then station id 1234

    // 4. Message 1006 adds the antenna height above the marker (clamped to 0..6.5535 m).
    //    set_antenna_height_m() only compiles for station_arp_height.
    station_arp_height with_height = make_station<station_arp_height>();
    with_height.set_antenna_height_m(1.5);
    CASTLE_SAMPLE_CHECK(with_height.antenna_height == 15000U);
    CASTLE_SAMPLE_CHECK(with_height.antenna_height_m() > 1.4999 && with_height.antenna_height_m() < 1.5001);
    with_height.set_antenna_height_m(100.0);
    CASTLE_SAMPLE_CHECK(with_height.antenna_height == 65535U);
    with_height.set_antenna_height_m(-1.0);
    CASTLE_SAMPLE_CHECK(with_height.antenna_height == 0U);
    with_height.set_antenna_height_m(1.5);

    result = encode_message(with_height, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(21U));
    CASTLE_SAMPLE_CHECK(frame[3U] == 0x3EU && frame[4U] == 0xE4U); // 1006 = 0x3EE

    // 5. A buffer that cannot hold the frame reports full and writes nothing useful.
    result = encode_message(with_height, frame, frame_size(21U) - 1U, written);
    CASTLE_SAMPLE_CHECK(result == castle::status::full);
    CASTLE_SAMPLE_CHECK(written == 0U);

    // 6. Coordinates are 38-bit signed: a position beyond +-13743895.3472 m is reported at encode time.
    station.set_position_m(1.0e9, 0.0, 0.0);
    result = encode_message(station, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(written == 0U);

    return 0;
}
