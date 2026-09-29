#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/cfg_valget.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    uint8_t payload[64U];
    castle::size_type offset = 0U;

    // ---------------------------------------------------------------------
    // CFG-VALGET response header
    // ---------------------------------------------------------------------

    payload[offset++] = UBX_VALGET_VERSION_RESP; // version
    payload[offset++] = 1U;                      // RAM layer
    write_le16(&payload[offset], 0U);            // position
    offset += 2U;

    // ---------------------------------------------------------------------
    // Entry #1 : 1-byte value
    // key size code = 1 -> 1-byte value
    // ---------------------------------------------------------------------

    write_le32(&payload[offset], 0x10000001U);
    offset += 4U;

    payload[offset++] = 0x55U;

    // ---------------------------------------------------------------------
    // Entry #2 : 2-byte value
    // key size code = 3 -> 2-byte value
    // ---------------------------------------------------------------------

    write_le32(&payload[offset], 0x30000002U);
    offset += 4U;

    write_le16(&payload[offset], 0x1234U);
    offset += 2U;

    // ---------------------------------------------------------------------
    // Entry #3 : 4-byte value
    // key size code = 4 -> 4-byte value
    // ---------------------------------------------------------------------

    write_le32(&payload[offset], 0x40000003U);
    offset += 4U;

    write_le32(&payload[offset], 0x89ABCDEFU);
    offset += 4U;

    // ---------------------------------------------------------------------
    // Build UBX frame
    // ---------------------------------------------------------------------

    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        UBX_CLASS_CFG,
        UBX_ID_CFG_VALGET,
        castle::container::array_view<const uint8_t>(payload, offset),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // ---------------------------------------------------------------------
    // Decode raw UBX frame
    // ---------------------------------------------------------------------

    message_view raw;

    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);
    CASTLE_SAMPLE_CHECK(cfg_valget<>::matches(raw));

    // ---------------------------------------------------------------------
    // Decode CFG-VALGET message
    // ---------------------------------------------------------------------

    cfg_valget<> response;

    CASTLE_SAMPLE_CHECK(cfg_valget<>::decode(raw, response));

    CASTLE_SAMPLE_CHECK(response.version == UBX_VALGET_VERSION_RESP);
    CASTLE_SAMPLE_CHECK(response.layer == 1U);
    CASTLE_SAMPLE_CHECK(response.position == 0U);

    CASTLE_SAMPLE_CHECK(response.entry_count == 3U);

    // Entry #1
    CASTLE_SAMPLE_CHECK(response.entries[0U].key_id == 0x10000001U);
    CASTLE_SAMPLE_CHECK(response.entries[0U].value == 0x55U);

    // Entry #2
    CASTLE_SAMPLE_CHECK(response.entries[1U].key_id == 0x30000002U);
    CASTLE_SAMPLE_CHECK(response.entries[1U].value == 0x1234U);

    // Entry #3
    CASTLE_SAMPLE_CHECK(response.entries[2U].key_id == 0x40000003U);
    CASTLE_SAMPLE_CHECK(response.entries[2U].value == 0x89ABCDEFU);

    // Verify value size extraction logic.
    CASTLE_SAMPLE_CHECK(cfg_valget<>::value_size(0x10000001U) == 1U);
    CASTLE_SAMPLE_CHECK(cfg_valget<>::value_size(0x30000002U) == 2U);
    CASTLE_SAMPLE_CHECK(cfg_valget<>::value_size(0x40000003U) == 4U);

    return 0;
}
