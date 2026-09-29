#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_clock.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a NAV-CLOCK payload.
    uint8_t payload[nav_clock::payload_length] = {};

    castle::write_le32(&payload[0U], 123456U);                        // iTOW
    castle::write_le32(&payload[4U], static_cast<uint32_t>(-500000)); // clkB
    castle::write_le32(&payload[8U], static_cast<uint32_t>(250));     // clkD
    castle::write_le32(&payload[12U], 50U);                           // tAcc
    castle::write_le32(&payload[16U], 10U);                           // fAcc

    // Encode a UBX-NAV-CLOCK frame.
    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        nav_clock::msg_class,
        nav_clock::msg_id,
        castle::container::array_view<const uint8_t>(
            payload,
            nav_clock::payload_length),
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
    CASTLE_SAMPLE_CHECK(nav_clock::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_NAV, UBX_ID_NAV_CLOCK));

    // Decode the NAV-CLOCK payload.
    nav_clock clock;
    bool decoded = nav_clock::decode(raw, clock);

    CASTLE_SAMPLE_CHECK(decoded);

    // Verify decoded values.
    CASTLE_SAMPLE_CHECK(clock.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(clock.clk_b == -500000);
    CASTLE_SAMPLE_CHECK(clock.clk_d == 250);
    CASTLE_SAMPLE_CHECK(clock.t_acc == 50U);
    CASTLE_SAMPLE_CHECK(clock.f_acc == 10U);

    // Different message types do not match NAV-CLOCK.
    message_view different{};
    different.header.msg_class = UBX_CLASS_CFG;
    different.header.msg_id = UBX_ID_CFG_VALSET;

    CASTLE_SAMPLE_CHECK(!nav_clock::matches(different));

    return 0;
}
