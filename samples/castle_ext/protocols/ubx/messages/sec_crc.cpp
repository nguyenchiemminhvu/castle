#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/sec_crc.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // A CRC report (generic u-blox layout; not part of the ZED-F9R spec).
    sec_crc report{};
    report.version = 0x01U;
    report.crc_type = 0x02U;
    report.reserved = 0U;
    report.crc32 = 0xCAFEBABEU;

    uint8_t frame[UBX_FRAME_OVERHEAD + sec_crc::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(report, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(sec_crc::matches(raw));

    sec_crc decoded{};
    CASTLE_SAMPLE_CHECK(sec_crc::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == 0x01U);
    CASTLE_SAMPLE_CHECK(decoded.crc_type == 0x02U);
    CASTLE_SAMPLE_CHECK(decoded.crc32 == 0xCAFEBABEU);

    // Firmware extensions after the 8-byte prefix are ignored.
    uint8_t extended[sec_crc::payload_length + 4U] = {};
    for (castle::size_type index = 0U; index < sec_crc::payload_length; ++index)
    {
        extended[index] = raw.payload[index];
    }
    extended[sec_crc::payload_length] = 0xFFU;
    message_view longer{{UBX_CLASS_SEC, UBX_ID_SEC_CRC, static_cast<uint16_t>(sizeof(extended))},
                        castle::container::array_view<const uint8_t>(extended, sizeof(extended)),
                        checksum{}};
    CASTLE_SAMPLE_CHECK(sec_crc::decode(longer, decoded));
    CASTLE_SAMPLE_CHECK(decoded.crc32 == 0xCAFEBABEU);

    // Truncated payloads are rejected.
    longer.header.payload_length = sec_crc::payload_length - 1U;
    CASTLE_SAMPLE_CHECK(!sec_crc::decode(longer, decoded));

    return 0;
}
