#include <gtest/gtest.h>

#include "castle/serialization/ini.hpp"
#include "castle/utility/move.hpp"

#include <stdint.h>

namespace
{

using castle::container::string;
using castle::container::string_view;
using castle::serialization::ini::document;
using castle::serialization::ini::error_code;
using castle::serialization::ini::parse;
using castle::serialization::ini::serialize;

TEST(IniDocumentTest, DefaultStateAndSetGet)
{
    document<4U, 16U, 16U, 32U> doc;

    EXPECT_TRUE(doc.empty());
    EXPECT_FALSE(doc.full());
    EXPECT_EQ(doc.size(), 0U);
    EXPECT_EQ(doc.capacity(), 4U);
    EXPECT_EQ(doc.get("missing").has_value(), false);

    EXPECT_EQ(doc.set("mode", "safe"), castle::status::ok);
    EXPECT_EQ(doc.size(), 1U);
    EXPECT_TRUE(doc.contains("mode"));
    EXPECT_FALSE(doc.contains_section("network"));

    auto found = doc.get("mode");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), string_view("safe"));

    string_view output;
    EXPECT_EQ(doc.read("mode", output), castle::status::ok);
    EXPECT_EQ(output, string_view("safe"));
    EXPECT_EQ(doc.read("missing", output), castle::status::not_found);
    EXPECT_TRUE(output.empty());
}

TEST(IniDocumentTest, SectionValuesAndDuplicateUpdateStayInPlace)
{
    document<6U, 16U, 16U, 32U> doc;

    EXPECT_EQ(doc.set("network", "host", "localhost"), castle::status::ok);
    EXPECT_EQ(doc.set("network", "port", "1883"), castle::status::ok);
    EXPECT_EQ(doc.set("network", "host", "127.0.0.1"), castle::status::ok);

    EXPECT_EQ(doc.size(), 2U);
    EXPECT_TRUE(doc.contains_section("network"));
    EXPECT_EQ(doc.section(0U), string_view("network"));
    EXPECT_EQ(doc.key(0U), string_view("host"));
    EXPECT_EQ(doc.value(0U), string_view("127.0.0.1"));
    EXPECT_EQ(doc.section(2U), string_view());
    EXPECT_EQ(doc.key(2U), string_view());

    auto found = doc.find("network", "port");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), string_view("1883"));
}

TEST(IniDocumentTest, ValidationCapacityAndRemoval)
{
    document<1U, 4U, 4U, 4U> doc;

    EXPECT_EQ(doc.set("toolong", "k", "v"), castle::status::full);
    EXPECT_EQ(doc.set("s", "toolong", "v"), castle::status::full);
    EXPECT_EQ(doc.set("s", "k", "12345"), castle::status::full);
    EXPECT_EQ(doc.set("s]bad", "k", "v"), castle::status::invalid_argument);
    EXPECT_EQ(doc.set("s", "bad=key", "v"), castle::status::invalid_argument);
    EXPECT_EQ(doc.set("s", "", "v"), castle::status::invalid_argument);

    EXPECT_EQ(doc.set("s", "k", "v"), castle::status::ok);
    EXPECT_TRUE(doc.full());
    EXPECT_EQ(doc.set("other", "k", "v"), castle::status::full);
    EXPECT_EQ(doc.remove("missing"), castle::status::not_found);
    EXPECT_EQ(doc.remove("s", "k"), castle::status::ok);
    EXPECT_TRUE(doc.empty());

    EXPECT_EQ(doc.set("s", "a", "1"), castle::status::ok);
    EXPECT_EQ(doc.set("s", "b", "2"), castle::status::full);
    EXPECT_EQ(doc.remove_section("missing"), castle::status::not_found);
    EXPECT_EQ(doc.remove_section("s"), castle::status::ok);
    EXPECT_TRUE(doc.empty());
}

