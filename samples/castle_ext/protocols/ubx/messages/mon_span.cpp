#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_span.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a synthetic UBX-MON-SPAN payload containing one RF block.
    static constexpr castle::size_type payload_size =
        mon_span<>::header_length + mon_span<>::block_length;

    uint8_t payload_data[payload_size] = {};

    // Header
    payload_data[0U] = 0x01U; // version
    payload_data[1U] = 0x01U; // declared RF block count
    payload_data[2U] = 0x00U;
    payload_data[3U] = 0x00U;

    castle::size_type offset = mon_span<>::header_length;

    // Spectrum bins
    for (castle::size_type i = 0U; i < MON_SPAN_SPECTRUM_BINS; ++i)
    {
        payload_data[offset + i] = static_cast<uint8_t>(i);
    }
    offset += MON_SPAN_SPECTRUM_BINS;

    // RF block metadata
    castle::write_le32(&payload_data[offset], 1000000UL); // span
    offset += 4U;

    castle::write_le32(&payload_data[offset], 1000UL); // resolution
    offset += 4U;

    castle::write_le32(&payload_data[offset], 1575420000UL); // center frequency
    offset += 4U;

    payload_data[offset] = 12U; // PGA
    offset += 1U;

    // Reserved bytes
    payload_data[offset++] = 0U;
    payload_data[offset++] = 0U;
    payload_data[offset++] = 0U;

    CASTLE_SAMPLE_CHECK(offset == payload_size);

    // Encode the payload into a UBX frame.
    castle::container::array_view<const uint8_t> payload(
        payload_data,
        payload_size);

    uint8_t frame[512U];
    castle::size_type bytes_written = 0U;

    castle::status encode_status = encode_frame(
        UBX_CLASS_MON,
        UBX_ID_MON_SPAN,
        payload,
        frame,
        sizeof(frame),
        bytes_written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    // Decode the UBX frame.
    message_view raw;
    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(
            frame,
            bytes_written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);
    CASTLE_SAMPLE_CHECK(mon_span<>::matches(raw));

    // Decode the MON-SPAN message payload.
    mon_span<> span;
    CASTLE_SAMPLE_CHECK(mon_span<>::decode(raw, span));

    CASTLE_SAMPLE_CHECK(span.version == 0x01U);
    CASTLE_SAMPLE_CHECK(span.num_rf_blocks == 1U);

    // Verify spectrum bins.
    CASTLE_SAMPLE_CHECK(span.rf_blocks[0U].spectrum[0U] == 0U);
    CASTLE_SAMPLE_CHECK(span.rf_blocks[0U].spectrum[1U] == 1U);
    CASTLE_SAMPLE_CHECK(span.rf_blocks[0U].spectrum[255U] == 255U);

    // Verify RF block metadata.
    CASTLE_SAMPLE_CHECK(span.rf_blocks[0U].span == 1000000UL);
    CASTLE_SAMPLE_CHECK(span.rf_blocks[0U].res == 1000UL);
    CASTLE_SAMPLE_CHECK(span.rf_blocks[0U].center == 1575420000UL);
    CASTLE_SAMPLE_CHECK(span.rf_blocks[0U].pga == 12U);

    return 0;
}
