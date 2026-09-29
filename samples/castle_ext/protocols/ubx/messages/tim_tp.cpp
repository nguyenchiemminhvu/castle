#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/tim_tp.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a UBX-TIM-TP payload.
    uint8_t payload[tim_tp::payload_length] = {};

    castle::write_le32(&payload[0U], 123456U);                        // tow_ms
    castle::write_le32(&payload[4U], 789U);                           // tow_sub_ms
    castle::write_le32(&payload[8U], static_cast<uint32_t>(-250));    // q_err

    castle::write_le16(&payload[12U], 2045U); // week

    payload[14U] = 0xA5U; // flags
    payload[15U] = 0x03U; // ref_info

    // Encode payload into a UBX frame.
    uint8_t frame[64U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        tim_tp::msg_class,
        tim_tp::msg_id,
        castle::container::array_view<const uint8_t>(payload, sizeof(payload)),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // Decode frame into a generic UBX message view.
    message_view raw;
    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);

    // Verify message identity.
    CASTLE_SAMPLE_CHECK(tim_tp::matches(raw));

    // Decode TIM-TP payload.
    tim_tp tp;
    CASTLE_SAMPLE_CHECK(tim_tp::decode(raw, tp));

    // Verify decoded fields.
    CASTLE_SAMPLE_CHECK(tp.tow_ms == 123456U);
    CASTLE_SAMPLE_CHECK(tp.tow_sub_ms == 789U);
    CASTLE_SAMPLE_CHECK(tp.q_err == -250);
    CASTLE_SAMPLE_CHECK(tp.week == 2045U);
    CASTLE_SAMPLE_CHECK(tp.flags == 0xA5U);
    CASTLE_SAMPLE_CHECK(tp.ref_info == 0x03U);

    // Different message type must not match TIM-TP.
    CASTLE_SAMPLE_CHECK(!raw.is(UBX_CLASS_SEC, UBX_ID_SEC_SIG));

    return 0;
}
