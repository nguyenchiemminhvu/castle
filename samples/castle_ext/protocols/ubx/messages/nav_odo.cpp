#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_odo.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    nav_odo odometer;
    odometer.version = 0U;
    odometer.i_tow = 123456U;
    odometer.distance = 1500U;          // m since last reset
    odometer.total_distance = 987654U;  // m cumulative
    odometer.distance_std = 12U;        // m, 1-sigma

    uint8_t frame[frame_size(nav_odo::payload_length)];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(odometer, frame, sizeof(frame), written) == castle::status::ok);

    // version is the first payload byte, iTOW starts after 3 reserved bytes.
    CASTLE_SAMPLE_CHECK(frame[UBX_FRAME_INDEX_PAYLOAD] == 0U);
    CASTLE_SAMPLE_CHECK(castle::read_le32(&frame[UBX_FRAME_INDEX_PAYLOAD + 4U]) == 123456U);

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_odo::matches(raw));

    nav_odo decoded;
    CASTLE_SAMPLE_CHECK(nav_odo::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == 0U);
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(decoded.distance == 1500U);
    CASTLE_SAMPLE_CHECK(decoded.total_distance == 987654U);
    CASTLE_SAMPLE_CHECK(decoded.distance_std == 12U);

    // A buffer too small for the frame reports castle::status::full.
    CASTLE_SAMPLE_CHECK(encode_message(odometer, frame, sizeof(frame) - 1U, written) == castle::status::full);

    // A different message does not match NAV-ODO.
    raw.header.msg_id = UBX_ID_NAV_PVT;
    CASTLE_SAMPLE_CHECK(!nav_odo::matches(raw));

    return 0;
}