TEST(IniParseTest, BasicSyntaxCommentsWhitespaceAndLineEndings)
{
    using doc_t = document<8U, 16U, 16U, 32U>;
    doc_t doc;
    const string_view source(
        "  ; comment\r\n"
        "  # comment\n"
        "global = value\r"
        "[ network ] ; section\n"
        "a: 1\r\n"
        "b = 2\n");

    const auto result = parse(doc, source);
    ASSERT_TRUE(result.succeeded());
    EXPECT_EQ(doc.size(), 3U);
    EXPECT_EQ(doc.get("global").value(), string_view("value"));
    EXPECT_EQ(doc.get("network", "a").value(), string_view("1"));
    EXPECT_EQ(doc.get("network", "b").value(), string_view("2"));
}

TEST(IniParseTest, QuotedValuesDecodeAndEmptyValues)
{
    using doc_t = document<8U, 16U, 16U, 64U>;
    doc_t doc;
    const string_view source(
        "empty = \"\"\n"
        "quoted = \"hello world\"\n"
        "escaped = \"quote=\\\" slash=\\\\ line=\\n tab=\\t\"\n"
        "plain = value#still-part-of-value\n");

    const auto result = parse(doc, source);
    ASSERT_TRUE(result.succeeded());
    EXPECT_TRUE(doc.get("empty").value().empty());
    EXPECT_EQ(doc.get("quoted").value(), string_view("hello world"));
    EXPECT_EQ(doc.get("escaped").value(), string_view("quote=\" slash=\\ line=\n tab=\t"));
    EXPECT_EQ(doc.get("plain").value(), string_view("value#still-part-of-value"));
}

TEST(IniParseTest, InvalidLinesReportSpecificErrors)
{
    {
        document<4U, 16U, 16U, 32U> doc;
        auto result = parse(doc, string_view("[broken\n"));
        EXPECT_EQ(result.status, castle::status::invalid_argument);
        EXPECT_EQ(result.code, error_code::invalid_section);
        EXPECT_EQ(result.line, 1U);
        EXPECT_TRUE(doc.empty());
    }

    {
        document<4U, 16U, 16U, 32U> doc;
        auto result = parse(doc, string_view("[ok] trailing-garbage\n"));
        EXPECT_EQ(result.code, error_code::invalid_section);
    }

    {
        document<4U, 16U, 16U, 32U> doc;
        auto result = parse(doc, string_view("= value\n"));
        EXPECT_EQ(result.code, error_code::invalid_key);
    }

    {
        document<4U, 16U, 16U, 32U> doc;
        auto result = parse(doc, string_view("key\n"));
        EXPECT_EQ(result.code, error_code::invalid_assignment);
    }

    {
        document<4U, 16U, 16U, 32U> doc;
        auto result = parse(doc, string_view("x = \"bad\"tail\n"));
        EXPECT_EQ(result.code, error_code::invalid_quote);
    }

    {
        document<4U, 16U, 16U, 32U> doc;
        auto result = parse(doc, string_view("x = \"bad\\q\"\n"));
        EXPECT_EQ(result.code, error_code::invalid_escape);
    }

    {
        document<4U, 16U, 16U, 32U> doc;
        auto result = parse(doc, string_view("[ ]\n"));
        EXPECT_EQ(result.code, error_code::invalid_section);
    }
}

