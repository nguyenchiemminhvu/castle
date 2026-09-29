#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_timeutc.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    // Build a valid UBX-NAV-TIMEUTC payload.
    uint8_t payload[nav_timeutc::payload_length] = {};

    write_le32(&payload[0U], 456000U);                          // iTOW
    write_le32(&payload[4U], 100U);                             // tAcc
    write_le32(&payload[8U], static_cast<uint32_t>(-500000));  // nano
    write_le16(&payload[12U], 2026U);                           // year
    payload[14U] = 10U;                                         // month
    payload[15U] = 2U;                                          // day
    payload[16U] = 14U;                                         // hour
    payload[17U] = 30U;                                         // min
    payload[18U] = 45U;                                         // sec
    payload[19U] = 0x07U;                                       // valid

    // Encode the payload into a UBX frame.
    uint8_t frame[64U];
    castle::size_type written = 0U;

    castle::status encode_status = encode_frame(
        nav_timeutc::msg_class,
        nav_timeutc::msg_id,
        castle::container::array_view<const uint8_t>(
            payload,
            nav_timeutc::payload_length),
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
    CASTLE_SAMPLE_CHECK(nav_timeutc::matches(raw));

    // Decode the NAV-TIMEUTC message fields.
    nav_timeutc timeutc;
    CASTLE_SAMPLE_CHECK(nav_timeutc::decode(raw, timeutc));

    CASTLE_SAMPLE_CHECK(timeutc.i_tow == 456000U);
    CASTLE_SAMPLE_CHECK(timeutc.t_acc == 100U);
    CASTLE_SAMPLE_CHECK(timeutc.nano == -500000);
    CASTLE_SAMPLE_CHECK(timeutc.year == 2026U);
    CASTLE_SAMPLE_CHECK(timeutc.month == 10U);
    CASTLE_SAMPLE_CHECK(timeutc.day == 2U);
    CASTLE_SAMPLE_CHECK(timeutc.hour == 14U);
    CASTLE_SAMPLE_CHECK(timeutc.min == 30U);
    CASTLE_SAMPLE_CHECK(timeutc.sec == 45U);
    CASTLE_SAMPLE_CHECK(timeutc.valid == 0x07U);

    // NAV-TIMEUTC decoding requires the exact payload size.
    message_view invalid = raw;
    invalid.header.payload_length = 0U;

    nav_timeutc rejected;
    CASTLE_SAMPLE_CHECK(!nav_timeutc::decode(invalid, rejected));

    return 0;
}
