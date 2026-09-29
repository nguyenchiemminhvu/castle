#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_status.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a valid UBX-NAV-STATUS payload.
    uint8_t payload[nav_status::payload_length] = {};

    castle::write_le32(&payload[0U], 123456U); // iTOW
    payload[4U] = static_cast<uint8_t>(nav_status_gps_fix::fix_3d);
    payload[5U] = nav_status::FLAGS_GPS_FIX_OK;
    payload[6U] = 0x00U; // fixStat
    payload[7U] = 0x00U; // flags2
    castle::write_le32(&payload[8U], 4500U);   // ttff
    castle::write_le32(&payload[12U], 9000U);  // msss

    // Encode the payload into a UBX frame.
    uint8_t frame[64U];
    castle::size_type written = 0U;

    castle::status encode_status = encode_frame(
        nav_status::msg_class,
        nav_status::msg_id,
        castle::container::array_view<const uint8_t>(
            payload,
            nav_status::payload_length),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    // Decode the raw frame.
    message_view raw;
    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);
    CASTLE_SAMPLE_CHECK(nav_status::matches(raw));

    // Decode the NAV-STATUS message fields.
    nav_status status;
    CASTLE_SAMPLE_CHECK(nav_status::decode(raw, status));

    CASTLE_SAMPLE_CHECK(status.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(status.gps_fix == nav_status_gps_fix::fix_3d);
    CASTLE_SAMPLE_CHECK(status.fix_ok());
    CASTLE_SAMPLE_CHECK(status.ttff == 4500U);
    CASTLE_SAMPLE_CHECK(status.msss == 9000U);

    // Decode fails if the payload length does not match NAV-STATUS.
    message_view invalid = raw;
    invalid.header.payload_length = 1U;

    nav_status rejected;
    CASTLE_SAMPLE_CHECK(!nav_status::decode(invalid, rejected));

    return 0;
}
