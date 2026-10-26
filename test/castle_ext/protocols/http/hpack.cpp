#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include "castle_ext/protocols/http/hpack.hpp"
#include "castle_ext/protocols/http/headers.hpp"

using namespace castle::protocols::http;
using castle::size_type;

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

TEST(Hpack, EncodeFieldUsesStaticTableIndexes)
{
    uint8_t encoded[32U]{};
    castle::size_type used = 0U;
    EXPECT_EQ(hpack_encoder::encode_field(":method", "GET", encoded, sizeof(encoded), used), castle::status::ok);
    ASSERT_EQ(used, 1U);
    EXPECT_EQ(encoded[0U], 0x82U); // indexed field, static entry 2

    EXPECT_EQ(hpack_encoder::encode_field(":path", "/x", encoded, sizeof(encoded), used), castle::status::ok);
    EXPECT_EQ(encoded[0U], 0x04U); // literal without indexing, name from static entry 4
    basic_headers<2U, 16U, 16U> headers;
    hpack_decoder<> decoder;
    EXPECT_EQ(decoder.decode_block(encoded, used, headers), castle::status::ok);
    EXPECT_TRUE(ascii_iequal(headers.get(":path"), "/x"));

    EXPECT_EQ(hpack_encoder::encode_field("x-id", "1", encoded, sizeof(encoded), used), castle::status::ok);
    EXPECT_EQ(encoded[0U], 0x00U); // unknown name stays a literal
    EXPECT_EQ(hpack_encoder::encode_field(":method", "GET", encoded, 0U, used), castle::status::full);
    EXPECT_EQ(hpack_encoder::encode_field("", "v", encoded, sizeof(encoded), used), castle::status::invalid_argument);
}

