#include "sample_support.hpp"

#include "castle_ext/protocols/rtcm/messages/receiver_antenna_descriptor.hpp"

int main()
{
    using namespace castle::protocols::rtcm;
    using namespace castle::protocols::rtcm::messages;

    CASTLE_SAMPLE_CHECK(receiver_antenna_descriptor::accepts(RTCM_TYPE_RECEIVER_ANTENNA));
    CASTLE_SAMPLE_CHECK(!receiver_antenna_descriptor::accepts(RTCM_TYPE_ANTENNA_DESCRIPTOR));

    // Antenna and receiver identification of the reference station.
    receiver_antenna_descriptor info{};
    info.station_id = 1234U;
    CASTLE_SAMPLE_CHECK(info.antenna.assign("ANT") == castle::status::ok);
    info.antenna_setup_id = 2U;
    CASTLE_SAMPLE_CHECK(info.antenna_serial.assign("SN123") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(info.receiver_type.assign("UBLOX ZED-F9P") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(info.firmware.assign("HPG 1.32") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(info.receiver_serial.assign("0042") == castle::status::ok);

    uint8_t frame[256U] = {};
    castle::size_type written = 0U;
    castle::status result = encode_message(info, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(42U));
    CASTLE_SAMPLE_CHECK(frame[0U] == RTCM_PREAMBLE);
    CASTLE_SAMPLE_CHECK(frame[2U] == 42U);
    CASTLE_SAMPLE_CHECK(frame[3U] == 0x40U && frame[4U] == 0x94U); // 1033 = 0x409, station id 1234
    CASTLE_SAMPLE_CHECK(frame[6U] == 3U && frame[7U] == 'A');      // antenna string

    // An exact-fit buffer works, one byte less is full.
    result = encode_message(info, frame, frame_size(42U), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::ok);
    result = encode_message(info, frame, frame_size(42U) - 1U, written);
    CASTLE_SAMPLE_CHECK(result == castle::status::full);
    CASTLE_SAMPLE_CHECK(written == 0U);

    // Empty strings are written as a zero count; every string is limited to 31 characters.
    receiver_antenna_descriptor empty{};
    result = encode_message(empty, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(9U));
    CASTLE_SAMPLE_CHECK(info.firmware.assign("ABCDEFGHIJKLMNOPQRSTUVWXYZ012345") == castle::status::full);

    // The 12-bit station ID is checked while encoding.
    info.station_id = 4096U;
    result = encode_message(info, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(written == 0U);

    return 0;
}
