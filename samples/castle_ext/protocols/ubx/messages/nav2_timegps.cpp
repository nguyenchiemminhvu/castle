#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav2_timegps.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // UBX-NAV2-TIMEGPS payload (16 bytes).
    //
    // iTOW  = 1000
    // fTOW  = -500
    // week  = 2300
    // leapS = 18
    // valid = 0x03
    // tAcc  = 25
    uint8_t payload_data[nav2_timegps::payload_length] = {};

    castle::write_le32(&payload_data[0U], 1000U);                 // iTOW
    castle::write_le32(&payload_data[4U], 0xFFFFFE0CU);          // fTOW = -500
    castle::write_le16(&payload_data[8U], 2300U);                // week
    payload_data[10U] = 18U;                             // leapS
    payload_data[11U] = 0x03U;                           // valid
    castle::write_le32(&payload_data[12U], 25U);                 // tAcc

    message_view raw;
    raw.header.msg_class = UBX_CLASS_NAV2;
    raw.header.msg_id = UBX_ID_NAV2_TIMEGPS;
    raw.header.payload_length = nav2_timegps::payload_length;
    raw.payload =
        castle::container::array_view<const uint8_t>(
            payload_data,
            sizeof(payload_data));

    CASTLE_SAMPLE_CHECK(nav2_timegps::matches(raw));

    nav2_timegps message;
    bool decoded = nav2_timegps::decode(raw, message);

    CASTLE_SAMPLE_CHECK(decoded);

    CASTLE_SAMPLE_CHECK(message.i_tow == 1000U);
    CASTLE_SAMPLE_CHECK(message.f_tow == -500);
    CASTLE_SAMPLE_CHECK(message.week == 2300);
    CASTLE_SAMPLE_CHECK(message.leap_s == 18);
    CASTLE_SAMPLE_CHECK(message.valid == 0x03U);
    CASTLE_SAMPLE_CHECK(message.t_acc == 25U);

    // Wrong payload length must be rejected.
    message_view invalid = raw;
    invalid.header.payload_length = nav2_timegps::payload_length - 1U;

    nav2_timegps rejected;
    CASTLE_SAMPLE_CHECK(!nav2_timegps::decode(invalid, rejected));

    return 0;
}
