#include <gtest/gtest.h>
#include "castle_ext/comm/http/hpack.hpp"
#include "castle_ext/comm/http/headers.hpp"

using namespace castle::comm::http;

TEST(Hpack, DecodesStaticIndexedFields)
{
    const uint8_t block[] = {0x82U, 0x88U}; // :method GET, :status 200
    basic_headers<4U, 64U, 128U> headers;
    hpack_decoder<> decoder;
    EXPECT_EQ(decoder.decode_block(block, sizeof(block), headers), castle::status::ok);
    EXPECT_TRUE(ascii_iequal(headers.get(":method"), "GET"));
    EXPECT_TRUE(ascii_iequal(headers.get(":status"), "200"));
    EXPECT_EQ(decoder.dynamic_table_size(), 0U);
}

TEST(Hpack, DecodesRfcHuffmanString)
{
    const uint8_t block[] = {0x41U, 0x8CU, 0xF1U, 0xE3U, 0xC2U, 0xE5U, 0xF2U,
                             0x3AU, 0x6BU, 0xA0U, 0xABU, 0x90U, 0xF4U, 0xFFU};
    basic_headers<2U, 64U, 128U> headers;
    hpack_decoder<> decoder;
    EXPECT_EQ(decoder.decode_block(block, sizeof(block), headers), castle::status::ok);
    EXPECT_TRUE(ascii_iequal(headers.get(":authority"), "www.example.com"));
}

TEST(Hpack, DynamicTableIsBoundedAndCanBeDisabled)
{
    const uint8_t insert[] = {0x40U, 0x01U, 'x', 0x01U, 'y'};
    basic_headers<4U, 16U, 16U> headers;
    hpack_decoder<128U, 16U, 2U> decoder;
    EXPECT_EQ(decoder.decode_block(insert, sizeof(insert), headers), castle::status::ok);
    EXPECT_EQ(decoder.dynamic_entry_count(), 1U);
    EXPECT_EQ(decoder.dynamic_table_size(), 34U);
    const uint8_t dynamic_index[] = {0xBEU}; // dynamic entry index 62
    basic_headers<4U, 16U, 16U> second;
    EXPECT_EQ(decoder.decode_block(dynamic_index, sizeof(dynamic_index), second), castle::status::ok);
    EXPECT_TRUE(ascii_iequal(second.get("x"), "y"));

    basic_headers<4U, 16U, 16U> no_dynamic_headers;
    hpack_decoder<0U, 16U, 1U> no_dynamic;
    EXPECT_EQ(no_dynamic.decode_block(insert, sizeof(insert), no_dynamic_headers), castle::status::ok);
    EXPECT_EQ(no_dynamic.dynamic_table_size(), 0U);
    EXPECT_EQ(no_dynamic.dynamic_entry_count(), 0U);
}

TEST(Hpack, ClearRestoresConfiguredDynamicTableLimit)
{
    hpack_decoder<128U, 16U, 4U> decoder;
    basic_headers<2U, 16U, 16U> headers;
    const uint8_t reduce_to_zero[] = {0x20U};
    EXPECT_EQ(decoder.decode_block(reduce_to_zero, sizeof(reduce_to_zero), headers), castle::status::ok);
    EXPECT_EQ(decoder.maximum_table_size(), 0U);
    decoder.clear_dynamic_table();
    EXPECT_EQ(decoder.maximum_table_size(), 128U);
}

TEST(Hpack, RejectsInvalidIndexesAndTruncatedStrings)
{
    hpack_decoder<> decoder;
    basic_headers<4U, 32U, 64U> headers;
    const uint8_t zero_index[] = {0x80U};
    const uint8_t truncated[] = {0x00U, 0x05U, 'a'};
    EXPECT_EQ(decoder.decode_block(zero_index, sizeof(zero_index), headers), castle::status::invalid_argument);
    EXPECT_EQ(decoder.decode_block(truncated, sizeof(truncated), headers), castle::status::data_loss);
    EXPECT_EQ(decoder.decode_block(nullptr, 1U, headers), castle::status::invalid_argument);
}

TEST(Hpack, EncoderRoundTripsLiteralAndReportsCapacity)
{
    uint8_t encoded[64U]{};
    castle::size_type used = 0U;
    EXPECT_EQ(hpack_encoder::encode_literal("x-device", "embedded", encoded, sizeof(encoded), used), castle::status::ok);
    basic_headers<2U, 32U, 32U> headers;
    hpack_decoder<> decoder;
    EXPECT_EQ(decoder.decode_block(encoded, used, headers), castle::status::ok);
    EXPECT_TRUE(ascii_iequal(headers.get("x-device"), "embedded"));
    EXPECT_EQ(hpack_encoder::encode_literal("x", "v", encoded, 1U, used), castle::status::full);
    EXPECT_EQ(hpack_encoder::encode_literal("", "v", encoded, sizeof(encoded), used), castle::status::invalid_argument);
}