TEST(Hpack, RejectsEosSymbolInHuffmanString)
{
    const uint8_t block[] = {0x00U, 0x01U, 'x', 0x84U, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    basic_headers<2U, 16U, 16U> headers;
    hpack_decoder<> decoder;
    EXPECT_EQ(decoder.decode_block(block, sizeof(block), headers), castle::status::invalid_argument);
}

TEST(Hpack, OversizedInsertEmptiesTableButKeepsNegotiatedSize)
{
    // Size update to 40 bytes, then an incremental literal of 32 + 1 + 16 = 49 bytes.
    const uint8_t block[] = {0x3FU, 0x09U, 0x40U, 0x01U, 'x', 0x10U,
                             'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a'};
    basic_headers<2U, 16U, 16U> headers;
    hpack_decoder<128U, 16U, 4U> decoder;
    EXPECT_EQ(decoder.decode_block(block, sizeof(block), headers), castle::status::ok);
    EXPECT_EQ(decoder.dynamic_entry_count(), 0U);
    EXPECT_EQ(decoder.maximum_table_size(), 40U);
}

namespace
{
// Encodes `symbols` with their RFC 7541 codes, appends `tail` bits and pads the last byte with `pad`.
std::string huffman(char const* symbols, char const* tail, char pad)
{
    std::string bits;
    for (size_type i = 0U; symbols[i] != '\0'; ++i)
    {
        uint8_t symbol = static_cast<uint8_t>(symbols[i]);
        uint32_t code = detail::hpack_huffman_codes[symbol];
        for (int b = detail::hpack_huffman_lengths[symbol] - 1; b >= 0; --b)
        {
            bits.push_back(((code >> b) & 1U) != 0U ? '1' : '0');
        }
    }
    bits += tail;
    while (bits.size() % 8U != 0U)
    {
        bits.push_back(pad);
    }
    std::string bytes;
    for (size_type i = 0U; i < bits.size(); i += 8U)
    {
        uint8_t value = 0U;
        for (size_type j = 0U; j < 8U; ++j)
        {
            value = static_cast<uint8_t>((value << 1U) | (bits[i + j] == '1' ? 1U : 0U));
        }
        bytes.push_back(static_cast<char>(value));
    }
    return bytes;
}

// Literal field with a Huffman-coded value and static name index 1 (:authority).
std::string huffman_authority(std::string const& value)
{
    std::string block;
    block.push_back(static_cast<char>(0x01U));
    block.push_back(static_cast<char>(0x80U | value.size()));
    block += value;
    return block;
}

// Appends an incrementally indexed literal (0x40 prefix) at the start of `out`.
size_type add_incremental(uint8_t* out, char const* name, char const* value)
{
    size_type at = 0U;
    out[at++] = 0x40U;
    out[at++] = static_cast<uint8_t>(std::strlen(name));
    for (size_type i = 0U; name[i] != '\0'; ++i) { out[at++] = static_cast<uint8_t>(name[i]); }
    out[at++] = static_cast<uint8_t>(std::strlen(value));
    for (size_type i = 0U; value[i] != '\0'; ++i) { out[at++] = static_cast<uint8_t>(value[i]); }
    return at;
}
}

TEST(Hpack, HuffmanPaddingAndOutputBounds)
{
    basic_headers<2U, 16U, 16U> headers;
    hpack_decoder<> decoder;
    std::string valid = huffman_authority(huffman("a", "", '1'));
    EXPECT_EQ(decoder.decode_block(reinterpret_cast<uint8_t const*>(valid.data()), valid.size(), headers), castle::status::ok);
    EXPECT_TRUE(ascii_iequal(headers.get(":authority"), "a"));

    std::string whole_bytes = huffman_authority(huffman("aa", "111111", '1')); // exactly 16 bits, no padding
    basic_headers<2U, 16U, 16U> aligned;
    EXPECT_EQ(decoder.decode_block(reinterpret_cast<uint8_t const*>(whole_bytes.data()), whole_bytes.size(), aligned), castle::status::ok);
    EXPECT_TRUE(ascii_iequal(aligned.get(":authority"), "aa"));

    std::string zero_padding = huffman_authority(huffman("a", "", '0'));
    basic_headers<2U, 16U, 16U> zero;
    EXPECT_EQ(decoder.decode_block(reinterpret_cast<uint8_t const*>(zero_padding.data()), zero_padding.size(), zero), castle::status::invalid_argument);

    std::string long_padding = huffman_authority(huffman("a", "11111111", '1')); // 11 bits of padding
    basic_headers<2U, 16U, 16U> long_tail;
    EXPECT_EQ(decoder.decode_block(reinterpret_cast<uint8_t const*>(long_padding.data()), long_padding.size(), long_tail), castle::status::invalid_argument);

    std::string www = huffman_authority(huffman("www.example.com", "", '1'));
    hpack_decoder<0U, 12U> small;
    basic_headers<2U, 16U, 16U> bounded;
    EXPECT_EQ(small.decode_block(reinterpret_cast<uint8_t const*>(www.data()), www.size(), bounded), castle::status::out_of_range);
}

TEST(Hpack, DecodesMalformedIndexedAndIntegerEncodings)
{
    hpack_decoder<> decoder;
    basic_headers<4U, 16U, 16U> headers;
    const uint8_t truncated_index[] = {0xFFU};
    EXPECT_EQ(decoder.decode_block(truncated_index, sizeof(truncated_index), headers), castle::status::invalid_argument);
    const uint8_t truncated_continuation[] = {0xFFU, 0x80U};
    EXPECT_EQ(decoder.decode_block(truncated_continuation, sizeof(truncated_continuation), headers), castle::status::invalid_argument);
    const uint8_t unterminated[] = {0xFFU, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U};
    EXPECT_EQ(decoder.decode_block(unterminated, sizeof(unterminated), headers), castle::status::invalid_argument);
    const uint8_t shift_overflow[] = {0xFFU, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x02U};
    EXPECT_EQ(decoder.decode_block(shift_overflow, sizeof(shift_overflow), headers), castle::status::invalid_argument);
    const uint8_t value_overflow[] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x01U};
    EXPECT_EQ(decoder.decode_block(value_overflow, sizeof(value_overflow), headers), castle::status::invalid_argument);
    const uint8_t zero_index[] = {0x80U};
    EXPECT_EQ(decoder.decode_block(zero_index, sizeof(zero_index), headers), castle::status::invalid_argument);
    const uint8_t beyond_static[] = {0xFFU, 0x01U};
    EXPECT_EQ(decoder.decode_block(beyond_static, sizeof(beyond_static), headers), castle::status::invalid_argument);
    const uint8_t empty_dynamic[] = {0xBEU};
    EXPECT_EQ(decoder.decode_block(empty_dynamic, sizeof(empty_dynamic), headers), castle::status::invalid_argument);
    EXPECT_EQ(decoder.decode_block(nullptr, 0U, headers), castle::status::ok);
    EXPECT_EQ(decoder.decode_block(nullptr, 1U, headers), castle::status::invalid_argument);
}