TEST(IniParseTest, CapacityErrorsAreReportedAndDocumentIsCleared)
{
    {
        document<4U, 2U, 4U, 16U> doc;
        auto result = parse(doc, string_view("[abc]\nkey = 1\n"));
        EXPECT_EQ(result.status, castle::status::full);
        EXPECT_EQ(result.code, error_code::capacity);
        EXPECT_TRUE(doc.empty());
    }

    {
        document<4U, 16U, 2U, 16U> doc;
        auto result = parse(doc, string_view("abc = 1\n"));
        EXPECT_EQ(result.status, castle::status::full);
        EXPECT_EQ(result.code, error_code::capacity);
        EXPECT_TRUE(doc.empty());
    }

    {
        document<1U, 16U, 8U, 2U> doc;
        auto result = parse(doc, string_view("a = 123\n"));
        EXPECT_EQ(result.status, castle::status::full);
        EXPECT_EQ(result.code, error_code::capacity);
        EXPECT_TRUE(doc.empty());
    }

    {
        document<1U, 16U, 8U, 16U> doc;
        auto result = parse(doc, string_view("a=1\nb=2\n"));
        EXPECT_EQ(result.status, castle::status::full);
        EXPECT_EQ(result.code, error_code::capacity);
        EXPECT_TRUE(doc.empty());
    }
}

TEST(IniConversionTest, BooleanReadWrite)
{
    document<8U, 16U, 16U, 16U> doc;
    bool value = false;

    EXPECT_EQ(doc.set_bool("a", true), castle::status::ok);
    EXPECT_EQ(doc.set_bool("b", false), castle::status::ok);
    EXPECT_EQ(doc.get_bool("a", value), castle::status::ok);
    EXPECT_TRUE(value);
    EXPECT_EQ(doc.get_bool("b", value), castle::status::ok);
    EXPECT_FALSE(value);

    EXPECT_EQ(doc.set("yes", "YES"), castle::status::ok);
    EXPECT_EQ(doc.set("no", "off"), castle::status::ok);
    EXPECT_EQ(doc.set("one", "1"), castle::status::ok);
    EXPECT_EQ(doc.set("zero", "0"), castle::status::ok);
    EXPECT_EQ(doc.set("bad", "maybe"), castle::status::ok);
    EXPECT_EQ(doc.get_bool("yes", value), castle::status::ok);
    EXPECT_TRUE(value);
    EXPECT_EQ(doc.get_bool("no", value), castle::status::ok);
    EXPECT_FALSE(value);
    EXPECT_EQ(doc.get_bool("one", value), castle::status::ok);
    EXPECT_TRUE(value);
    EXPECT_EQ(doc.get_bool("zero", value), castle::status::ok);
    EXPECT_FALSE(value);
    EXPECT_EQ(doc.get_bool("bad", value), castle::status::invalid_argument);
}

TEST(IniConversionTest, IntegerReadWriteAndRangeChecks)
{
    document<16U, 16U, 16U, 32U> doc;
    int32_t signed_value = 0;
    uint32_t unsigned_value = 0U;

    EXPECT_EQ(doc.set_integer("signed", -42), castle::status::ok);
    EXPECT_EQ(doc.set_integer("unsigned", static_cast<uint32_t>(42U)), castle::status::ok);
    EXPECT_EQ(doc.get_integer("signed", signed_value), castle::status::ok);
    EXPECT_EQ(doc.get_integer("unsigned", unsigned_value), castle::status::ok);
    EXPECT_EQ(signed_value, -42);
    EXPECT_EQ(unsigned_value, 42U);

    EXPECT_EQ(doc.set("max", "2147483647"), castle::status::ok);
    EXPECT_EQ(doc.set("min", "-2147483648"), castle::status::ok);
    EXPECT_EQ(doc.get_integer("max", signed_value), castle::status::ok);
    EXPECT_EQ(signed_value, INT32_MAX);
    EXPECT_EQ(doc.get_integer("min", signed_value), castle::status::ok);
    EXPECT_EQ(signed_value, INT32_MIN);

    EXPECT_EQ(doc.set("too_high", "2147483648"), castle::status::ok);
    EXPECT_EQ(doc.get_integer("too_high", signed_value), castle::status::out_of_range);
    EXPECT_EQ(doc.set("negative_unsigned", "-1"), castle::status::full);
    EXPECT_EQ(doc.get_integer("negative_unsigned", unsigned_value), castle::status::not_found);
    EXPECT_EQ(doc.set("neg_unsigned", "-1"), castle::status::ok);
    EXPECT_EQ(doc.get_integer("neg_unsigned", unsigned_value), castle::status::invalid_argument);
    EXPECT_EQ(doc.set("bad", "12x"), castle::status::ok);
    EXPECT_EQ(doc.get_integer("bad", signed_value), castle::status::invalid_argument);
}

