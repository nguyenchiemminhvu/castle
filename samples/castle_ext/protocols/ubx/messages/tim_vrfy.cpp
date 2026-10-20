#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/tim_vrfy.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // RTC-sourced time was 3 ms 250 ns behind the current receiver time.
    tim_vrfy verify{};
    verify.itow = 345600123;
    verify.frac = 456789;
    verify.delta_ms = 3;
    verify.delta_ns = 250;
    verify.wno = 2300U;
    verify.flags = static_cast<uint8_t>(tim_vrfy_src::rtc);

    uint8_t frame[UBX_FRAME_OVERHEAD + tim_vrfy::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(verify, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(tim_vrfy::matches(raw));

    tim_vrfy decoded{};
    CASTLE_SAMPLE_CHECK(tim_vrfy::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.itow == 345600123);
    CASTLE_SAMPLE_CHECK(decoded.frac == 456789);
    CASTLE_SAMPLE_CHECK(decoded.delta_ms == 3);
    CASTLE_SAMPLE_CHECK(decoded.delta_ns == 250);
    CASTLE_SAMPLE_CHECK(decoded.wno == 2300U);
    CASTLE_SAMPLE_CHECK(decoded.src() == tim_vrfy_src::rtc);

    // Assistance-data source and "no aiding".
    decoded.flags = static_cast<uint8_t>(tim_vrfy_src::assistance);
    CASTLE_SAMPLE_CHECK(decoded.src() == tim_vrfy_src::assistance);
    decoded.flags = static_cast<uint8_t>(tim_vrfy_src::no_aiding);
    CASTLE_SAMPLE_CHECK(decoded.src() == tim_vrfy_src::no_aiding);

    // TIM-TP is not TIM-VRFY.
    message_view other = raw;
    other.header.msg_id = UBX_ID_TIM_TP;
    CASTLE_SAMPLE_CHECK(!tim_vrfy::matches(other));

    return 0;
}
