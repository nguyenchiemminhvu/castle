#include "sample_support.hpp"

#include "castle_ext/protocols/rtcm/rtcm.hpp"

int main()
{
    using namespace castle::protocols::rtcm;
    using castle::container::array_view;

    // 1. Frame layout constants and sizes.
    CASTLE_SAMPLE_CHECK(RTCM_PREAMBLE == 0xD3U);
    CASTLE_SAMPLE_CHECK(RTCM_FRAME_INDEX_PAYLOAD == 3U);
    CASTLE_SAMPLE_CHECK(RTCM_FRAME_OVERHEAD == 6U);
    CASTLE_SAMPLE_CHECK(RTCM_MAX_PAYLOAD_LEN == 1023U);
    CASTLE_SAMPLE_CHECK(RTCM_MAX_FRAME_SIZE == 1029U);
    CASTLE_SAMPLE_CHECK(RTCM_TYPE_STATION_ARP == 1005U);
    CASTLE_SAMPLE_CHECK(frame_size(19U) == 25U);

    // 2. encode_frame(): frame an already encoded payload (message 1005, 19 bytes).
    const uint8_t payload_bytes[19U] = {0x3EU, 0xD4U, 0xD2U, 0x3BU, 0x09U, 0x60U, 0xCFU, 0xD9U, 0x9CU, 0x80U,
                                        0xB7U, 0x03U, 0x70U, 0xC0U, 0x8BU, 0x74U, 0x3CU, 0x4CU, 0xACU};
    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    castle::status result = encode_frame(array_view<const uint8_t>(payload_bytes, sizeof(payload_bytes)),
                                         frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    CASTLE_SAMPLE_CHECK(written == frame_size(sizeof(payload_bytes)));
    CASTLE_SAMPLE_CHECK(frame[0U] == RTCM_PREAMBLE);
    CASTLE_SAMPLE_CHECK(frame[1U] == 0x00U); // 6 reserved bits + 2 length MSBs
    CASTLE_SAMPLE_CHECK(frame[2U] == 0x13U); // 19 payload bytes
    CASTLE_SAMPLE_CHECK(frame[3U] == 0x3EU);
    CASTLE_SAMPLE_CHECK(frame[written - 3U] == 0x19U); // CRC-24Q, most significant byte first
    CASTLE_SAMPLE_CHECK(frame[written - 2U] == 0xD1U);
    CASTLE_SAMPLE_CHECK(frame[written - 1U] == 0xF1U);

    // The CRC covers preamble, length and payload; the whole frame is a CRC-24Q codeword.
    const uint32_t crc = castle::checksums::crc24q::calculate(frame, written - RTCM_CRC_SIZE);
    CASTLE_SAMPLE_CHECK(crc == 0x19D1F1U);
    CASTLE_SAMPLE_CHECK(castle::checksums::crc24q::calculate(frame, written) == 0U);

    // 3. detail::finish_frame(): the shared tail of encode_frame() and encode_message().
    uint8_t in_place[RTCM_FRAME_OVERHEAD + 19U] = {};
    for (castle::size_type index = 0U; index < sizeof(payload_bytes); ++index)
    {
        in_place[RTCM_FRAME_INDEX_PAYLOAD + index] = payload_bytes[index];
    }
    const castle::size_type total = detail::finish_frame(in_place, sizeof(payload_bytes));
    CASTLE_SAMPLE_CHECK(total == written);
    CASTLE_SAMPLE_CHECK(in_place[total - 1U] == 0xF1U);

    // 4. An empty payload is a valid 6-byte frame.
    uint8_t empty_frame[RTCM_FRAME_OVERHEAD] = {};
    result = encode_frame(array_view<const uint8_t>(), empty_frame, sizeof(empty_frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    CASTLE_SAMPLE_CHECK(written == RTCM_FRAME_OVERHEAD);

    // 5. Error reporting: written is zero on every failure.
    result = encode_frame(array_view<const uint8_t>(payload_bytes, sizeof(payload_bytes)), nullptr, 64U, written);
    CASTLE_SAMPLE_CHECK(result == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(written == 0U);

    result = encode_frame(array_view<const uint8_t>(nullptr, 4U), frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::invalid_argument);

    static uint8_t too_long[RTCM_MAX_PAYLOAD_LEN + 1U] = {};
    static uint8_t large_frame[RTCM_MAX_FRAME_SIZE + 8U] = {};
    result = encode_frame(array_view<const uint8_t>(too_long, sizeof(too_long)), large_frame, sizeof(large_frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(written == 0U);

    result = encode_frame(array_view<const uint8_t>(payload_bytes, sizeof(payload_bytes)),
                          frame, frame_size(sizeof(payload_bytes)) - 1U, written);
    CASTLE_SAMPLE_CHECK(result == castle::status::full);
    CASTLE_SAMPLE_CHECK(written == 0U);

    // 6. The largest payload fills the largest frame; the length field spans two bytes.
    result = encode_frame(array_view<const uint8_t>(too_long, RTCM_MAX_PAYLOAD_LEN), large_frame, sizeof(large_frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    CASTLE_SAMPLE_CHECK(written == RTCM_MAX_FRAME_SIZE);
    CASTLE_SAMPLE_CHECK(large_frame[1U] == 0x03U);
    CASTLE_SAMPLE_CHECK(large_frame[2U] == 0xFFU);

    return 0;
}
