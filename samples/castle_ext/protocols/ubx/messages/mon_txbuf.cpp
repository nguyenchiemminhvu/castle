#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_txbuf.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Construct a UBX-MON-TXBUF payload.
    uint8_t payload_data[mon_txbuf::payload_length] = {};
    castle::size_type offset = 0U;

    // pending[6]
    castle::write_le16(&payload_data[offset], 10U);
    offset += 2U;
    castle::write_le16(&payload_data[offset], 20U);
    offset += 2U;
    castle::write_le16(&payload_data[offset], 30U);
    offset += 2U;
    castle::write_le16(&payload_data[offset], 40U);
    offset += 2U;
    castle::write_le16(&payload_data[offset], 50U);
    offset += 2U;
    castle::write_le16(&payload_data[offset], 60U);
    offset += 2U;

    // usage[6]
    payload_data[offset++] = 1U;
    payload_data[offset++] = 2U;
    payload_data[offset++] = 3U;
    payload_data[offset++] = 4U;
    payload_data[offset++] = 5U;
    payload_data[offset++] = 6U;

    // peak_usage[6]
    payload_data[offset++] = 11U;
    payload_data[offset++] = 12U;
    payload_data[offset++] = 13U;
    payload_data[offset++] = 14U;
    payload_data[offset++] = 15U;
    payload_data[offset++] = 16U;

    // Summary fields
    payload_data[offset++] = 25U; // t_used
    payload_data[offset++] = 40U; // t_peak
    payload_data[offset++] = 2U;  // errors
    payload_data[offset++] = 0U;  // reserved

    CASTLE_SAMPLE_CHECK(offset == mon_txbuf::payload_length);

    // Encode into a UBX frame.
    castle::container::array_view<const uint8_t> payload(
        payload_data,
        sizeof(payload_data));

    uint8_t frame[64U];
    castle::size_type bytes_written = 0U;

    castle::status encode_status = encode_frame(
        UBX_CLASS_MON,
        UBX_ID_MON_TXBUF,
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
    CASTLE_SAMPLE_CHECK(mon_txbuf::matches(raw));

    // Decode MON-TXBUF payload.
    mon_txbuf txbuf;
    CASTLE_SAMPLE_CHECK(mon_txbuf::decode(raw, txbuf));

    // Verify pending bytes.
    CASTLE_SAMPLE_CHECK(txbuf.pending[0U] == 10U);
    CASTLE_SAMPLE_CHECK(txbuf.pending[1U] == 20U);
    CASTLE_SAMPLE_CHECK(txbuf.pending[2U] == 30U);
    CASTLE_SAMPLE_CHECK(txbuf.pending[3U] == 40U);
    CASTLE_SAMPLE_CHECK(txbuf.pending[4U] == 50U);
    CASTLE_SAMPLE_CHECK(txbuf.pending[5U] == 60U);

    // Verify current usage.
    CASTLE_SAMPLE_CHECK(txbuf.usage[0U] == 1U);
    CASTLE_SAMPLE_CHECK(txbuf.usage[5U] == 6U);

    // Verify peak usage.
    CASTLE_SAMPLE_CHECK(txbuf.peak_usage[0U] == 11U);
    CASTLE_SAMPLE_CHECK(txbuf.peak_usage[5U] == 16U);

    // Verify summary statistics.
    CASTLE_SAMPLE_CHECK(txbuf.t_used == 25U);
    CASTLE_SAMPLE_CHECK(txbuf.t_peak == 40U);
    CASTLE_SAMPLE_CHECK(txbuf.errors == 2U);

    return 0;
}