TEST(Hpack, RejectsMalformedLiteralsSizeUpdatesAndFullHeaders)
{
    hpack_decoder<128U> decoder;
    basic_headers<4U, 16U, 16U> headers;
    const uint8_t truncated_literal[] = {0x40U};
    EXPECT_EQ(decoder.decode_block(truncated_literal, sizeof(truncated_literal), headers), castle::status::data_loss);
    const uint8_t truncated_prefix[] = {0x7FU};
    EXPECT_EQ(decoder.decode_block(truncated_prefix, sizeof(truncated_prefix), headers), castle::status::data_loss);
    const uint8_t short_string[] = {0x00U, 0x01U, 'n', 0x05U, 'h'};
    EXPECT_EQ(decoder.decode_block(short_string, sizeof(short_string), headers), castle::status::data_loss);
    const uint8_t empty_name[] = {0x00U, 0x00U, 0x01U, 'x'};
    EXPECT_EQ(decoder.decode_block(empty_name, sizeof(empty_name), headers), castle::status::invalid_argument);
    const uint8_t unknown_name_index[] = {0x0FU, 0x71U};
    EXPECT_EQ(decoder.decode_block(unknown_name_index, sizeof(unknown_name_index), headers), castle::status::invalid_argument);
    const uint8_t size_after_field[] = {0x82U, 0x20U};
    EXPECT_EQ(decoder.decode_block(size_after_field, sizeof(size_after_field), headers), castle::status::invalid_argument);
    const uint8_t truncated_size[] = {0x3FU};
    EXPECT_EQ(decoder.decode_block(truncated_size, sizeof(truncated_size), headers), castle::status::invalid_argument);
    const uint8_t too_large_size[] = {0x3FU, 0x62U};
    EXPECT_EQ(decoder.decode_block(too_large_size, sizeof(too_large_size), headers), castle::status::invalid_argument);
    const uint8_t valid_size[] = {0x3FU, 0x00U};
    EXPECT_EQ(decoder.decode_block(valid_size, sizeof(valid_size), headers), castle::status::ok);
    EXPECT_EQ(decoder.maximum_table_size(), 31U);

    basic_headers<1U, 16U, 16U> one;
    const uint8_t two_indexed[] = {0x82U, 0x84U};
    EXPECT_EQ(hpack_decoder<>().decode_block(two_indexed, sizeof(two_indexed), one), castle::status::full);
    const uint8_t two_literal[] = {0x00U, 0x01U, 'a', 0x01U, 'b', 0x00U, 0x01U, 'c', 0x01U, 'd'};
    basic_headers<1U, 16U, 16U> one_literal;
    EXPECT_EQ(hpack_decoder<>().decode_block(two_literal, sizeof(two_literal), one_literal), castle::status::full);
}

TEST(Hpack, EnforcesStringLimitsOfTheDecoder)
{
    hpack_decoder<0U, 4U> decoder;
    basic_headers<2U, 16U, 16U> headers;
    const uint8_t indexed_long_name[] = {0x0FU, 0x0DU, 0x01U, 'x'}; // content-length (14 bytes) > 4
    EXPECT_EQ(decoder.decode_block(indexed_long_name, sizeof(indexed_long_name), headers), castle::status::out_of_range);
    const uint8_t long_value[] = {0x00U, 0x01U, 'n', 0x05U, 'h', 'e', 'l', 'l', 'o'};
    EXPECT_EQ(decoder.decode_block(long_value, sizeof(long_value), headers), castle::status::out_of_range);
}

