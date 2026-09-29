#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/nmea.hpp"

namespace
{

using namespace castle_ext::protocols::nmea;
using castle::container::string_view;

// ---------------------------------------------------------------------------
// message_view
// ---------------------------------------------------------------------------

TEST(NmeaProtocol, SentenceViewAccessors)
{
    string_view fields_data[3U] = {string_view("1"), string_view(""), string_view("3")};
    message_view view;
    view.talker = string_view("GP");
    view.type = string_view("GSA");
    view.fields = castle::container::array_view<CASTLE_CONST string_view>(fields_data, 3U);
    view.checksum = 0x7BU;
    view.checksum_present = true;

    EXPECT_EQ(view.field_count(), 3U);
    EXPECT_EQ(view.field(0U), string_view("1"));
    EXPECT_TRUE(view.field(10U).empty());
    EXPECT_TRUE(view.has_field(0U));
    EXPECT_FALSE(view.has_field(1U));
    EXPECT_FALSE(view.has_field(10U));
    EXPECT_TRUE(view.is(string_view("GSA")));
    EXPECT_FALSE(view.is(string_view("GGA")));
    EXPECT_TRUE(view.is_talker(string_view("GP")));
    EXPECT_FALSE(view.is_talker(string_view("GN")));
}

// ---------------------------------------------------------------------------
// checksum_accumulator
// ---------------------------------------------------------------------------

TEST(NmeaProtocol, ChecksumAccumulatorResetUpdateValue)
{
    checksum_accumulator accumulator;
    EXPECT_EQ(accumulator.value(), 0U);

    accumulator.update('A');
    accumulator.update('B');
    EXPECT_EQ(accumulator.value(), static_cast<uint8_t>('A' ^ 'B'));

    accumulator.reset();
    EXPECT_EQ(accumulator.value(), 0U);
}

// ---------------------------------------------------------------------------
// Hex helpers
// ---------------------------------------------------------------------------

TEST(NmeaProtocol, IsHexDigit)
{
    EXPECT_TRUE(is_hex_digit('0'));
    EXPECT_TRUE(is_hex_digit('9'));
    EXPECT_TRUE(is_hex_digit('A'));
    EXPECT_TRUE(is_hex_digit('F'));
    EXPECT_TRUE(is_hex_digit('a'));
    EXPECT_TRUE(is_hex_digit('f'));
    EXPECT_FALSE(is_hex_digit('g'));
    EXPECT_FALSE(is_hex_digit('$'));
}

TEST(NmeaProtocol, HexNibble)
{
    EXPECT_EQ(hex_nibble('0'), 0U);
    EXPECT_EQ(hex_nibble('9'), 9U);
    EXPECT_EQ(hex_nibble('A'), 10U);
    EXPECT_EQ(hex_nibble('F'), 15U);
    EXPECT_EQ(hex_nibble('a'), 10U);
    EXPECT_EQ(hex_nibble('f'), 15U);
}

TEST(NmeaProtocol, ParseHexByte)
{
    uint8_t out = 0U;
    EXPECT_FALSE(parse_hex_byte('g', '0', out));
    EXPECT_FALSE(parse_hex_byte('0', 'g', out));
    EXPECT_TRUE(parse_hex_byte('5', 'B', out));
    EXPECT_EQ(out, 0x5BU);
}

TEST(NmeaProtocol, WriteHexByte)
{
    char out[2U] = {0, 0};
    write_hex_byte(0x5AU, out);
    EXPECT_EQ(out[0U], '5');
    EXPECT_EQ(out[1U], 'A');

    write_hex_byte(0x00U, out);
    EXPECT_EQ(out[0U], '0');
    EXPECT_EQ(out[1U], '0');
}

// ---------------------------------------------------------------------------
// compute_checksum
// ---------------------------------------------------------------------------

TEST(NmeaProtocol, ComputeChecksumEmptyAndNonEmpty)
{
    EXPECT_EQ(compute_checksum(string_view()), 0U);
    EXPECT_EQ(compute_checksum(string_view("A")), static_cast<uint8_t>('A'));
}

// ---------------------------------------------------------------------------
// split_identifier
// ---------------------------------------------------------------------------

TEST(NmeaProtocol, SplitIdentifierEmpty)
{
    string_view talker;
    string_view type;
    split_identifier(string_view(), talker, type);
    EXPECT_TRUE(talker.empty());
    EXPECT_TRUE(type.empty());
}

TEST(NmeaProtocol, SplitIdentifierProprietary)
{
    string_view talker;
    string_view type;
    split_identifier(string_view("PUBX"), talker, type);
    EXPECT_EQ(talker, string_view("P"));
    EXPECT_EQ(type, string_view("UBX"));
}

TEST(NmeaProtocol, SplitIdentifierStandard)
{
    string_view talker;
    string_view type;
    split_identifier(string_view("GNGGA"), talker, type);
    EXPECT_EQ(talker, string_view("GN"));
    EXPECT_EQ(type, string_view("GGA"));
}

TEST(NmeaProtocol, SplitIdentifierTalkerLess)
{
    string_view talker;
    string_view type;
    split_identifier(string_view("GGA"), talker, type);
    EXPECT_TRUE(talker.empty());
    EXPECT_EQ(type, string_view("GGA"));
}

// ---------------------------------------------------------------------------
// tokenize_fields
// ---------------------------------------------------------------------------

TEST(NmeaProtocol, TokenizeFieldsEmptyContent)
{
    string_view storage[4U];
    string_view talker;
    string_view type;
    EXPECT_EQ(tokenize_fields(string_view(), storage, 4U, talker, type), 0U);
    EXPECT_TRUE(talker.empty());
    EXPECT_TRUE(type.empty());
}

TEST(NmeaProtocol, TokenizeFieldsNoFieldsPresent)
{
    string_view storage[4U];
    string_view talker;
    string_view type;
    EXPECT_EQ(tokenize_fields(string_view("GPTXT"), storage, 4U, talker, type), 0U);
    EXPECT_EQ(talker, string_view("GP"));
    EXPECT_EQ(type, string_view("TXT"));
}

TEST(NmeaProtocol, TokenizeFieldsRejectsNullStorage)
{
    string_view talker;
    string_view type;
    EXPECT_EQ(tokenize_fields(string_view("GPGSA,1,2"), nullptr, 4U, talker, type), 0U);
    EXPECT_EQ(talker, string_view("GP"));
    EXPECT_EQ(type, string_view("GSA"));
}

TEST(NmeaProtocol, TokenizeFieldsRejectsZeroCapacity)
{
    string_view storage[4U];
    string_view talker;
    string_view type;
    EXPECT_EQ(tokenize_fields(string_view("GPGSA,1,2"), storage, 0U, talker, type), 0U);
}

TEST(NmeaProtocol, TokenizeFieldsSplitsTrailingEmptyField)
{
    string_view storage[4U];
    string_view talker;
    string_view type;
    castle::size_type count = tokenize_fields(string_view("GPGGA,1,2,"), storage, 4U, talker, type);
    ASSERT_EQ(count, 3U);
    EXPECT_EQ(storage[0U], string_view("1"));
    EXPECT_EQ(storage[1U], string_view("2"));
    EXPECT_TRUE(storage[2U].empty());
}

TEST(NmeaProtocol, TokenizeFieldsTruncatesAtCapacity)
{
    string_view storage[2U];
    string_view talker;
    string_view type;
    castle::size_type count = tokenize_fields(string_view("GPGGA,1,2,3,4"), storage, 2U, talker, type);
    EXPECT_EQ(count, 2U);
    EXPECT_EQ(storage[0U], string_view("1"));
    EXPECT_EQ(storage[1U], string_view("2"));
}

// ---------------------------------------------------------------------------
// decode_sentence
// ---------------------------------------------------------------------------

TEST(NmeaProtocol, DecodeSentenceKnownGoodSample)
{
    string_view storage[16U];
    message_view view;
    bool ok = decode_sentence(
        string_view("$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,,*5B\r\n"),
        storage, 16U, view);
    ASSERT_TRUE(ok);
    EXPECT_EQ(view.talker, string_view("GP"));
    EXPECT_EQ(view.type, string_view("GGA"));
    EXPECT_EQ(view.field_count(), 14U);
    EXPECT_TRUE(view.checksum_present);
    EXPECT_EQ(view.checksum, 0x5BU);
}

TEST(NmeaProtocol, DecodeSentenceWithoutLeadingDollarOrTrailingTerminators)
{
    string_view storage[16U];
    message_view view;
    bool ok = decode_sentence(string_view("GPGSA,1,2*41"), storage, 16U, view);
    ASSERT_TRUE(ok);
    EXPECT_EQ(view.type, string_view("GSA"));
}

TEST(NmeaProtocol, DecodeSentenceWithoutChecksum)
{
    string_view storage[16U];
    message_view view;
    bool ok = decode_sentence(string_view("$GPTXT,1,1,01,OK\r\n"), storage, 16U, view);
    ASSERT_TRUE(ok);
    EXPECT_FALSE(view.checksum_present);
}

TEST(NmeaProtocol, DecodeSentenceRejectsShortChecksumField)
{
    string_view storage[16U];
    message_view view;
    EXPECT_FALSE(decode_sentence(string_view("$GPGSA,1*5"), storage, 16U, view));
}

TEST(NmeaProtocol, DecodeSentenceRejectsNonHexChecksum)
{
    string_view storage[16U];
    message_view view;
    EXPECT_FALSE(decode_sentence(string_view("$GPGSA,1*ZZ"), storage, 16U, view));
}

TEST(NmeaProtocol, DecodeSentenceRejectsEmptyType)
{
    string_view storage[16U];
    message_view view;
    EXPECT_FALSE(decode_sentence(string_view("$,1,2*00"), storage, 16U, view));
}

TEST(NmeaProtocol, DecodeSentenceRejectsChecksumMismatch)
{
    string_view storage[16U];
    message_view view;
    EXPECT_FALSE(decode_sentence(
        string_view("$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,,*00\r\n"),
        storage, 16U, view));
}

// ---------------------------------------------------------------------------
// encode_sentence
// ---------------------------------------------------------------------------

TEST(NmeaProtocol, EncodeSentenceRejectsEmptyType)
{
    char out[32U];
    castle::size_type written = 0U;
    castle::status result = encode_sentence(
        string_view("GP"), string_view(),
        castle::container::array_view<CASTLE_CONST string_view>(nullptr, 0U),
        out, sizeof(out), written);
    EXPECT_EQ(result, castle::status::invalid_argument);
}

TEST(NmeaProtocol, EncodeSentenceRejectsNullOutput)
{
    castle::size_type written = 0U;
    castle::status result = encode_sentence(
        string_view("GP"), string_view("GSA"),
        castle::container::array_view<CASTLE_CONST string_view>(nullptr, 0U),
        nullptr, 32U, written);
    EXPECT_EQ(result, castle::status::invalid_argument);
}

TEST(NmeaProtocol, EncodeSentenceRejectsUndersizedBuffer)
{
    string_view fields_data[1U] = {string_view("1")};
    char out[4U];
    castle::size_type written = 0U;
    castle::status result = encode_sentence(
        string_view("GP"), string_view("GSA"),
        castle::container::array_view<CASTLE_CONST string_view>(fields_data, 1U),
        out, sizeof(out), written);
    EXPECT_EQ(result, castle::status::full);
}

TEST(NmeaProtocol, EncodeSentenceRoundTripsWithEmptyTalkerAndNoFields)
{
    char out[32U];
    castle::size_type written = 0U;
    castle::status result = encode_sentence(
        string_view(), string_view("TXT"),
        castle::container::array_view<CASTLE_CONST string_view>(nullptr, 0U),
        out, sizeof(out), written);
    ASSERT_EQ(result, castle::status::ok);

    string_view storage[4U];
    message_view view;
    ASSERT_TRUE(decode_sentence(string_view(out, written), storage, 4U, view));
    EXPECT_TRUE(view.talker.empty());
    EXPECT_EQ(view.type, string_view("TXT"));
    EXPECT_EQ(view.field_count(), 0U);
}

TEST(NmeaProtocol, EncodeSentenceRoundTripsWithFields)
{
    string_view fields_data[2U] = {string_view("1"), string_view("08")};
    char out[32U];
    castle::size_type written = 0U;
    castle::status result = encode_sentence(
        string_view("GP"), string_view("GSA"),
        castle::container::array_view<CASTLE_CONST string_view>(fields_data, 2U),
        out, sizeof(out), written);
    ASSERT_EQ(result, castle::status::ok);

    string_view storage[4U];
    message_view view;
    ASSERT_TRUE(decode_sentence(string_view(out, written), storage, 4U, view));
    EXPECT_EQ(view.field_count(), 2U);
    EXPECT_EQ(view.field(1U), string_view("08"));
}

// ---------------------------------------------------------------------------
// Generic field decode helpers
// ---------------------------------------------------------------------------

TEST(NmeaProtocol, ParseDoubleBranches)
{
    double value = 0.0;
    EXPECT_FALSE(parse_double(string_view(), value));
    EXPECT_FALSE(parse_double(string_view("not-a-number"), value));
    EXPECT_TRUE(parse_double(string_view("123.456"), value));
    EXPECT_DOUBLE_EQ(value, 123.456);

    char oversized[40U];
    for (castle::size_type i = 0U; i < sizeof(oversized) - 1U; ++i)
    {
        oversized[i] = '1';
    }
    oversized[sizeof(oversized) - 1U] = '\0';
    EXPECT_FALSE(parse_double(string_view(oversized), value));
}

TEST(NmeaProtocol, ParseIntBranches)
{
    int value = 0;
    EXPECT_FALSE(parse_int(string_view(), value));
    EXPECT_FALSE(parse_int(string_view("xyz"), value));
    EXPECT_TRUE(parse_int(string_view("-42"), value));
    EXPECT_EQ(value, -42);
}

TEST(NmeaProtocol, ParseUintBranches)
{
    uint32_t value = 0U;
    EXPECT_FALSE(parse_uint(string_view(), value));
    EXPECT_FALSE(parse_uint(string_view("xyz"), value));
    EXPECT_TRUE(parse_uint(string_view("160223"), value));
    EXPECT_EQ(value, 160223U);
}

TEST(NmeaProtocol, ParseCharBranches)
{
    char value = '\0';
    EXPECT_FALSE(parse_char(string_view(), value));
    EXPECT_TRUE(parse_char(string_view("N"), value));
    EXPECT_EQ(value, 'N');
}

TEST(NmeaProtocol, ParseLatLonPropagatesFailure)
{
    double value = 0.0;
    EXPECT_FALSE(parse_latlon(string_view(), string_view("N"), value));
}

TEST(NmeaProtocol, ParseLatLonNorthEastNoSignFlip)
{
    double value = 0.0;
    ASSERT_TRUE(parse_latlon(string_view("4717.11399"), string_view("N"), value));
    EXPECT_GT(value, 0.0);
    ASSERT_TRUE(parse_latlon(string_view("00833.91590"), string_view("E"), value));
    EXPECT_GT(value, 0.0);
}

TEST(NmeaProtocol, ParseLatLonSouthWestSignFlip)
{
    double value = 0.0;
    ASSERT_TRUE(parse_latlon(string_view("4717.11399"), string_view("S"), value));
    EXPECT_LT(value, 0.0);
    ASSERT_TRUE(parse_latlon(string_view("00833.91590"), string_view("W"), value));
    EXPECT_LT(value, 0.0);
    ASSERT_TRUE(parse_latlon(string_view("4717.11399"), string_view("s"), value));
    EXPECT_LT(value, 0.0);
    ASSERT_TRUE(parse_latlon(string_view("00833.91590"), string_view("w"), value));
    EXPECT_LT(value, 0.0);
}

TEST(NmeaProtocol, ParseLatLonEmptyDirectionKeepsSign)
{
    double value = 0.0;
    ASSERT_TRUE(parse_latlon(string_view("4717.11399"), string_view(), value));
    EXPECT_GT(value, 0.0);
}

TEST(NmeaProtocol, ParseUtcTimeAndDate)
{
    double time_value = 0.0;
    EXPECT_TRUE(parse_utc_time(string_view("092725.00"), time_value));
    EXPECT_DOUBLE_EQ(time_value, 92725.00);

    uint32_t date_value = 0U;
    EXPECT_TRUE(parse_utc_date(string_view("160223"), date_value));
    EXPECT_EQ(date_value, 160223U);
}

// ---------------------------------------------------------------------------
// Encode/Decode Round-Trip Tests
// ---------------------------------------------------------------------------

TEST(NmeaProtocol, EncodeDecodeRoundTripKeepsChecksumAndFields)
{
    string_view fields_data[3U] = {string_view("1"), string_view(""), string_view("3")};
    castle::size_type written = 0U;
    char output[64U] = {};

    ASSERT_EQ(
        encode_sentence(
            string_view("GN"), string_view("GSA"),
            castle::container::array_view<CASTLE_CONST string_view>(fields_data, 3U),
            output, sizeof(output), written),
        castle::status::ok);

    string_view storage[4U];
    message_view view;
    ASSERT_TRUE(decode_sentence(string_view(output, written), storage, 4U, view));
    EXPECT_EQ(view.talker, string_view("GN"));
    EXPECT_EQ(view.type, string_view("GSA"));
    EXPECT_EQ(view.field_count(), 3U);
    EXPECT_TRUE(view.checksum_present);
    EXPECT_EQ(view.field(1U), string_view());
}

TEST(NmeaProtocol, DecodeRejectsMalformedChecksumSuffix)
{
    string_view storage[4U];
    message_view view;
    EXPECT_FALSE(decode_sentence(string_view("$GPGGA,1*0"), storage, 4U, view));
    EXPECT_FALSE(decode_sentence(string_view("$GPGGA,1*GG"), storage, 4U, view));
}

} // namespace
