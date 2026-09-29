#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_att.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    // Build a NAV-ATT payload.
    uint8_t payload[nav_att::payload_length] = {};

    write_le32(&payload[0U], 123456U);               // iTOW
    payload[4U] = 1U;                               // version

    write_le32(&payload[8U],  static_cast<uint32_t>(1000));   // roll
    write_le32(&payload[12U], static_cast<uint32_t>(-2000));  // pitch
    write_le32(&payload[16U], static_cast<uint32_t>(3000));   // heading

    write_le32(&payload[20U], 100U);  // accRoll
    write_le32(&payload[24U], 200U);  // accPitch
    write_le32(&payload[28U], 300U);  // accHeading

    // Encode a UBX-NAV-ATT frame.
    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        nav_att::msg_class,
        nav_att::msg_id,
        castle::container::array_view<const uint8_t>(
            payload,
            nav_att::payload_length),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // Decode the UBX frame.
    message_view raw;
    bool frame_ok = decode_frame(
        castle::container::array_view<const uint8_t>(
            frame,
            written),
        raw);

    CASTLE_SAMPLE_CHECK(frame_ok);

    // Verify message identification helpers.
    CASTLE_SAMPLE_CHECK(nav_att::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_NAV, UBX_ID_NAV_ATT));

    // Decode the NAV-ATT payload.
    nav_att attitude;
    bool decoded = nav_att::decode(raw, attitude);

    CASTLE_SAMPLE_CHECK(decoded);

    // Verify decoded values.
    CASTLE_SAMPLE_CHECK(attitude.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(attitude.version == 1U);

    CASTLE_SAMPLE_CHECK(attitude.roll == 1000);
    CASTLE_SAMPLE_CHECK(attitude.pitch == -2000);
    CASTLE_SAMPLE_CHECK(attitude.heading == 3000);

    CASTLE_SAMPLE_CHECK(attitude.acc_roll == 100U);
    CASTLE_SAMPLE_CHECK(attitude.acc_pitch == 200U);
    CASTLE_SAMPLE_CHECK(attitude.acc_heading == 300U);

    // A different message type should not match NAV-ATT.
    message_view different{};
    different.header.msg_class = UBX_CLASS_CFG;
    different.header.msg_id = UBX_ID_CFG_VALSET;

    CASTLE_SAMPLE_CHECK(!nav_att::matches(different));

    return 0;
}
