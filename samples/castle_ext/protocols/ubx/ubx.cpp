#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"

int main()
{
    using namespace castle::protocols::ubx;

    // Encode a CFG-VALSET frame with a 2-byte payload into a caller buffer.
    uint8_t payload_data[2U] = {0x01U, 0x02U};
    castle::container::array_view<const uint8_t> payload(payload_data, 2U);

    uint8_t frame[64U];
    castle::size_type written = 0U;
    castle::status encode_status = encode_frame(
        UBX_CLASS_CFG, UBX_ID_CFG_VALSET, payload, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));
    CASTLE_SAMPLE_CHECK(written == frame_size(payload.size()));

    // Decode it back into a zero-copy view over the same buffer.
    message_view view;
    bool decoded = decode_frame(castle::container::array_view<const uint8_t>(frame, written), view);
    CASTLE_SAMPLE_CHECK(decoded);
    CASTLE_SAMPLE_CHECK(view.is(UBX_CLASS_CFG, UBX_ID_CFG_VALSET));
    CASTLE_SAMPLE_CHECK(view.payload_length() == 2U);
    CASTLE_SAMPLE_CHECK(view.payload[0U] == 0x01U);
    CASTLE_SAMPLE_CHECK(view.payload[1U] == 0x02U);

    // A single corrupted byte flips the checksum and decode_frame() rejects it.
    uint8_t corrupted[64U];
    for (castle::size_type i = 0U; i < written; ++i)
    {
        corrupted[i] = frame[i];
    }
    corrupted[written - 1U] ^= 0xFFU;
    message_view rejected;
    CASTLE_SAMPLE_CHECK(!decode_frame(castle::container::array_view<const uint8_t>(corrupted, written), rejected));

    // Little-endian field helpers used to decode a message payload manually.
    uint8_t little_endian_u32[4U] = {0x78U, 0x56U, 0x34U, 0x12U};
    CASTLE_SAMPLE_CHECK(castle::read_le32(little_endian_u32) == 0x12345678U);

    // A UBX CFG key ID's top nibble encodes the stored value's byte width.
    CASTLE_SAMPLE_CHECK(value_byte_size(0x10000000U) == 1U); // L/U1/I1/X1/E1
    CASTLE_SAMPLE_CHECK(value_byte_size(0x40000000U) == 4U); // U4/I4/X4/E4

    return 0;
}
