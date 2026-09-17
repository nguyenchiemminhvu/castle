#include <gtest/gtest.h>

#include "castle/serialization/csv.hpp"
#include "castle/utility/move.hpp"

namespace
{

using castle::container::string;
using castle::container::string_view;
using castle::serialization::csv::document;
using castle::serialization::csv::error_code;
using castle::serialization::csv::options;
using castle::serialization::csv::parse;
using castle::serialization::csv::serialize;
using castle::serialization::csv::write;

TEST(CsvParseTest, EmptyBomAndBasicRows)
{
    document<8U, 8U, 32U> doc;

    EXPECT_TRUE(parse(doc, string_view()).succeeded());
    EXPECT_TRUE(doc.empty());

    const char bom_only[] = {static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF), '\0'};
    EXPECT_TRUE(parse(doc, string_view(bom_only)).succeeded());
    EXPECT_TRUE(doc.empty());

    EXPECT_TRUE(parse(doc, string_view("name,port\ncastle,1883\n")).succeeded());
    EXPECT_EQ(doc.rows(), 2U);
    EXPECT_EQ(doc.column_count(0U), 2U);
    EXPECT_EQ(doc.column_count(1U), 2U);
    EXPECT_EQ(doc.get(0U, 0U), string_view("name"));
    EXPECT_EQ(doc.get(1U, 1U), string_view("1883"));
    EXPECT_EQ(doc.max_column_count(), 2U);
}

TEST(CsvParseTest, EmptyFieldsTrailingDelimiterAndBlankRecords)
{
    document<8U, 8U, 16U> doc;
    ASSERT_TRUE(parse(doc, string_view(",b,\n\nlast,,tail")).succeeded());

    ASSERT_EQ(doc.rows(), 3U);
    EXPECT_EQ(doc.column_count(0U), 3U);
    EXPECT_TRUE(doc.get(0U, 0U).empty());
    EXPECT_EQ(doc.get(0U, 1U), string_view("b"));
    EXPECT_TRUE(doc.get(0U, 2U).empty());

    EXPECT_EQ(doc.column_count(1U), 1U);
    EXPECT_TRUE(doc.get(1U, 0U).empty());

    EXPECT_EQ(doc.column_count(2U), 3U);
    EXPECT_TRUE(doc.get(2U, 1U).empty());
}

TEST(CsvParseTest, QuotingEscapedQuotesAndMultilineFields)
{
    document<8U, 8U, 64U> doc;
    const string_view source(
        "a,\"b,c\",\"quoted \"\"value\"\"\"\n"
        "\"line1\r\nline2\",tail\r\n"
        "last,\"\"\n");

    const auto result = parse(doc, source);
    ASSERT_TRUE(result.succeeded());
    ASSERT_EQ(doc.rows(), 3U);

    EXPECT_EQ(doc.get(0U, 0U), string_view("a"));
    EXPECT_EQ(doc.get(0U, 1U), string_view("b,c"));
    EXPECT_EQ(doc.get(0U, 2U), string_view("quoted \"value\""));
    EXPECT_EQ(doc.get(1U, 0U), string_view("line1\r\nline2"));
    EXPECT_EQ(doc.get(1U, 1U), string_view("tail"));
    EXPECT_TRUE(doc.get(2U, 1U).empty());
}

TEST(CsvParseTest, SingleCarriageReturnLineEndings)
{
    document<4U, 4U, 16U> doc;
    ASSERT_TRUE(parse(doc, string_view("a,b\rc,d\r")).succeeded());
    ASSERT_EQ(doc.rows(), 2U);
    EXPECT_EQ(doc.get(1U, 0U), string_view("c"));
    EXPECT_EQ(doc.get(1U, 1U), string_view("d"));
}

TEST(CsvParseTest, StrictColumnMode)
{
    document<8U, 8U, 16U> doc;
    options parse_options;
    parse_options.strict_columns = true;

    EXPECT_TRUE(parse(doc, string_view("a,b\n1,2\n3,4\n"), parse_options).succeeded());
    EXPECT_EQ(doc.rows(), 3U);

    const auto result = parse(doc, string_view("a,b\n1\n"), parse_options);
    EXPECT_EQ(result.status, castle::status::invalid_argument);
    EXPECT_EQ(result.code, error_code::inconsistent_columns);
    EXPECT_TRUE(doc.empty());
}

