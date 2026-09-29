#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/payload_reader.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;

    // Example payload:
    // U32 = 0x12345678
    // U16 = 0xABCD
    // U8  = 0x42
    uint8_t payload_data[] =
    {
        0x78U, 0x56U, 0x34U, 0x12U,
        0xCDU, 0xABU,
        0x42U
    };

    payload_reader reader(
        castle::container::array_view<const uint8_t>(
            payload_data,
            sizeof(payload_data)));

    // Read fields sequentially.
    uint32_t value32 = reader.read_u32();
    uint16_t value16 = reader.read_u16();
    uint8_t value8 = reader.read_u8();

    CASTLE_SAMPLE_CHECK(value32 == 0x12345678U);
    CASTLE_SAMPLE_CHECK(value16 == 0xABCDU);
    CASTLE_SAMPLE_CHECK(value8 == 0x42U);

    // Reader tracks current position and remaining bytes.
    CASTLE_SAMPLE_CHECK(reader.position() == 7U);
    CASTLE_SAMPLE_CHECK(reader.remaining() == 0U);

    CASTLE_SAMPLE_CHECK(reader.ok());
    CASTLE_SAMPLE_CHECK(reader.position() == sizeof(payload_data));
    CASTLE_SAMPLE_CHECK(reader.remaining() == 0U);

    // Skip over reserved bytes.
    uint8_t reserved_payload[] =
    {
        0x11U,
        0x22U,
        0xAAU,
        0xBBU,
        0x33U
    };

    payload_reader skip_reader(
        castle::container::array_view<const uint8_t>(
            reserved_payload,
            sizeof(reserved_payload)));

    CASTLE_SAMPLE_CHECK(skip_reader.read_u8() == 0x11U);
    CASTLE_SAMPLE_CHECK(skip_reader.read_u8() == 0x22U);

    bool skipped = skip_reader.skip(2U);
    CASTLE_SAMPLE_CHECK(skipped);

    CASTLE_SAMPLE_CHECK(skip_reader.read_u8() == 0x33U);
    CASTLE_SAMPLE_CHECK(skip_reader.ok());

    // Reads beyond the end latch the reader into an error state.
    uint8_t short_payload[] = {0x01U, 0x02U};

    payload_reader failing_reader(
        castle::container::array_view<const uint8_t>(
            short_payload,
            sizeof(short_payload)));

    uint32_t oversized = failing_reader.read_u32();

    CASTLE_SAMPLE_CHECK(oversized == 0U);
    CASTLE_SAMPLE_CHECK(!failing_reader.ok());

    // Once failed, all subsequent reads continue to fail safely.
    CASTLE_SAMPLE_CHECK(failing_reader.read_u8() == 0U);
    CASTLE_SAMPLE_CHECK(failing_reader.read_bytes(1U) == nullptr);
    CASTLE_SAMPLE_CHECK(!failing_reader.ok());

    return 0;
}
