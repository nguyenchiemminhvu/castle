#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/ubx_config.hpp"

namespace
{

using namespace castle_ext::protocols::ubx;
using namespace castle_ext::protocols::ubx::config;

// Synthetic CFG key IDs, one per recognized size code (bit pattern matches real UBX keys).
static CASTLE_CONSTEXPR uint32_t KEY_L_BOOL   = 0x10210001U; // size code 1 -> 1 byte (L)
static CASTLE_CONSTEXPR uint32_t KEY_U1       = 0x20210002U; // size code 2 -> 1 byte (U1)
static CASTLE_CONSTEXPR uint32_t KEY_U2       = 0x30210001U; // size code 3 -> 2 bytes (CFG-RATE-MEAS)
static CASTLE_CONSTEXPR uint32_t KEY_U4       = 0x40230001U; // size code 4 -> 4 bytes
static CASTLE_CONSTEXPR uint32_t KEY_U8       = 0x50230001U; // size code 5 -> 8 bytes
static CASTLE_CONSTEXPR uint32_t KEY_UNKNOWN  = 0x00230001U; // size code 0 -> unrecognized

// ---------------------------------------------------------------------------
// config_value
// ---------------------------------------------------------------------------

TEST(UbxConfigValue, DefaultIsZero)
{
    config_value value;
    EXPECT_EQ(value.as_u64(), 0U);
}

TEST(UbxConfigValue, BoolConstruction)
{
    EXPECT_TRUE(config_value(true).as_bool());
    EXPECT_FALSE(config_value(false).as_bool());
    EXPECT_EQ(config_value(true).as_u64(), 1ULL);
    EXPECT_EQ(config_value(false).as_u64(), 0ULL);
}

TEST(UbxConfigValue, UnsignedConstructionAndAccessors)
{
    EXPECT_EQ(config_value(static_cast<uint8_t>(0xABU)).as_u8(), 0xABU);
    EXPECT_EQ(config_value(static_cast<uint16_t>(0x1234U)).as_u16(), 0x1234U);
    EXPECT_EQ(config_value(static_cast<uint32_t>(0x12345678U)).as_u32(), 0x12345678U);
    EXPECT_EQ(config_value(static_cast<uint64_t>(0x0102030405060708ULL)).as_u64(), 0x0102030405060708ULL);
}

TEST(UbxConfigValue, SignedConstructionAndAccessors)
{
    EXPECT_EQ(config_value(static_cast<int8_t>(-1)).as_i8(), static_cast<int8_t>(-1));
    EXPECT_EQ(config_value(static_cast<int8_t>(5)).as_i8(), static_cast<int8_t>(5));

    EXPECT_EQ(config_value(static_cast<int16_t>(-1234)).as_i16(), static_cast<int16_t>(-1234));
    EXPECT_EQ(config_value(static_cast<int32_t>(-123456)).as_i32(), static_cast<int32_t>(-123456));
    EXPECT_EQ(config_value(static_cast<int64_t>(-1)).as_i64(), static_cast<int64_t>(-1));
}

TEST(UbxConfigValue, EqualityOperators)
{
    config_value a(static_cast<uint32_t>(42U));
    config_value b(static_cast<uint32_t>(42U));
    config_value c(static_cast<uint32_t>(43U));

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

// ---------------------------------------------------------------------------
// config_layer
// ---------------------------------------------------------------------------

TEST(UbxConfigLayer, OperatorOrCombinesBitFlags)
{
    EXPECT_EQ(config_layer::ram | config_layer::bbr, 0x03U);
    EXPECT_EQ(config_layer::ram | config_layer::flash, 0x05U);
    EXPECT_EQ(config_layer::bbr | config_layer::flash, 0x06U);
}

// ---------------------------------------------------------------------------
// write_config_value / read_config_value
// ---------------------------------------------------------------------------

TEST(UbxConfigCodec, WriteAndReadEachWidth)
{
    uint8_t buffer[8U] = {};

    EXPECT_TRUE(write_config_value(config_value(static_cast<uint8_t>(0x7AU)), 1U, buffer));
    EXPECT_EQ(buffer[0U], 0x7AU);
    EXPECT_EQ(read_config_value(buffer, 1U).as_u8(), 0x7AU);

    EXPECT_TRUE(write_config_value(config_value(static_cast<uint16_t>(0x1234U)), 2U, buffer));
    EXPECT_EQ(read_le16(buffer), 0x1234U);
    EXPECT_EQ(read_config_value(buffer, 2U).as_u16(), 0x1234U);

    EXPECT_TRUE(write_config_value(config_value(static_cast<uint32_t>(0x89ABCDEFU)), 4U, buffer));
    EXPECT_EQ(read_le32(buffer), 0x89ABCDEFU);
    EXPECT_EQ(read_config_value(buffer, 4U).as_u32(), 0x89ABCDEFU);

    EXPECT_TRUE(write_config_value(config_value(static_cast<uint64_t>(0x0102030405060708ULL)), 8U, buffer));
    EXPECT_EQ(read_le64(buffer), 0x0102030405060708ULL);
    EXPECT_EQ(read_config_value(buffer, 8U).as_u64(), 0x0102030405060708ULL);
}

TEST(UbxConfigCodec, WriteUnknownSizeFails)
{
    uint8_t buffer[8U] = {};
    EXPECT_FALSE(write_config_value(config_value(static_cast<uint32_t>(1U)), 3U, buffer));
    EXPECT_FALSE(write_config_value(config_value(static_cast<uint32_t>(1U)), 0U, buffer));
}

TEST(UbxConfigCodec, ReadUnknownSizeReturnsZero)
{
    uint8_t buffer[8U] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    EXPECT_EQ(read_config_value(buffer, 3U).as_u64(), 0U);
    EXPECT_EQ(read_config_value(buffer, 0U).as_u64(), 0U);
}

// ---------------------------------------------------------------------------
// build_valset_frame
// ---------------------------------------------------------------------------

TEST(UbxConfigBuildValset, EmptyEntriesIsInvalidArgument)
{
    uint8_t output[64U];
    castle::size_type written = 0xDEADU;
    castle::status result = build_valset_frame(
        castle::container::array_view<CASTLE_CONST config_entry>(nullptr, 0U),
        static_cast<uint8_t>(config_layer::ram),
        output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(UbxConfigBuildValset, NullOutputIsInvalidArgument)
{
    config_entry entries[1U] = {{KEY_U2, config_value(static_cast<uint16_t>(1000U))}};
    castle::size_type written = 0xDEADU;
    castle::status result = build_valset_frame(
        castle::container::array_view<CASTLE_CONST config_entry>(entries, 1U),
        static_cast<uint8_t>(config_layer::ram),
        nullptr, 64U, written);

    EXPECT_EQ(result, castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(UbxConfigBuildValset, UnknownKeySizeIsInvalidArgument)
{
    config_entry entries[1U] = {{KEY_UNKNOWN, config_value(static_cast<uint32_t>(1U))}};
    uint8_t output[64U];
    castle::size_type written = 0xDEADU;
    castle::status result = build_valset_frame(
        castle::container::array_view<CASTLE_CONST config_entry>(entries, 1U),
        static_cast<uint8_t>(config_layer::ram),
        output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(UbxConfigBuildValset, ScratchTooSmallForHeaderIsFull)
{
    config_entry entries[1U] = {{KEY_U2, config_value(static_cast<uint16_t>(1000U))}};
    uint8_t output[64U];
    castle::size_type written = 0xDEADU;
    // MaxPayloadLen smaller than the 4-byte header itself.
    castle::status result = build_valset_frame<2U>(
        castle::container::array_view<CASTLE_CONST config_entry>(entries, 1U),
        static_cast<uint8_t>(config_layer::ram),
        output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxConfigBuildValset, ScratchTooSmallForEntryIsFull)
{
    config_entry entries[1U] = {{KEY_U8, config_value(static_cast<uint64_t>(1U))}};
    uint8_t output[64U];
    castle::size_type written = 0xDEADU;
    // Header (4) + key(4) + 8-byte value = 16, MaxPayloadLen only fits the header.
    castle::status result = build_valset_frame<8U>(
        castle::container::array_view<CASTLE_CONST config_entry>(entries, 1U),
        static_cast<uint8_t>(config_layer::ram),
        output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxConfigBuildValset, OutputBufferTooSmallForwardsFull)
{
    config_entry entries[1U] = {{KEY_U2, config_value(static_cast<uint16_t>(1000U))}};
    uint8_t output[4U]; // smaller than the required encoded frame size
    castle::size_type written = 0xDEADU;
    castle::status result = build_valset_frame(
        castle::container::array_view<CASTLE_CONST config_entry>(entries, 1U),
        static_cast<uint8_t>(config_layer::ram),
        output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxConfigBuildValset, SuccessRoundTripsThroughDecodeFrame)
{
    config_entry entries[4U] = {
        {KEY_L_BOOL, config_value(true)},
        {KEY_U2, config_value(static_cast<uint16_t>(1000U))},
        {KEY_U4, config_value(static_cast<uint32_t>(0x11223344U))},
        {KEY_U8, config_value(static_cast<uint64_t>(0x1122334455667788ULL))},
    };
    uint8_t output[64U];
    castle::size_type written = 0U;
    CASTLE_CONST uint8_t layers = config_layer::ram | config_layer::bbr;

    castle::status result = build_valset_frame(
        castle::container::array_view<CASTLE_CONST config_entry>(entries, 4U),
        layers, output, sizeof(output), written);

    ASSERT_EQ(result, castle::status::ok);
    ASSERT_GT(written, 0U);

    message_view view;
    ASSERT_TRUE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(output, written), view));
    EXPECT_TRUE(view.is(UBX_CLASS_CFG, UBX_ID_CFG_VALSET));

    ASSERT_EQ(view.payload_length(), 4U + (4U + 1U) + (4U + 2U) + (4U + 4U) + (4U + 8U));
    EXPECT_EQ(view.payload[0U], UBX_VALSET_VERSION);
    EXPECT_EQ(view.payload[1U], layers);
    EXPECT_EQ(view.payload[2U], 0U);
    EXPECT_EQ(view.payload[3U], 0U);

    config_entry parsed[4U];
    castle::size_type parsed_count = parse_valget_response(view.payload, parsed, 4U);
    ASSERT_EQ(parsed_count, 4U);
    EXPECT_EQ(parsed[0U].key_id, KEY_L_BOOL);
    EXPECT_TRUE(parsed[0U].value.as_bool());
    EXPECT_EQ(parsed[1U].key_id, KEY_U2);
    EXPECT_EQ(parsed[1U].value.as_u16(), 1000U);
    EXPECT_EQ(parsed[2U].key_id, KEY_U4);
    EXPECT_EQ(parsed[2U].value.as_u32(), 0x11223344U);
    EXPECT_EQ(parsed[3U].key_id, KEY_U8);
    EXPECT_EQ(parsed[3U].value.as_u64(), 0x1122334455667788ULL);
}

// ---------------------------------------------------------------------------
// build_valget_frame
// ---------------------------------------------------------------------------

TEST(UbxConfigBuildValget, EmptyKeysIsInvalidArgument)
{
    uint8_t output[64U];
    castle::size_type written = 0xDEADU;
    castle::status result = build_valget_frame(
        castle::container::array_view<CASTLE_CONST uint32_t>(nullptr, 0U),
        UBX_CFG_VALGET_LAYER_RAM, 0U, output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(UbxConfigBuildValget, NullOutputIsInvalidArgument)
{
    uint32_t keys[1U] = {KEY_U2};
    castle::size_type written = 0xDEADU;
    castle::status result = build_valget_frame(
        castle::container::array_view<CASTLE_CONST uint32_t>(keys, 1U),
        UBX_CFG_VALGET_LAYER_RAM, 0U, nullptr, 64U, written);

    EXPECT_EQ(result, castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(UbxConfigBuildValget, ScratchTooSmallIsFull)
{
    uint32_t keys[2U] = {KEY_U2, KEY_U4};
    uint8_t output[64U];
    castle::size_type written = 0xDEADU;
    // required = 4 + 2*4 = 12, MaxPayloadLen only fits 1 key.
    castle::status result = build_valget_frame<8U>(
        castle::container::array_view<CASTLE_CONST uint32_t>(keys, 2U),
        UBX_CFG_VALGET_LAYER_RAM, 0U, output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxConfigBuildValget, OutputBufferTooSmallForwardsFull)
{
    uint32_t keys[1U] = {KEY_U2};
    uint8_t output[4U];
    castle::size_type written = 0xDEADU;
    castle::status result = build_valget_frame(
        castle::container::array_view<CASTLE_CONST uint32_t>(keys, 1U),
        UBX_CFG_VALGET_LAYER_RAM, 0U, output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxConfigBuildValget, SuccessRoundTripsThroughDecodeFrame)
{
    uint32_t keys[3U] = {KEY_U2, KEY_U4, KEY_U8};
    uint8_t output[64U];
    castle::size_type written = 0U;

    castle::status result = build_valget_frame(
        castle::container::array_view<CASTLE_CONST uint32_t>(keys, 3U),
        UBX_CFG_VALGET_LAYER_BBR, 7U, output, sizeof(output), written);

    ASSERT_EQ(result, castle::status::ok);
    ASSERT_GT(written, 0U);

    message_view view;
    ASSERT_TRUE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(output, written), view));
    EXPECT_TRUE(view.is(UBX_CLASS_CFG, UBX_ID_CFG_VALGET));
    ASSERT_EQ(view.payload_length(), 4U + (3U * 4U));

    EXPECT_EQ(view.payload[0U], UBX_VALGET_VERSION_POLL);
    EXPECT_EQ(view.payload[1U], UBX_CFG_VALGET_LAYER_BBR);
    EXPECT_EQ(read_le16(&view.payload[2U]), 7U);
    EXPECT_EQ(read_le32(&view.payload[4U]), KEY_U2);
    EXPECT_EQ(read_le32(&view.payload[8U]), KEY_U4);
    EXPECT_EQ(read_le32(&view.payload[12U]), KEY_U8);
}

// ---------------------------------------------------------------------------
// parse_valget_response
// ---------------------------------------------------------------------------

TEST(UbxConfigParseValgetResponse, PayloadShorterThanHeaderReturnsZero)
{
    uint8_t payload_data[3U] = {0U, 0U, 0U};
    config_entry entries[2U];
    castle::size_type count = parse_valget_response(
        castle::container::array_view<CASTLE_CONST uint8_t>(payload_data, 3U), entries, 2U);
    EXPECT_EQ(count, 0U);
}

TEST(UbxConfigParseValgetResponse, NullEntriesOrZeroCapacityReturnsZero)
{
    uint8_t payload_data[4U] = {UBX_VALGET_VERSION_RESP, 0U, 0U, 0U};
    castle::container::array_view<CASTLE_CONST uint8_t> payload(payload_data, 4U);
    config_entry entries[1U];

    EXPECT_EQ(parse_valget_response(payload, nullptr, 2U), 0U);
    EXPECT_EQ(parse_valget_response(payload, entries, 0U), 0U);
}

TEST(UbxConfigParseValgetResponse, StopsAtUnrecognizedKeySize)
{
    // header(4) + one valid U2 entry(4+2) + one malformed key (unknown size code)
    uint8_t payload_data[16U] = {};
    payload_data[0U] = UBX_VALGET_VERSION_RESP;
    payload_data[1U] = UBX_CFG_VALGET_LAYER_RAM;
    write_le16(&payload_data[2U], 0U);
    write_le32(&payload_data[4U], KEY_U2);
    write_le16(&payload_data[8U], 1000U);
    write_le32(&payload_data[10U], KEY_UNKNOWN);

    config_entry entries[4U];
    castle::size_type count = parse_valget_response(
        castle::container::array_view<CASTLE_CONST uint8_t>(payload_data, 14U), entries, 4U);

    ASSERT_EQ(count, 1U);
    EXPECT_EQ(entries[0U].key_id, KEY_U2);
    EXPECT_EQ(entries[0U].value.as_u16(), 1000U);
}

TEST(UbxConfigParseValgetResponse, StopsWhenValueTruncated)
{
    // header(4) + a U4 key whose value bytes are cut short by the payload view.
    uint8_t payload_data[9U] = {};
    payload_data[0U] = UBX_VALGET_VERSION_RESP;
    write_le32(&payload_data[4U], KEY_U4);
    // Only 1 byte of value data follows instead of the required 4.

    config_entry entries[2U];
    castle::size_type count = parse_valget_response(
        castle::container::array_view<CASTLE_CONST uint8_t>(payload_data, 9U), entries, 2U);

    EXPECT_EQ(count, 0U);
}

TEST(UbxConfigParseValgetResponse, ClampsToEntriesCapacity)
{
    uint8_t payload_data[4U + (4U + 1U) * 3U] = {};
    payload_data[0U] = UBX_VALGET_VERSION_RESP;
    castle::size_type pos = 4U;
    for (uint32_t i = 0U; i < 3U; ++i)
    {
        write_le32(&payload_data[pos], KEY_U1);
        payload_data[pos + 4U] = static_cast<uint8_t>(i + 1U);
        pos += 5U;
    }

    config_entry entries[2U];
    castle::size_type count = parse_valget_response(
        castle::container::array_view<CASTLE_CONST uint8_t>(payload_data, sizeof(payload_data)), entries, 2U);

    ASSERT_EQ(count, 2U);
    EXPECT_EQ(entries[0U].value.as_u8(), 1U);
    EXPECT_EQ(entries[1U].value.as_u8(), 2U);
}

TEST(UbxConfigParseValgetResponse, ExactlyFitsWithNoTrailingBytes)
{
    uint8_t payload_data[4U + 4U + 2U] = {};
    payload_data[0U] = UBX_VALGET_VERSION_RESP;
    write_le32(&payload_data[4U], KEY_U2);
    write_le16(&payload_data[8U], 42U);

    config_entry entries[1U];
    castle::size_type count = parse_valget_response(
        castle::container::array_view<CASTLE_CONST uint8_t>(payload_data, sizeof(payload_data)), entries, 1U);

    ASSERT_EQ(count, 1U);
    EXPECT_EQ(entries[0U].key_id, KEY_U2);
    EXPECT_EQ(entries[0U].value.as_u16(), 42U);
}

// ---------------------------------------------------------------------------
// config_layer and config_value tests
// ---------------------------------------------------------------------------

TEST(UbxProtocol, LayerBitmaskAndValueConversionsAreDeterministic)
{
    EXPECT_EQ(config_layer::ram | config_layer::flash, 0x05U);
    const config_value value(static_cast<int32_t>(-2));
    EXPECT_EQ(value.as_u32(), 0xFFFFFFFEU);
    EXPECT_EQ(value.as_i32(), -2);
    EXPECT_TRUE(value != config_value(static_cast<uint32_t>(1U)));
}

TEST(UbxProtocol, ReadWriteEightByteConfigurationValueRoundTrips)
{
    uint8_t bytes[8U] = {};
    const config_value input(static_cast<uint64_t>(0x0123456789ABCDEFULL));
    ASSERT_TRUE(write_config_value(input, 8U, bytes));
    EXPECT_EQ(read_config_value(bytes, 8U), input);
}

} // namespace