TEST(CsvParseTest, InvalidSyntaxReportsLocationAndClearsDocument)
{
    struct invalid_case
    {
        string_view input;
        error_code code;
        castle::size_type line;
        castle::size_type column;
    };

    const invalid_case cases[] = {
        {string_view("a,\"unterminated"), error_code::unexpected_end, 1U, 16U},
        {string_view("a,b\n\"broken"), error_code::unexpected_end, 2U, 8U},
        {string_view("a,b\n1,2x\"3\n"), error_code::invalid_quote, 2U, 5U},
        {string_view("a,\"ok\"tail\n"), error_code::invalid_quote, 1U, 7U},
        {string_view("a\0b\n", 4U), error_code::invalid_character, 1U, 2U},
    };

    for (const auto& test_case : cases)
    {
        document<8U, 8U, 32U> doc;
        const auto result = parse(doc, test_case.input);
        EXPECT_TRUE(result.status == castle::status::invalid_argument);
        EXPECT_TRUE(result.code == test_case.code);
        EXPECT_TRUE(result.line == test_case.line);
        EXPECT_TRUE(result.column == test_case.column);
        EXPECT_TRUE(doc.empty());
    }
}

TEST(CsvParseTest, InvalidDelimiterIsRejected)
{
    document<4U, 4U, 16U> doc;

    options invalid;
    invalid.delimiter = '"';
    auto result = parse(doc, string_view("a,b\n"), invalid);
    EXPECT_EQ(result.status, castle::status::invalid_argument);
    EXPECT_EQ(result.code, error_code::invalid_delimiter);
    EXPECT_TRUE(doc.empty());

    invalid.delimiter = '\n';
    result = parse(doc, string_view("a,b\n"), invalid);
    EXPECT_EQ(result.code, error_code::invalid_delimiter);

    invalid.delimiter = '\0';
    result = parse(doc, string_view("a,b\n"), invalid);
    EXPECT_EQ(result.code, error_code::invalid_delimiter);
}

TEST(CsvParseTest, RowAndFieldCapacityFailuresClearDocument)
{
    {
        document<1U, 4U, 8U> doc;
        const auto result = parse(doc, string_view("a\nb\n"));
        EXPECT_EQ(result.status, castle::status::full);
        EXPECT_EQ(result.code, error_code::capacity);
        EXPECT_TRUE(doc.empty());
    }

    {
        document<4U, 2U, 8U> doc;
        const auto result = parse(doc, string_view("a,b,c\n"));
        EXPECT_EQ(result.status, castle::status::full);
        EXPECT_EQ(result.code, error_code::capacity);
        EXPECT_TRUE(doc.empty());
    }

    {
        document<4U, 4U, 3U> doc;
        const auto result = parse(doc, string_view("abcd\n"));
        EXPECT_EQ(result.status, castle::status::full);
        EXPECT_EQ(result.code, error_code::capacity);
        EXPECT_TRUE(doc.empty());
    }
}

TEST(CsvDocumentTest, ConstructionMutationLookupAndBounds)
{
    document<4U, 4U, 16U> doc;
    castle::size_type row = document<4U, 4U, 16U>::npos;

    EXPECT_EQ(doc.add_row(row), castle::status::ok);
    EXPECT_EQ(row, 0U);
    EXPECT_EQ(doc.append(row, string_view("device")), castle::status::ok);
    EXPECT_EQ(doc.append(row, string_view("castle")), castle::status::ok);
    EXPECT_EQ(doc.append_character(row, 1U, '!'), castle::status::ok);
    EXPECT_EQ(doc.get(0U, 1U), string_view("castle!"));
    EXPECT_EQ(doc.set(row, 1U, string_view("castle-2")), castle::status::ok);
    EXPECT_EQ(doc.get(0U, 1U), string_view("castle-2"));

    EXPECT_EQ(doc.set(0U, 3U, string_view("invalid-gap")), castle::status::out_of_range);
    EXPECT_EQ(doc.set(8U, 0U, string_view("invalid-row")), castle::status::out_of_range);

    string_view value;
    EXPECT_EQ(doc.read(0U, 0U, value), castle::status::ok);
    EXPECT_EQ(value, string_view("device"));
    EXPECT_EQ(doc.read(0U, 9U, value), castle::status::out_of_range);
    EXPECT_EQ(doc.read(9U, 0U, value), castle::status::out_of_range);
    EXPECT_TRUE(value.empty());
    EXPECT_TRUE(doc.get(9U, 0U).empty());

    document<1U, 1U, 8U> one_row;
    EXPECT_EQ(one_row.add_row(row), castle::status::ok);
    EXPECT_EQ(one_row.add_row(row), castle::status::full);
    EXPECT_EQ(row, (document<1U, 1U, 8U>::npos));

    EXPECT_EQ(doc.append(0U, string_view("other")), castle::status::ok);
    EXPECT_EQ(doc.append(0U, string_view("overflow-column")), castle::status::ok);
}

