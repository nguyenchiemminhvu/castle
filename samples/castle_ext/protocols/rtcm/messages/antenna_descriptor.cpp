#include "sample_support.hpp"

#include "castle_ext/protocols/rtcm/messages/antenna_descriptor.hpp"

int main()
{
    using namespace castle::protocols::rtcm;
    using namespace castle::protocols::rtcm::messages;

    CASTLE_SAMPLE_CHECK(antenna_descriptor::accepts(RTCM_TYPE_ANTENNA_DESCRIPTOR));
    CASTLE_SAMPLE_CHECK(!antenna_descriptor::accepts(RTCM_TYPE_RECEIVER_ANTENNA));
    CASTLE_SAMPLE_CHECK(RTCM_MAX_DESCRIPTOR_LEN == 31U);

    // Antenna type as listed by IGS/NGS plus the antenna setup ID.
    antenna_descriptor antenna{};
    antenna.station_id = 1234U;
    CASTLE_SAMPLE_CHECK(antenna.descriptor.assign("TRM59800.00     NONE") == castle::status::ok);
    antenna.setup_id = 3U;

    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    castle::status result = encode_message(antenna, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(25U)); // 3 header bytes + count + 20 characters + setup id
    CASTLE_SAMPLE_CHECK(frame[0U] == RTCM_PREAMBLE);
    CASTLE_SAMPLE_CHECK(frame[3U] == 0x3EU && frame[4U] == 0xF4U); // 1007 = 0x3EF, station id 1234
    CASTLE_SAMPLE_CHECK(frame[6U] == 20U);                          // character count
    CASTLE_SAMPLE_CHECK(frame[7U] == 'T');
    CASTLE_SAMPLE_CHECK(frame[27U] == 3U);                          // setup id

    // A descriptor holds at most 31 characters; longer text is refused by the string itself.
    CASTLE_SAMPLE_CHECK(antenna.descriptor.assign("ABCDEFGHIJKLMNOPQRSTUVWXYZ012345") == castle::status::full);
    CASTLE_SAMPLE_CHECK(antenna.descriptor.assign("ABCDEFGHIJKLMNOPQRSTUVWXYZ01234") == castle::status::ok);

    // A station ID wider than 12 bits is reported at encode time.
    antenna.station_id = 4096U;
    result = encode_message(antenna, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(written == 0U);
    antenna.station_id = 4095U;

    // The exact frame size is enough; one byte less is full.
    result = encode_message(antenna, frame, frame_size(36U), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(36U));
    result = encode_message(antenna, frame, frame_size(36U) - 1U, written);
    CASTLE_SAMPLE_CHECK(result == castle::status::full);
    CASTLE_SAMPLE_CHECK(written == 0U);

    return 0;
}
