#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_timegps.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a valid UBX-NAV-TIMEGPS payload.
    uint8_t payload[nav_timegps::payload_length] = {};

    castle::write_le32(&payload[0U], 345000U);                        // iTOW
    castle::write_le32(&payload[4U], static_cast<uint32_t>(-250));    // fTOW
    castle::write_le16(&payload[8U], static_cast<uint16_t>(2300));    // week
    payload[10U] = static_cast<uint8_t>(18);                  // leapS
    payload[11U] = 0x03U;                                     // valid
    castle::write_le32(&payload[12U], 50U);                           // tAcc

    // Encode the payload into a UBX frame.
    uint8_t frame[64U];
    castle::size_type written = 0U;

    castle::status encode_status = encode_frame(
        nav_timegps::msg_class,
        nav_timegps::msg_id,
        castle::container::array_view<const uint8_t>(
            payload,
            nav_timegps::payload_length),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    // Decode the raw UBX frame.
    message_view raw;
    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);
    CASTLE_SAMPLE_CHECK(nav_timegps::matches(raw));

    // Decode the NAV-TIMEGPS message fields.
    nav_timegps timegps;
    CASTLE_SAMPLE_CHECK(nav_timegps::decode(raw, timegps));

    CASTLE_SAMPLE_CHECK(timegps.i_tow == 345000U);
    CASTLE_SAMPLE_CHECK(timegps.f_tow == -250);
    CASTLE_SAMPLE_CHECK(timegps.week == 2300);
    CASTLE_SAMPLE_CHECK(timegps.leap_s == 18);
    CASTLE_SAMPLE_CHECK(timegps.valid == 0x03U);
    CASTLE_SAMPLE_CHECK(timegps.t_acc == 50U);

    // NAV-TIMEGPS decoding requires the exact payload size.
    message_view invalid = raw;
    invalid.header.payload_length = 0U;

    nav_timegps rejected;
    CASTLE_SAMPLE_CHECK(!nav_timegps::decode(invalid, rejected));

    return 0;
}
