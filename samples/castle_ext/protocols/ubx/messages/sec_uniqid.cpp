#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/sec_uniqid.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Poll the chip ID: UBX-SEC-UNIQID poll requests have an empty payload.
    uint8_t poll[UBX_FRAME_OVERHEAD];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_poll<sec_uniqid>(poll, sizeof(poll), written)));
    CASTLE_SAMPLE_CHECK(written == UBX_FRAME_OVERHEAD);

    // The receiver answers with version 1 and a 5-byte unique ID.
    sec_uniqid answer{};
    answer.version = 0x01U;
    answer.unique_id = {{0xDEU, 0xADU, 0xBEU, 0xEFU, 0x42U}};

    uint8_t frame[UBX_FRAME_OVERHEAD + sec_uniqid::payload_length];
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(answer, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(sec_uniqid::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload_length() == 9U);

    sec_uniqid decoded{};
    CASTLE_SAMPLE_CHECK(sec_uniqid::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == 0x01U);
    CASTLE_SAMPLE_CHECK(decoded.unique_id[0U] == 0xDEU);
    CASTLE_SAMPLE_CHECK(decoded.unique_id[4U] == 0x42U);

    // Only the 9-byte layout of this spec revision is accepted.
    message_view longer = raw;
    longer.header.payload_length = 10U;
    CASTLE_SAMPLE_CHECK(!sec_uniqid::decode(longer, decoded));

    return 0;
}