enum class mode : uint8_t
{
    safe = 0U,
    fast = 1U
};

TEST(IniConversionTest, EnumReadWrite)
{
    document<4U, 16U, 16U, 16U> doc;
    mode value = mode::safe;
    EXPECT_EQ(doc.set_enum("mode", mode::fast), castle::status::ok);
    EXPECT_EQ(doc.get_enum("mode", value), castle::status::ok);
    EXPECT_EQ(value, mode::fast);
}

TEST(IniConversionTest, FloatingPointRead)
{
    document<8U, 16U, 16U, 32U> doc;
    float float_value = 0.0F;
    double double_value = 0.0;

    EXPECT_EQ(doc.set("negative", "-1.25"), castle::status::ok);
    EXPECT_EQ(doc.get_floating("negative", float_value), castle::status::ok);
    EXPECT_FLOAT_EQ(float_value, -1.25F);

    EXPECT_EQ(doc.set("positive", "+3.125e+1"), castle::status::ok);
    EXPECT_EQ(doc.get_floating("positive", double_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(double_value, 31.25);

    EXPECT_EQ(doc.set("small", "4e-2"), castle::status::ok);
    EXPECT_EQ(doc.get_floating("small", double_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(double_value, 0.04);

    EXPECT_EQ(doc.set("bad1", "."), castle::status::ok);
    EXPECT_EQ(doc.get_floating("bad1", double_value), castle::status::invalid_argument);
    EXPECT_EQ(doc.set("bad2", "1e"), castle::status::ok);
    EXPECT_EQ(doc.get_floating("bad2", double_value), castle::status::invalid_argument);
    EXPECT_EQ(doc.set("bad3", "1.2x"), castle::status::ok);
    EXPECT_EQ(doc.get_floating("bad3", double_value), castle::status::invalid_argument);
    EXPECT_EQ(doc.set("huge", "1e1025"), castle::status::ok);
    EXPECT_EQ(doc.get_floating("huge", double_value), castle::status::out_of_range);
}

TEST(IniSerializeTest, CanonicalOutputQuotesAndGroupsSections)
{
    document<16U, 16U, 16U, 64U> doc;
    EXPECT_EQ(doc.set("plain", "value"), castle::status::ok);
    EXPECT_EQ(doc.set("special", "hello world"), castle::status::ok);
    EXPECT_EQ(doc.set("first", "a", "1"), castle::status::ok);
    EXPECT_EQ(doc.set("second", "b", "2"), castle::status::ok);
    EXPECT_EQ(doc.set("first", "quoted", "quote \" and line\n"), castle::status::ok);
    EXPECT_EQ(doc.set("first", "tail", "3"), castle::status::ok);

    castle::container::string<512U> output;
    const auto result = serialize(doc, output);
    ASSERT_TRUE(result.succeeded());

    EXPECT_EQ(output.view(), string_view(
        "plain = value\n"
        "special = \"hello world\"\n"
        "\n"
        "[first]\n"
        "a = 1\n"
        "quoted = \"quote \\\" and line\\n\"\n"
        "tail = 3\n"
        "\n"
        "[second]\n"
        "b = 2\n"
        "\n"));

    document<16U, 16U, 16U, 64U> round_trip;
    ASSERT_TRUE(parse(round_trip, output.view()).succeeded());
    EXPECT_EQ(round_trip.get("special").value(), string_view("hello world"));
    EXPECT_EQ(round_trip.get("first", "quoted").value(), string_view("quote \" and line\n"));
}

TEST(IniSerializeTest, OutputCapacityFailure)
{
    document<4U, 16U, 16U, 32U> doc;
    ASSERT_EQ(doc.set("key", "value"), castle::status::ok);

    castle::container::string<1U> output;
    const auto result = serialize(doc, output);
    EXPECT_EQ(result.status, castle::status::full);
    EXPECT_EQ(result.code, error_code::output_full);
}

TEST(IniParseTest, EmptyInputIsValid)
{
    document<4U, 8U, 8U, 8U> doc;
    EXPECT_TRUE(parse(doc, string_view()).succeeded());
    EXPECT_TRUE(doc.empty());
}

TEST(IniDocumentTest, CopyAndMovePreserveContent)
{
    document<4U, 8U, 8U, 16U> original;
    ASSERT_EQ(original.set("section", "key", "value"), castle::status::ok);

    document<4U, 8U, 8U, 16U> copy(original);
    EXPECT_EQ(copy.get("section", "key").value(), string_view("value"));

    document<4U, 8U, 8U, 16U> moved(castle::move(copy));
    EXPECT_EQ(moved.get("section", "key").value(), string_view("value"));
}

TEST(IniDocumentTest, DetailValueDecodeAndEscapingBranches)
{
    using namespace castle::serialization::ini;

    string<16U> decoded;
    EXPECT_TRUE(detail::decode_value(string_view(), decoded, 2U, 3U).succeeded());
    EXPECT_EQ(detail::decode_value(string_view("plain"), decoded, 1U, 1U).status, castle::status::ok);
    EXPECT_EQ(decoded.view(), string_view("plain"));
    EXPECT_EQ(detail::decode_value(string_view("\"a\\\"b\\\\c\\n\\r\\t\""), decoded, 1U, 1U).status, castle::status::ok);
    EXPECT_EQ(decoded.view(), string_view("a\"b\\c\n\r\t"));
    EXPECT_EQ(detail::decode_value(string_view("\"bad"), decoded, 1U, 1U).code, error_code::invalid_quote);
    const char trailing_escape[] = {'\"', 'a', '\\', '\"'};
    EXPECT_EQ(detail::decode_value(string_view(trailing_escape, 4U), decoded, 1U, 1U).code, error_code::invalid_escape);
    EXPECT_EQ(detail::decode_value(string_view("\"a\\q\""), decoded, 1U, 1U).code, error_code::invalid_escape);
    EXPECT_EQ(detail::decode_value(string_view("\"a\"b\""), decoded, 1U, 1U).code, error_code::invalid_quote);

    string<1U> tight;
    EXPECT_EQ(detail::decode_value(string_view("ab"), tight, 4U, 2U).status, castle::status::full);
    EXPECT_EQ(detail::decode_value(string_view("\"ab\""), tight, 4U, 2U).status, castle::status::full);
    EXPECT_EQ(detail::decode_value(string_view("\"a\\n\""), tight, 4U, 2U).status, castle::status::full);

    string<64U> encoded;
    EXPECT_EQ(detail::append_escaped_value(string_view("plain"), encoded), castle::status::ok);
    EXPECT_EQ(encoded.view(), string_view("plain"));
    encoded.clear();
    EXPECT_EQ(detail::append_escaped_value(string_view("a\"\\\n\r\t"), encoded), castle::status::ok);
    EXPECT_EQ(encoded.view(), string_view("\"a\\\"\\\\\\n\\r\\t\""));

    string<1U> quote_full;
    EXPECT_EQ(detail::append_escaped_value(string_view("x y"), quote_full), castle::status::full);
    string<1U> char_full;
    EXPECT_EQ(detail::append_escaped_value(string_view("ab"), char_full), castle::status::full);
}

TEST(IniDocumentTest, NumericBoundaryAndDocumentGroupingPaths)
{
    using namespace castle::serialization::ini;

    int32_t signed_value = 0;
    EXPECT_EQ(detail::parse_integer_value(string_view("-2147483648"), signed_value), castle::status::ok);
    EXPECT_EQ(signed_value, static_cast<int32_t>(-2147483647 - 1));
    EXPECT_EQ(detail::parse_integer_value(string_view("2147483648"), signed_value), castle::status::out_of_range);
    EXPECT_EQ(detail::parse_integer_value(string_view("-2147483649"), signed_value), castle::status::out_of_range);

    double floating_value = 0.0;
    EXPECT_EQ(detail::parse_floating_value(string_view("-3.125e+2"), floating_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(floating_value, -312.5);
    EXPECT_EQ(detail::parse_floating_value(string_view("3e-2"), floating_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(floating_value, 0.03);
    EXPECT_EQ(detail::parse_floating_value(string_view("1e1025"), floating_value), castle::status::out_of_range);
    EXPECT_EQ(detail::parse_floating_value(string_view("1e+"), floating_value), castle::status::invalid_argument);
    EXPECT_EQ(detail::parse_floating_value(string_view("1.2x"), floating_value), castle::status::invalid_argument);

    document<8U, 8U, 16U, 64U> doc;
    EXPECT_EQ(doc.set("global", "value"), castle::status::ok);
    EXPECT_EQ(doc.set("s1", "a", "1"), castle::status::ok);
    EXPECT_EQ(doc.set("s2", "b", "2"), castle::status::ok);
    EXPECT_EQ(doc.set("s1", "c", "3"), castle::status::ok);
    string<512U> output;
    ASSERT_TRUE(serialize(doc, output).succeeded());
    EXPECT_NE(output.view().find("\n\n[s1]"), string_view::npos);

    document<1U, 1U, 1U, 4U> tiny;
    EXPECT_EQ(parse(tiny, string_view("[x]\nkey=value\n")).status, castle::status::full);
}

TEST(IniDocumentTest, TypedSectionConversionsAndOutputEdges)
{
    using namespace castle::serialization::ini;
    using document_type = document<16U, 16U, 16U, 32U>;

    enum class mode : int32_t { off = 0, on = 2 };
    document_type doc;
    EXPECT_EQ(doc.set_integer("network", "count", 12345), castle::status::ok);
    EXPECT_EQ(doc.set("network", "gain", "2.5"), castle::status::ok);
    EXPECT_EQ(doc.set_enum("network", "mode", mode::on), castle::status::ok);
    int32_t count = 0;
    double gain = 0.0;
    mode selected = mode::off;
    EXPECT_EQ(doc.get_integer("network", "count", count), castle::status::ok);
    EXPECT_EQ(doc.get_floating("network", "gain", gain), castle::status::ok);
    EXPECT_EQ(doc.get_enum("network", "mode", selected), castle::status::ok);
    EXPECT_EQ(count, 12345);
    EXPECT_DOUBLE_EQ(gain, 2.5);
    EXPECT_EQ(selected, mode::on);
    EXPECT_EQ(doc.get_integer("network", "missing", count), castle::status::not_found);
    bool boolean = false;
    EXPECT_EQ(doc.get_bool("network", "count", boolean), castle::status::invalid_argument);

    document_type invalid;
    EXPECT_EQ(invalid.set("network", "bad-int", "12x"), castle::status::ok);
    EXPECT_EQ(invalid.get_integer("network", "bad-int", count), castle::status::invalid_argument);
    EXPECT_EQ(invalid.set("network", "bad-float", "1e1025"), castle::status::ok);
    EXPECT_EQ(invalid.get_floating("network", "bad-float", gain), castle::status::out_of_range);

    string<2U> quoted_close_tight;
    EXPECT_EQ(detail::append_escaped_value(string_view(" "), quoted_close_tight), castle::status::full);
    EXPECT_EQ(detail::parse_integer_value(string_view("2147483648"), count), castle::status::out_of_range);
    EXPECT_EQ(detail::parse_integer_value(string_view("12x"), count), castle::status::invalid_argument);
}

} // namespace