TEST(Hpack, DynamicTableEvictsByBytesEntriesAndSizeUpdates)
{
    uint8_t block[256]{};
    size_type at = add_incremental(block, "x", "y");
    at += add_incremental(block + at, "aa", "bb");
    at += add_incremental(block + at, "cccc", "dddd");
    at += add_incremental(block + at, "eeeeeeee", "ffffffff");
    basic_headers<8U, 16U, 16U> headers;
    hpack_decoder<128U, 16U, 4U> decoder;
    EXPECT_EQ(decoder.decode_block(block, at, headers), castle::status::ok);
    EXPECT_EQ(decoder.dynamic_entry_count(), 3U);
    EXPECT_EQ(decoder.dynamic_table_size(), 124U);

    const uint8_t shrink_partly[] = {0x3FU, 0x45U}; // size 100 evicts only the oldest entry
    basic_headers<8U, 16U, 16U> none;
    EXPECT_EQ(decoder.decode_block(shrink_partly, sizeof(shrink_partly), none), castle::status::ok);
    EXPECT_EQ(decoder.dynamic_entry_count(), 2U);
    EXPECT_EQ(decoder.dynamic_table_size(), 88U);
    const uint8_t shrink_to_zero[] = {0x20U};
    EXPECT_EQ(decoder.decode_block(shrink_to_zero, sizeof(shrink_to_zero), none), castle::status::ok);
    EXPECT_EQ(decoder.dynamic_entry_count(), 0U);
    EXPECT_EQ(decoder.dynamic_table_size(), 0U);

    size_type second = add_incremental(block, "x", "y");
    second += add_incremental(block + second, "aa", "bb");
    second += add_incremental(block + second, "cc", "dd");
    hpack_decoder<128U, 16U, 2U> count_limited;
    EXPECT_EQ(count_limited.decode_block(block, second, none), castle::status::ok);
    EXPECT_EQ(count_limited.dynamic_entry_count(), 2U);

    hpack_decoder<40U, 16U, 4U> oversized;
    EXPECT_EQ(oversized.decode_block(block, add_incremental(block, "x", "y"), none), castle::status::ok);
    EXPECT_EQ(oversized.dynamic_entry_count(), 1U);
    uint8_t huge[64]{};
    size_type huge_size = add_incremental(huge, "longname", "longvalue");
    EXPECT_EQ(oversized.decode_block(huge, huge_size, none), castle::status::ok);
    EXPECT_EQ(oversized.dynamic_entry_count(), 0U);
}

TEST(Hpack, EncoderHandlesIndexesBoundsAndLongStrings)
{
    uint8_t buffer[400]{};
    castle::size_type used = 0U;
    EXPECT_EQ(hpack_encoder::encode_literal("x", "v", nullptr, sizeof(buffer), used), castle::status::invalid_argument);
    EXPECT_EQ(hpack_encoder::encode_field("x-id", "1", nullptr, sizeof(buffer), used), castle::status::invalid_argument);
    EXPECT_EQ(hpack_encoder::encode_field(":path", "/x", buffer, 0U, used), castle::status::full);
    EXPECT_EQ(hpack_encoder::encode_field(":method", "PURGE", buffer, 0U, used), castle::status::full);

    std::string text(300U, 'a');
    string_view value(text.data(), text.size());
    hpack_decoder<0U, 512U> decoder;
    basic_headers<2U, 16U, 512U> headers;
    ASSERT_EQ(hpack_encoder::encode_literal("x", value, buffer, sizeof(buffer), used), castle::status::ok);
    EXPECT_EQ(decoder.decode_block(buffer, used, headers), castle::status::ok);
    EXPECT_EQ(headers.get("x").size(), 300U);

    // A 300-byte value has a two-byte length prefix; these capacities stop inside it or before the data.
    EXPECT_EQ(hpack_encoder::encode_literal("x", value, buffer, 4U, used), castle::status::full);
    EXPECT_EQ(hpack_encoder::encode_literal("x", value, buffer, 5U, used), castle::status::full);
    EXPECT_EQ(hpack_encoder::encode_literal("x", value, buffer, 6U, used), castle::status::full);
    EXPECT_EQ(hpack_encoder::encode_literal("x", string_view("abc"), buffer, 5U, used), castle::status::full);
}

TEST(Hpack, HuffmanStringEndingOnSymbolBoundaryAndTruncatedLength)
{
    std::string exact = huffman_authority(huffman("aaaaaaaa", "", '1'));
    basic_headers<2U, 16U, 16U> headers;
    hpack_decoder<> decoder;
    EXPECT_EQ(decoder.decode_block(reinterpret_cast<uint8_t const*>(exact.data()), exact.size(), headers), castle::status::ok);
    EXPECT_TRUE(ascii_iequal(headers.get(":authority"), "aaaaaaaa"));

    const uint8_t truncated[] = {0x00U, 0x01U, 'n', 0x05U, 'v'};
    basic_headers<2U, 16U, 16U> none;
    EXPECT_EQ(decoder.decode_block(truncated, sizeof(truncated), none), castle::status::data_loss);
}
