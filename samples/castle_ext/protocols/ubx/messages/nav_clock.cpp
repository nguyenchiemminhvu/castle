#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_clock.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Receiver clock: -1.5 ms bias drifting at +120 ns/s.
    nav_clock value{};
    value.i_tow = 345600000U;
    value.clk_b = -1500000;
    value.clk_d = 120;
    value.t_acc = 15U;
    value.f_acc = 400U;

    // Encode a complete UBX frame (header + payload + checksum) in place.
    uint8_t frame[UBX_FRAME_OVERHEAD + nav_clock::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(value, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    // Validate the frame and identify it.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_clock::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload_length() == nav_clock::payload_length);

    // Decode the payload back into a typed model.
    nav_clock decoded{};
    CASTLE_SAMPLE_CHECK(nav_clock::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.clk_b == -1500000);
    CASTLE_SAMPLE_CHECK(decoded.clk_d == 120);
    CASTLE_SAMPLE_CHECK(decoded.t_acc == 15U);
    CASTLE_SAMPLE_CHECK(decoded.f_acc == 400U);

    // A different message is not NAV-CLOCK; a too-small buffer is reported as full.
    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_TIMEUTC;
    CASTLE_SAMPLE_CHECK(!nav_clock::matches(other));
    CASTLE_SAMPLE_CHECK(!nav_clock::decode(other, decoded));
    CASTLE_SAMPLE_CHECK(encode_message(value, frame, sizeof(frame) - 1U, written) == castle::status::full);

    return 0;
}
