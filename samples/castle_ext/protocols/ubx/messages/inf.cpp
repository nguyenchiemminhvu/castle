#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/inf.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using castle_ext::protocols::ubx::messages::inf;
    using castle_ext::protocols::ubx::messages::inf_subtype;

    // Build a UBX-INF-NOTICE message carrying a nul-terminated text payload.
    const uint8_t text_payload[] =
    {
        'G', 'N', 'S', 'S', ' ',
        'r', 'e', 'a', 'd', 'y',
        0U
    };

    castle::container::array_view<const uint8_t> payload(
        text_payload,
        sizeof(text_payload));

    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status encode_status = encode_frame(
        UBX_CLASS_INF,
        UBX_ID_INF_NOTICE,
        payload,
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    // Decode raw UBX frame.
    message_view raw;
    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);

    // Verify message identification.
    CASTLE_SAMPLE_CHECK(inf<>::matches(raw));

    // Decode into typed UBX-INF structure.
    inf<> message;
    CASTLE_SAMPLE_CHECK(inf<>::decode(raw, message));

    CASTLE_SAMPLE_CHECK(message.msg_class == UBX_CLASS_INF);
    CASTLE_SAMPLE_CHECK(message.message_id == UBX_ID_INF_NOTICE);
    CASTLE_SAMPLE_CHECK(message.subtype == inf_subtype::notice);

    // Text is copied until the first nul terminator.
    castle::container::string_view text = message.text();

    CASTLE_SAMPLE_CHECK(text.size() == 10U);
    CASTLE_SAMPLE_CHECK(text[0U] == 'G');
    CASTLE_SAMPLE_CHECK(text[1U] == 'N');
    CASTLE_SAMPLE_CHECK(text[2U] == 'S');
    CASTLE_SAMPLE_CHECK(text[3U] == 'S');

    // Different INF severities are distinguished by message ID.
    CASTLE_SAMPLE_CHECK(
        static_cast<uint8_t>(message.subtype) == UBX_ID_INF_NOTICE);

    // Non-INF messages must not match or decode.
    uint8_t cfg_payload[1U] = {0U};

    castle::size_type cfg_written = 0U;
    encode_status = encode_frame(
        UBX_CLASS_CFG,
        UBX_ID_CFG_VALGET,
        castle::container::array_view<const uint8_t>(cfg_payload, 1U),
        frame,
        sizeof(frame),
        cfg_written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    message_view non_inf_raw;
    CASTLE_SAMPLE_CHECK(
        decode_frame(
            castle::container::array_view<const uint8_t>(frame, cfg_written),
            non_inf_raw));

    CASTLE_SAMPLE_CHECK(!inf<>::matches(non_inf_raw));

    inf<> rejected;
    CASTLE_SAMPLE_CHECK(!inf<>::decode(non_inf_raw, rejected));

    return 0;
}
