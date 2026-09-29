#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/sec_crc.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a UBX-SEC-CRC payload.
    uint8_t payload[sec_crc::payload_length] = {};

    payload[0U] = 1U; // version
    payload[1U] = 2U; // crc_type

    castle::write_le16(&payload[2U], 0x1234U);       // reserved
    castle::write_le32(&payload[4U], 0x89ABCDEFU);   // crc32

    // Encode payload into a UBX frame.
    uint8_t frame[64U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        sec_crc::msg_class,
        sec_crc::msg_id,
        castle::container::array_view<const uint8_t>(payload, sizeof(payload)),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // Decode the frame into a generic UBX message view.
    message_view raw;
    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);

    // Verify message identity.
    CASTLE_SAMPLE_CHECK(sec_crc::matches(raw));

    // Decode the SEC-CRC payload.
    sec_crc crc;
    CASTLE_SAMPLE_CHECK(sec_crc::decode(raw, crc));

    // Verify decoded fields.
    CASTLE_SAMPLE_CHECK(crc.version == 1U);
    CASTLE_SAMPLE_CHECK(crc.crc_type == 2U);
    CASTLE_SAMPLE_CHECK(crc.reserved == 0x1234U);
    CASTLE_SAMPLE_CHECK(crc.crc32 == 0x89ABCDEFU);

    // Different class/ID should not match SEC-CRC.
    CASTLE_SAMPLE_CHECK(!raw.is(UBX_CLASS_CFG, UBX_ID_CFG_VALSET));

    return 0;
}