TEST(CsvDocumentTest, AdditionalMutationErrorsAndNumericBoundaries)
{
    document<4U, 3U, 4U> doc;
    castle::size_type row = 0U;
    ASSERT_EQ(doc.add_row(row), castle::status::ok);

    EXPECT_EQ(doc.append(99U, string_view("bad-row")), castle::status::out_of_range);
    EXPECT_EQ(doc.set(99U, 0U, string_view("bad-row")), castle::status::out_of_range);
    EXPECT_EQ(doc.set(row, 2U, string_view("gap")), castle::status::out_of_range);
    EXPECT_EQ(doc.row_full(99U), false);

    EXPECT_EQ(doc.append(row, string_view("a")), castle::status::ok);
    EXPECT_EQ(doc.append(row, string_view("b")), castle::status::ok);
    EXPECT_EQ(doc.set(row, 1U, string_view("12345")), castle::status::full);
    EXPECT_EQ(doc.get(row, 1U), string_view("b"));

    EXPECT_EQ(doc.set_integer(row, 2U, static_cast<int32_t>(-2147483648LL)), castle::status::full);

    document<4U, 4U, 32U> numeric;
    ASSERT_EQ(numeric.add_row(row), castle::status::ok);
    EXPECT_EQ(numeric.set_integer(row, 0U, static_cast<int32_t>(-2147483648LL)), castle::status::ok);
    EXPECT_EQ(numeric.set_integer(row, 1U, static_cast<uint32_t>(4294967295UL)), castle::status::ok);

    int32_t signed_value = 0;
    uint32_t unsigned_value = 0U;
    EXPECT_EQ(numeric.get_integer(row, 0U, signed_value), castle::status::ok);
    EXPECT_EQ(signed_value, static_cast<int32_t>(-2147483648LL));
    EXPECT_EQ(numeric.get_integer(row, 1U, unsigned_value), castle::status::ok);
    EXPECT_EQ(unsigned_value, 4294967295UL);

    EXPECT_EQ(numeric.set(row, 2U, string_view("2147483648")), castle::status::ok);
    EXPECT_EQ(numeric.get_integer(row, 2U, signed_value), castle::status::out_of_range);
    EXPECT_EQ(numeric.set(row, 2U, string_view("-2147483649")), castle::status::ok);
    EXPECT_EQ(numeric.get_integer(row, 2U, signed_value), castle::status::out_of_range);
    EXPECT_EQ(numeric.set(row, 2U, string_view("184467440737095516160")), castle::status::ok);
    EXPECT_EQ(numeric.get_integer(row, 2U, unsigned_value), castle::status::out_of_range);
    EXPECT_EQ(numeric.set(row, 2U, string_view("+7")), castle::status::ok);
    EXPECT_EQ(numeric.get_integer(row, 2U, signed_value), castle::status::ok);
    EXPECT_EQ(signed_value, 7);

    EXPECT_EQ(numeric.set(row, 3U, string_view("+1.5")), castle::status::ok);
    double floating_value = 0.0;
    EXPECT_EQ(numeric.get_floating(row, 3U, floating_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(floating_value, 1.5);
    EXPECT_EQ(numeric.set(row, 3U, string_view("1e1024")), castle::status::ok);
    EXPECT_EQ(numeric.get_floating(row, 3U, floating_value), castle::status::out_of_range);
}

TEST(CsvConversionTest, FloatingSerializationSpecialCases)
{
    document<4U, 4U, 32U> doc;
    castle::size_type row = 0U;
    ASSERT_EQ(doc.add_row(row), castle::status::ok);

    EXPECT_EQ(doc.set_floating(row, 0U, 1.9999, 3U), castle::status::ok);
    EXPECT_EQ(doc.get(row, 0U), string_view("2.000"));
    EXPECT_EQ(doc.set_floating(row, 1U, 1.23456789, 12U), castle::status::ok);
    EXPECT_EQ(doc.get(row, 1U), string_view("1.234567890"));
    EXPECT_EQ(doc.set_floating(row, 2U, -0.5, 0U), castle::status::ok);
    EXPECT_EQ(doc.get(row, 2U), string_view("-1"));

    const double nan_value = 0.0 / 0.0;
    const double infinity_value = 1.0 / 0.0;
    EXPECT_EQ(doc.set_floating(row, 3U, nan_value), castle::status::invalid_argument);
    EXPECT_EQ(doc.set_floating(row, 3U, infinity_value), castle::status::invalid_argument);

    document<1U, 2U, 4U> small;
    ASSERT_EQ(small.add_row(row), castle::status::ok);
    EXPECT_EQ(small.set_floating(row, 0U, 123.4, 1U), castle::status::full);
    EXPECT_EQ(small.set_integer(row, 0U, static_cast<int32_t>(12345)), castle::status::full);
}

TEST(CsvConversionTest, BooleanIntegerEnumAndFloatingPoint)
{
    document<8U, 8U, 32U> doc;
    castle::size_type row = 0U;
    ASSERT_EQ(doc.add_row(row), castle::status::ok);

    EXPECT_EQ(doc.set_bool(row, 0U, true), castle::status::ok);
    EXPECT_EQ(doc.set_bool(row, 1U, false), castle::status::ok);
    EXPECT_EQ(doc.set_integer(row, 2U, static_cast<int32_t>(-42)), castle::status::ok);
    EXPECT_EQ(doc.set_integer(row, 3U, static_cast<uint32_t>(42U)), castle::status::ok);
    EXPECT_EQ(doc.set_floating(row, 4U, -1.25F, 3U), castle::status::ok);
    EXPECT_EQ(doc.set_floating(row, 5U, 31.25, 2U), castle::status::ok);

    bool bool_value = false;
    int32_t signed_value = 0;
    uint32_t unsigned_value = 0U;
    float float_value = 0.0F;
    double double_value = 0.0;

    EXPECT_EQ(doc.get_bool(row, 0U, bool_value), castle::status::ok);
    EXPECT_TRUE(bool_value);
    EXPECT_EQ(doc.get_bool(row, 1U, bool_value), castle::status::ok);
    EXPECT_FALSE(bool_value);
    EXPECT_EQ(doc.get_integer(row, 2U, signed_value), castle::status::ok);
    EXPECT_EQ(signed_value, -42);
    EXPECT_EQ(doc.get_integer(row, 3U, unsigned_value), castle::status::ok);
    EXPECT_EQ(unsigned_value, 42U);
    EXPECT_EQ(doc.get_floating(row, 4U, float_value), castle::status::ok);
    EXPECT_FLOAT_EQ(float_value, -1.25F);
    EXPECT_EQ(doc.get_floating(row, 5U, double_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(double_value, 31.25);

    enum class mode : uint8_t
    {
        safe = 0U,
        fast = 1U
    };
    EXPECT_EQ(doc.set_enum(row, 6U, mode::fast), castle::status::ok);
    mode current = mode::safe;
    EXPECT_EQ(doc.get_enum(row, 6U, current), castle::status::ok);
    EXPECT_EQ(current, mode::fast);

    EXPECT_EQ(doc.set(row, 7U, string_view("TRUE")), castle::status::ok);
    EXPECT_EQ(doc.get_bool(row, 7U, bool_value), castle::status::ok);
    EXPECT_TRUE(bool_value);
    EXPECT_EQ(doc.set(row, 0U, string_view("maybe")), castle::status::ok);
    EXPECT_EQ(doc.get_bool(row, 0U, bool_value), castle::status::invalid_argument);

    EXPECT_EQ(doc.set(row, 0U, string_view("999999999999999999999999")), castle::status::ok);
    EXPECT_EQ(doc.get_integer(row, 0U, signed_value), castle::status::out_of_range);
    EXPECT_EQ(doc.set(row, 1U, string_view("-1")), castle::status::ok);
    EXPECT_EQ(doc.get_integer(row, 1U, unsigned_value), castle::status::invalid_argument);
    EXPECT_EQ(doc.set(row, 2U, string_view("12x")), castle::status::ok);
    EXPECT_EQ(doc.get_integer(row, 2U, signed_value), castle::status::invalid_argument);

    EXPECT_EQ(doc.set(row, 3U, string_view("1e")), castle::status::ok);
    EXPECT_EQ(doc.get_floating(row, 3U, double_value), castle::status::invalid_argument);
    EXPECT_EQ(doc.set(row, 3U, string_view("1e1025")), castle::status::ok);
    EXPECT_EQ(doc.get_floating(row, 3U, double_value), castle::status::out_of_range);
    EXPECT_EQ(doc.set(row, 3U, string_view(".")), castle::status::ok);
    EXPECT_EQ(doc.get_floating(row, 3U, double_value), castle::status::invalid_argument);
    EXPECT_EQ(doc.set(row, 3U, string_view("1.2x")), castle::status::ok);
    EXPECT_EQ(doc.get_floating(row, 3U, double_value), castle::status::invalid_argument);
    EXPECT_EQ(doc.set(row, 3U, string_view("3.125e+1")), castle::status::ok);
    EXPECT_EQ(doc.get_floating(row, 3U, double_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(double_value, 31.25);
    EXPECT_EQ(doc.set(row, 3U, string_view("4e-2")), castle::status::ok);
    EXPECT_EQ(doc.get_floating(row, 3U, double_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(double_value, 0.04);

    EXPECT_EQ(doc.get_bool(row, 99U, bool_value), castle::status::out_of_range);
    EXPECT_EQ(doc.get_integer(row, 99U, signed_value), castle::status::out_of_range);
    EXPECT_EQ(doc.get_floating(row, 99U, double_value), castle::status::out_of_range);
}



TEST(CsvDetailTest, NumericAndFieldFormattingFailureBranches)
{
    using namespace castle::serialization::csv::detail;

    int32_t signed_value = 0;
    uint32_t unsigned_value = 0U;
    double floating_value = 0.0;

    EXPECT_EQ(parse_integer(string_view(), signed_value), castle::status::invalid_argument);
    EXPECT_EQ(parse_integer(string_view("-"), signed_value), castle::status::invalid_argument);
    EXPECT_EQ(parse_integer(string_view("-1"), unsigned_value), castle::status::invalid_argument);
    EXPECT_EQ(parse_integer(string_view("12x"), signed_value), castle::status::invalid_argument);
    EXPECT_EQ(parse_integer(string_view("999999999999999999999999999999999999999999"), unsigned_value),
              castle::status::out_of_range);
    EXPECT_EQ(parse_integer(string_view("2147483648"), signed_value), castle::status::out_of_range);
    EXPECT_EQ(parse_integer(string_view("-2147483649"), signed_value), castle::status::out_of_range);
    EXPECT_EQ(parse_integer(string_view("-2147483648"), signed_value), castle::status::ok);
    EXPECT_EQ(signed_value, static_cast<int32_t>(-2147483648LL));
    EXPECT_EQ(parse_integer(string_view("-7"), signed_value), castle::status::ok);
    EXPECT_EQ(signed_value, -7);

    EXPECT_EQ(parse_floating(string_view(), floating_value), castle::status::invalid_argument);
    EXPECT_EQ(parse_floating(string_view("+3.5"), floating_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(floating_value, 3.5);
    EXPECT_EQ(parse_floating(string_view("."), floating_value), castle::status::invalid_argument);
    EXPECT_EQ(parse_floating(string_view("1e"), floating_value), castle::status::invalid_argument);
    EXPECT_EQ(parse_floating(string_view("1e+3"), floating_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(floating_value, 1000.0);
    EXPECT_EQ(parse_floating(string_view("1e-3"), floating_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(floating_value, 0.001);
    EXPECT_EQ(parse_floating(string_view("1e1025"), floating_value), castle::status::out_of_range);
    EXPECT_EQ(parse_floating(string_view("1e100000"), floating_value), castle::status::out_of_range);
    EXPECT_EQ(parse_floating(string_view("1.2x"), floating_value), castle::status::invalid_argument);

    castle::container::string<1U> tiny_integer;
    EXPECT_EQ(format_integer(static_cast<int32_t>(-12), tiny_integer), castle::status::full);
    EXPECT_EQ(tiny_integer.view(), string_view("-"));

    castle::container::string<1U> tiny_float;
    EXPECT_EQ(format_floating(-1.0, tiny_float, 0U), castle::status::full);
    EXPECT_EQ(tiny_float.view(), string_view("-"));

    castle::container::string<2U> closing_quote_full;
    EXPECT_EQ(append_serialized_field(string_view(","), ',', closing_quote_full), castle::status::full);
    EXPECT_EQ(closing_quote_full.view(), string_view("\","));

    castle::container::string<2U> escaped_quote_full;
    EXPECT_EQ(append_serialized_field(string_view("\""), ',', escaped_quote_full), castle::status::full);
    EXPECT_EQ(escaped_quote_full.view(), string_view("\""));

    castle::container::string<16U> direct_output;
    EXPECT_EQ(append_serialized_field(string_view("plain"), ',', direct_output), castle::status::ok);
    EXPECT_EQ(append_serialized_field(string_view("line\nfeed"), ',', direct_output), castle::status::ok);
    const char bad[] = {'a', '\0', 'b'};
    EXPECT_EQ(append_serialized_field(string_view(bad, 3U), ',', direct_output), castle::status::invalid_argument);
}

TEST(CsvSerializeTest, EmptyDocumentProducesEmptyOutput)
{
    document<4U, 4U, 16U> doc;
    castle::container::string<16U> output;
    ASSERT_TRUE(serialize(doc, output).succeeded());
    EXPECT_TRUE(output.empty());
}

TEST(CsvSerializeTest, CanonicalOutputEscapesQuotesCommasAndNewlines)
{
    document<8U, 8U, 64U> doc;
    castle::size_type row = 0U;
    ASSERT_EQ(doc.add_row(row), castle::status::ok);
    ASSERT_EQ(doc.append(row, string_view("plain")), castle::status::ok);
    ASSERT_EQ(doc.append(row, string_view("hello, world")), castle::status::ok);
    ASSERT_EQ(doc.append(row, string_view("quote \" here")), castle::status::ok);
    ASSERT_EQ(doc.append(row, string_view("line1\nline2")), castle::status::ok);
    ASSERT_EQ(doc.append(row, string_view() ), castle::status::ok);

    castle::container::string<256U> output;
    const auto result = serialize(doc, output);
    ASSERT_TRUE(result.succeeded());
    EXPECT_EQ(output.view(), string_view("plain,\"hello, world\",\"quote \"\" here\",\"line1\nline2\",\n"));

    document<8U, 8U, 64U> round_trip;
    ASSERT_TRUE(parse(round_trip, output.view()).succeeded());
    ASSERT_EQ(round_trip.rows(), 1U);
    ASSERT_EQ(round_trip.column_count(0U), 5U);
    EXPECT_EQ(round_trip.get(0U, 2U), string_view("quote \" here"));
    EXPECT_EQ(round_trip.get(0U, 3U), string_view("line1\nline2"));
    EXPECT_TRUE(round_trip.get(0U, 4U).empty());
}

TEST(CsvSerializeTest, CustomDelimiterAndWriteAlias)
{
    document<4U, 4U, 32U> doc;
    castle::size_type row = 0U;
    ASSERT_EQ(doc.add_row(row), castle::status::ok);
    ASSERT_EQ(doc.append(row, string_view("a;b")), castle::status::ok);
    ASSERT_EQ(doc.append(row, string_view("c")), castle::status::ok);

    options csv_options;
    csv_options.delimiter = ';';

    castle::container::string<64U> output;
    ASSERT_TRUE(write(doc, output, csv_options).succeeded());
    EXPECT_EQ(output.view(), string_view("\"a;b\";c\n"));

    document<4U, 4U, 32U> round_trip;
    ASSERT_TRUE(parse(round_trip, output.view(), csv_options).succeeded());
    EXPECT_EQ(round_trip.get(0U, 0U), string_view("a;b"));
    EXPECT_EQ(round_trip.get(0U, 1U), string_view("c"));
}

TEST(CsvSerializeTest, InvalidDelimiterAndOutputCapacity)
{
    document<4U, 4U, 16U> doc;
    castle::size_type row = 0U;
    ASSERT_EQ(doc.add_row(row), castle::status::ok);
    ASSERT_EQ(doc.append(row, string_view("value,value")), castle::status::ok);

    options invalid;
    invalid.delimiter = '\n';
    castle::container::string<64U> output;
    auto result = serialize(doc, output, invalid);
    EXPECT_EQ(result.status, castle::status::invalid_argument);
    EXPECT_EQ(result.code, error_code::invalid_delimiter);
    EXPECT_TRUE(output.empty());

    castle::container::string<3U> small_output;
    result = serialize(doc, small_output);
    EXPECT_EQ(result.status, castle::status::full);
    EXPECT_EQ(result.code, error_code::output_full);
    EXPECT_EQ(small_output.view(), string_view("\"va"));
}

TEST(CsvSerializeTest, CopyMoveAndClear)
{
    document<4U, 4U, 16U> original;
    castle::size_type row = 0U;
    ASSERT_EQ(original.add_row(row), castle::status::ok);
    ASSERT_EQ(original.append(row, string_view("copy")), castle::status::ok);

    document<4U, 4U, 16U> copy(original);
    EXPECT_EQ(copy.get(0U, 0U), string_view("copy"));

    document<4U, 4U, 16U> moved(castle::move(copy));
    EXPECT_EQ(moved.get(0U, 0U), string_view("copy"));

    moved.clear();
    EXPECT_TRUE(moved.empty());
}

TEST(CsvDocumentTest, NullAndCapacityValidation)
{
    document<4U, 4U, 4U> doc;
    castle::size_type row = 0U;
    ASSERT_EQ(doc.add_row(row), castle::status::ok);
    ASSERT_EQ(doc.append(row, string_view("ok")), castle::status::ok);

    const char bad[] = {'a', '\0', 'b'};
    EXPECT_EQ(doc.append(row, string_view(bad, 3U)), castle::status::invalid_argument);
    EXPECT_EQ(doc.set(row, 0U, string_view(bad, 3U)), castle::status::invalid_argument);
    EXPECT_EQ(doc.get(row, 0U), string_view("ok"));

    EXPECT_EQ(doc.append_character(row, 0U, '\0'), castle::status::invalid_argument);
    EXPECT_EQ(doc.append_character(row, 9U, 'x'), castle::status::out_of_range);
    EXPECT_EQ(doc.append_character(9U, 0U, 'x'), castle::status::out_of_range);
    EXPECT_EQ(doc.append(row, string_view("12345")), castle::status::full);
    EXPECT_EQ(doc.column_count(row), 1U);

    document<1U, 2U, 4U> two;
    ASSERT_EQ(two.add_row(row), castle::status::ok);
    EXPECT_EQ(two.append(row, string_view("a")), castle::status::ok);
    EXPECT_EQ(two.append(row, string_view("b")), castle::status::ok);
    EXPECT_EQ(two.append(row, string_view("c")), castle::status::full);
    EXPECT_TRUE(two.row_full(row));
}

TEST(CsvParseTest, RowMutationAndNumericBoundaryPaths)
{
    using document = castle::serialization::csv::document<2U, 3U, 8U>;

    document csv;
    document::size_type row_id = 0U;
    ASSERT_EQ(csv.add_row(row_id), castle::status::ok);
    EXPECT_EQ(csv.append_character(row_id, 0U, 'x'), castle::status::out_of_range);
    EXPECT_EQ(csv.append(row_id, "one"), castle::status::ok);
    EXPECT_EQ(csv.append_character(row_id, 0U, '2'), castle::status::ok);
    EXPECT_EQ(csv.get(row_id, 0U), string_view("one2"));
    EXPECT_EQ(csv.set(row_id, 4U, "bad"), castle::status::out_of_range);
    const char bad[] = {'a', '\0', 'b'};
    EXPECT_EQ(csv.set(row_id, 0U, string_view(bad, 3U)), castle::status::invalid_argument);
    EXPECT_EQ(csv.set(row_id, 1U, "two"), castle::status::ok);
    EXPECT_EQ(csv.set(row_id, 1U, "TWO"), castle::status::ok);
    string_view value;
    EXPECT_EQ(csv.read(row_id, 1U, value), castle::status::ok);
    EXPECT_EQ(value, string_view("TWO"));
    EXPECT_EQ(csv.read(row_id, 8U, value), castle::status::out_of_range);

    int16_t signed_value = 0;
    EXPECT_EQ(csv.set(row_id, 0U, "65535"), castle::status::ok);
    EXPECT_EQ(csv.get_integer(row_id, 0U, signed_value), castle::status::out_of_range);
    EXPECT_EQ(csv.set(row_id, 0U, "-32768"), castle::status::ok);
    EXPECT_EQ(csv.get_integer(row_id, 0U, signed_value), castle::status::ok);
    EXPECT_EQ(signed_value, static_cast<int16_t>(-32768));
    EXPECT_EQ(csv.set(row_id, 0U, "-32769"), castle::status::ok);
    EXPECT_EQ(csv.get_integer(row_id, 0U, signed_value), castle::status::out_of_range);
}

TEST(CsvParseTest, QuotedParserLineEndingAndQuoteClosedPaths)
{
    using document = castle::serialization::csv::document<8U, 4U, 16U>;

    document doc;
    EXPECT_TRUE(castle::serialization::csv::parse(
        doc, string_view("\"a\r\nb\",c\r\n\"q\"\"u\",z\n")).succeeded());
    EXPECT_EQ(doc.get(0U, 0U), string_view("a\r\nb"));
    EXPECT_EQ(doc.get(0U, 0U).size(), 4U);
    EXPECT_EQ(doc.get(0U, 1U), string_view("c"));
    EXPECT_EQ(doc.get(1U, 0U), string_view("q\"u"));
    EXPECT_EQ(doc.get(1U, 1U), string_view("z"));

    document malformed;
    const auto result = castle::serialization::csv::parse(malformed, string_view("\"a\"x\n"));
    EXPECT_EQ(result.status, castle::status::invalid_argument);
    EXPECT_EQ(result.code, castle::serialization::csv::error_code::invalid_quote);

    using tight_document = castle::serialization::csv::document<2U, 2U, 1U>;
    tight_document tight;
    const auto capacity = castle::serialization::csv::parse(tight, string_view("\"\r\n\"\n"));
    EXPECT_EQ(capacity.status, castle::status::full);
    EXPECT_EQ(capacity.code, castle::serialization::csv::error_code::capacity);
}

TEST(CsvParseTest, FloatingFormattingRoundingPrecisionAndOutputEdges)
{
    string<32U> output;
    EXPECT_EQ(castle::serialization::csv::detail::format_floating(9.9999, output, 3U), castle::status::ok);
    EXPECT_EQ(output.view(), string_view("10.000"));
    EXPECT_EQ(castle::serialization::csv::detail::format_floating(1.2345678901, output, 12U), castle::status::ok);
    EXPECT_EQ(output.view(), string_view("1.234567890"));

    string<2U> negative_tight;
    EXPECT_EQ(castle::serialization::csv::detail::format_floating(-12.3, negative_tight, 0U), castle::status::full);

    string<5U> exact_integer;
    EXPECT_EQ(castle::serialization::csv::detail::format_floating(12.34, exact_integer, 2U), castle::status::ok);
    EXPECT_EQ(exact_integer.view(), string_view("12.34"));
    string<5U> newline_fail;
    EXPECT_EQ(castle::serialization::csv::detail::format_floating(12.34, newline_fail, 3U), castle::status::full);
}

TEST(CsvParseTest, SerializationDelimiterAndTrailingNewlineFailures)
{
    using document = castle::serialization::csv::document<2U, 3U, 8U>;
    document doc;
    document::size_type row = 0U;
    ASSERT_EQ(doc.add_row(row), castle::status::ok);
    ASSERT_EQ(doc.append(row, "a"), castle::status::ok);
    ASSERT_EQ(doc.append(row, "b"), castle::status::ok);

    string<1U> delimiter_fail;
    const auto delimiter_result = castle::serialization::csv::serialize(doc, delimiter_fail);
    EXPECT_EQ(delimiter_result.status, castle::status::full);
    EXPECT_EQ(delimiter_result.code, castle::serialization::csv::error_code::output_full);

    document single;
    ASSERT_EQ(single.add_row(row), castle::status::ok);
    ASSERT_EQ(single.append(row, "abcd"), castle::status::ok);
    string<4U> newline_fail;
    const auto newline_result = castle::serialization::csv::serialize(single, newline_fail);
    EXPECT_EQ(newline_result.status, castle::status::full);
    EXPECT_EQ(newline_result.code, castle::serialization::csv::error_code::output_full);
}

TEST(CsvParseTest, ConversionParserAndFieldCapacityEdges)
{
    using namespace castle::serialization::csv;
    using document = castle::serialization::csv::document<4U, 1U, 16U>;

    document doc;
    document::size_type row = 0U;
    ASSERT_EQ(doc.add_row(row), castle::status::ok);
    ASSERT_EQ(doc.append(row, "2"), castle::status::ok);

    enum class mode : int32_t { off = 0, on = 2 };
    mode selected = mode::off;
    double floating = 0.0;
    EXPECT_EQ(doc.get_enum(row, 0U, selected), castle::status::ok);
    EXPECT_EQ(selected, mode::on);
    EXPECT_EQ(doc.get_floating(row, 0U, floating), castle::status::ok);
    EXPECT_DOUBLE_EQ(floating, 2.0);
    EXPECT_EQ(doc.append(row, "second"), castle::status::full);

    EXPECT_EQ(detail::parse_floating(string_view("1e309"), floating), castle::status::out_of_range);
    int32_t integer = 0;
    EXPECT_EQ(detail::parse_floating(string_view("12.3x"), floating), castle::status::invalid_argument);
    EXPECT_EQ(detail::parse_integer(string_view("12x"), integer), castle::status::invalid_argument);

    string<2U> decimal_tight;
    EXPECT_EQ(detail::format_floating(12.3, decimal_tight, 1U), castle::status::full);

    // castle::container::string requires MaxCapacity > 0, so the negative-sign
    // push_full branch in format_floating cannot be exercised through a valid
    // string instantiation without changing the frozen container API.

    string<1U> quoted_tight;
    EXPECT_EQ(detail::append_serialized_field(string_view("a,b"), ',', quoted_tight), castle::status::full);
    string<2U> quoted_close_tight;
    EXPECT_EQ(detail::append_serialized_field(string_view(","), ',', quoted_close_tight), castle::status::full);
    string<2U> quoted_inner_tight;
    EXPECT_EQ(detail::append_serialized_field(string_view("\""), ',', quoted_inner_tight), castle::status::full);

    document malformed;
    const char embedded_nul[] = {'a', '\0', 'b'};
    const auto parsed = parse(malformed, string_view(embedded_nul, 3U));
    EXPECT_EQ(parsed.status, castle::status::invalid_argument);
    EXPECT_EQ(parsed.code, error_code::invalid_character);
}

} // namespace
