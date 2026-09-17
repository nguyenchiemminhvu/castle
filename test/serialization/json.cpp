#include "castle/serialization/json.hpp"

#include <gtest/gtest.h>

namespace
{
using castle::container::string;
using castle::container::string_view;
using castle::serialization::json::document;
using castle::serialization::json::error_code;
using castle::serialization::json::parse;
using castle::serialization::json::serialize;
using castle::serialization::json::type;

TEST(JsonParseTest, ObjectsArraysAndTypes)
{
    document<32U, 256U, 8U> doc;
    auto r = parse(doc, string_view("{\"name\":\"castle\",\"ok\":true,\"n\":-12.5e2,\"x\":null,\"a\":[1,false,\"v\"]}"));
    ASSERT_TRUE(r.succeeded());
    auto root = doc.root();
    EXPECT_EQ(doc.kind(root), type::object);
    auto name = doc.find(root, "name");
    ASSERT_NE(name, doc.npos);
    EXPECT_EQ(doc.string(name), string_view("castle"));
    bool ok = false; EXPECT_EQ(doc.get_bool(doc.find(root, "ok"), ok), castle::status::ok); EXPECT_TRUE(ok);
    double number = 0.0; EXPECT_EQ(doc.get_floating(doc.find(root, "n"), number), castle::status::ok); EXPECT_DOUBLE_EQ(number, -1250.0);
    auto array = doc.find(root, "a");
    EXPECT_EQ(doc.kind(array), type::array);
    EXPECT_EQ(doc.kind(doc.at(array, 0U)), type::number);
    EXPECT_EQ(doc.kind(doc.at(array, 1U)), type::boolean);
}

TEST(JsonParseTest, EscapesUnicodeAndLiterals)
{
    document<16U, 256U, 8U> doc;
    auto r = parse(doc, string_view("{\"s\":\"a\\n\\t\\u0041\\/\\\\\\\"\",\"f\":false,\"n\":null}"));
    ASSERT_TRUE(r.succeeded());
    EXPECT_EQ(doc.string(doc.find(doc.root(), "s")), string_view("a\n\tA/\\\""));
    bool value = true; EXPECT_EQ(doc.get_bool(doc.find(doc.root(), "f"), value), castle::status::ok); EXPECT_FALSE(value);
    EXPECT_EQ(doc.kind(doc.find(doc.root(), "n")), type::null_value);
}

TEST(JsonParseTest, RejectsMalformedInput)
{
    const char* invalid[] = {
        "", "{", "[", "{\"a\"}", "{\"a\":}", "{\"a\":1,}", "[1,]",
        "{\"a\":\"bad\\q\"}", "{\"a\":\"bad\\u12x4\"}", "{\"a\":01}", "{\"a\":1.}",
        "{\"a\":1e}", "{\"a\":NaN}", "{\"a\":truex}", "{\"a\":nul}"
    };
    for (const char* text : invalid)
    {
        document<32U, 256U, 8U> doc;
        auto r = parse(doc, string_view(text));
        EXPECT_FALSE(r.succeeded());
        EXPECT_TRUE(doc.empty());
    }
    document<8U, 64U, 4U> doc;
    auto r = parse(doc, string_view("{\"a\":\"x\"}"));
    EXPECT_TRUE(r.succeeded());
}

TEST(JsonParseTest, CapacityIsDeterministic)
{
    document<2U, 16U, 4U> doc;
    auto r = parse(doc, string_view("{\"a\":1,\"b\":2}"));
    EXPECT_EQ(r.status, castle::status::full);
    EXPECT_EQ(r.code, error_code::capacity);
    EXPECT_TRUE(doc.empty());

    document<8U, 3U, 4U> small;
    r = parse(small, string_view("{\"long\":\"abcd\"}"));
    EXPECT_EQ(r.status, castle::status::full);
    EXPECT_TRUE(small.empty());
}

TEST(JsonDocumentTest, ProgrammaticConstructionAndTypedAccess)
{
    document<16U, 256U, 8U> doc;
    document<16U, 256U, 8U>::node_id root = document<16U, 256U, 8U>::npos;
    document<16U, 256U, 8U>::node_id value = document<16U, 256U, 8U>::npos;
    ASSERT_EQ(doc.make_object(root), castle::status::ok);
    EXPECT_EQ(doc.set_string(value, "hello", root, "text"), castle::status::ok);
    EXPECT_EQ(doc.set_integer(value, -42, root, "integer"), castle::status::ok);
    EXPECT_EQ(doc.set_bool(value, true, root, "flag"), castle::status::ok);
    EXPECT_EQ(doc.set_number(value, "3.14", root, "pi"), castle::status::ok);
    EXPECT_EQ(doc.set_number(value, "01", root, "bad"), castle::status::invalid_argument);
    int integer = 0; EXPECT_EQ(doc.get_integer(doc.find(root, "integer"), integer), castle::status::ok); EXPECT_EQ(integer, -42);
    EXPECT_EQ(doc.get_integer(doc.find(root, "pi"), integer), castle::status::invalid_argument);
    string_view text; EXPECT_EQ(doc.get_string(doc.find(root, "text"), text), castle::status::ok); EXPECT_EQ(text, string_view("hello"));
}

enum class mode : uint8_t { safe = 1U, fast = 2U };

TEST(JsonDocumentTest, EnumFloatAndArray)
{
    document<16U, 256U, 8U> doc;
    document<16U, 256U, 8U>::node_id root = document<16U, 256U, 8U>::npos;
    document<16U, 256U, 8U>::node_id array = document<16U, 256U, 8U>::npos;
    document<16U, 256U, 8U>::node_id item = document<16U, 256U, 8U>::npos;
    ASSERT_EQ(doc.make_object(root), castle::status::ok);
    ASSERT_EQ(doc.make_array(array, root, "items"), castle::status::ok);
    EXPECT_EQ(doc.set_enum(item, mode::fast, array), castle::status::ok);
    EXPECT_EQ(doc.set_floating(item, 1.25, root, "ratio", 4U), castle::status::ok);
    mode m = mode::safe; EXPECT_EQ(doc.get_enum(doc.at(array, 0U), m), castle::status::ok); EXPECT_EQ(m, mode::fast);
    double d = 0.0; EXPECT_EQ(doc.get_floating(doc.find(root, "ratio"), d), castle::status::ok); EXPECT_DOUBLE_EQ(d, 1.25);
}

TEST(JsonSerializeTest, CompactAndPretty)
{
    document<16U, 256U, 8U> doc;
    ASSERT_EQ(parse(doc, string_view("{\"b\":true,\"s\":\"x\\ny\",\"a\":[1,2]}" )).status, castle::status::ok);
    castle::container::string<256U> compact;
    ASSERT_TRUE(serialize(doc, compact).succeeded());
    EXPECT_EQ(compact.view(), string_view("{\"b\":true,\"s\":\"x\\ny\",\"a\":[1,2]}"));
    castle::container::string<256U> pretty;
    ASSERT_TRUE(serialize(doc, pretty, true, 2U).succeeded());
    EXPECT_NE(pretty.view().find("\n"), pretty.view().npos);

    castle::container::string<8U> too_small;
    auto r = serialize(doc, too_small);
    EXPECT_EQ(r.status, castle::status::full);
    EXPECT_TRUE(too_small.empty());
}

TEST(JsonSerializeTest, EmptyAndStringEscaping)
{
    document<4U, 128U, 4U> doc;
    document<4U, 128U, 4U>::node_id root = document<4U, 128U, 4U>::npos;
    ASSERT_EQ(doc.make_array(root), castle::status::ok);
    castle::container::string<32U> out;
    ASSERT_TRUE(serialize(doc, out).succeeded()); EXPECT_EQ(out.view(), string_view("[]"));
    doc.clear();
    ASSERT_EQ(doc.set_string(root, "a\"b\\c\n"), castle::status::ok);
    ASSERT_TRUE(serialize(doc, out).succeeded()); EXPECT_EQ(out.view(), string_view("\"a\\\"b\\\\c\\n\""));
}

TEST(JsonParseTest, StringDecoderEscapesErrorsAndCapacity)
{
    using namespace castle::serialization::json;

    string<32U> output;
    castle::size_type position = 0U;
    const result decoded = detail::decode_string(
        string_view("\"\\\"\\\\\\/\\b\\f\\n\\r\\t\""), position, output);
    EXPECT_TRUE(decoded.succeeded());
    EXPECT_EQ(output.size(), 8U);
    EXPECT_EQ(output[0U], '"');
    EXPECT_EQ(output[1U], '\\');
    EXPECT_EQ(output[2U], '/');
    EXPECT_EQ(output[3U], '\b');
    EXPECT_EQ(output[4U], '\f');
    EXPECT_EQ(output[5U], '\n');
    EXPECT_EQ(output[6U], '\r');
    EXPECT_EQ(output[7U], '\t');

    position = 0U;
    EXPECT_EQ(detail::decode_string(string_view("\"\\u0041\\u00E9\\u20AC\""), position, output).status, castle::status::ok);
    EXPECT_EQ(output.size(), 6U);

    position = 0U;
    EXPECT_EQ(detail::decode_string(string_view("x"), position, output).code, error_code::invalid_string);
    position = 0U;
    EXPECT_EQ(detail::decode_string(string_view("\"abc"), position, output).code, error_code::unexpected_end);
    position = 0U;
    EXPECT_EQ(detail::decode_string(string_view("\"abc\\"), position, output).code, error_code::unexpected_end);
    position = 0U;
    EXPECT_EQ(detail::decode_string(string_view("\"abc\\q\""), position, output).code, error_code::invalid_escape);
    position = 0U;
    EXPECT_EQ(detail::decode_string(string_view("\"abc\\u12x4\""), position, output).code, error_code::invalid_unicode_escape);
    position = 0U;
    EXPECT_EQ(detail::decode_string(string_view("\"\\uD800\""), position, output).code, error_code::invalid_unicode_escape);

    string<2U> tight;
    position = 0U;
    EXPECT_EQ(detail::decode_string(string_view("\"abc\""), position, tight).status, castle::status::full);
    position = 0U;
    EXPECT_EQ(detail::decode_string(string_view("\"\\n\""), position, tight).status, castle::status::ok);
    position = 0U;
}

TEST(JsonParseTest, NumberHelpersAndDocumentInvalidHandles)
{
    using namespace castle::serialization::json;

    int32_t integer = 0;
    EXPECT_EQ(detail::parse_integer(string_view("-2147483648"), integer), castle::status::ok);
    EXPECT_EQ(integer, static_cast<int32_t>(-2147483647 - 1));
    EXPECT_EQ(detail::parse_integer(string_view("2147483648"), integer), castle::status::out_of_range);
    EXPECT_EQ(detail::parse_integer(string_view("-2147483649"), integer), castle::status::out_of_range);

    double value = 0.0;
    EXPECT_EQ(detail::parse_floating(string_view("-1.25e+2"), value), castle::status::ok);
    EXPECT_DOUBLE_EQ(value, -125.0);
    EXPECT_EQ(detail::parse_floating(string_view("5e-2"), value), castle::status::ok);
    EXPECT_DOUBLE_EQ(value, 0.05);
    EXPECT_EQ(detail::parse_floating(string_view("1e1024"), value), castle::status::out_of_range);

    string<32U> escaped;
    EXPECT_EQ(detail::append_escaped(escaped, string_view("a\"b\\c\b\f\n\r\t")), castle::status::ok);
    EXPECT_EQ(escaped.view(), string_view("\"a\\\"b\\\\c\\b\\f\\n\\r\\t\""));
    string<1U> too_small;
    EXPECT_EQ(detail::append_escaped(too_small, string_view("abc")), castle::status::full);

    document<8U, 64U, 4U> doc;
    document<8U, 64U, 4U>::node_id node = document<8U, 64U, 4U>::npos;
    EXPECT_EQ(doc.kind(doc.npos), type::null_value);
    EXPECT_EQ(doc.find(doc.npos, "x"), doc.npos);
    EXPECT_EQ(doc.at(doc.npos, 0U), doc.npos);
    EXPECT_EQ(doc.set_number(node, "bad"), castle::status::invalid_argument);
    ASSERT_EQ(doc.make_object(node), castle::status::ok);
    EXPECT_EQ(doc.set_string(node, "v", doc.npos), castle::status::already_exists);
    EXPECT_EQ(doc.append(doc.npos, node, type::string, "x"), castle::status::invalid_argument);
    EXPECT_EQ(doc.append(node, node, type::string, "x"), castle::status::invalid_argument);

    document<1U, 16U, 4U> one;
    document<1U, 16U, 4U>::node_id root = document<1U, 16U, 4U>::npos;
    ASSERT_EQ(one.make_object(root), castle::status::ok);
    document<1U, 16U, 4U>::node_id child = document<1U, 16U, 4U>::npos;
    EXPECT_EQ(one.set_string(child, "x", root, "k"), castle::status::full);
}

TEST(JsonParseTest, ParserCapacityDepthAndLiteralBranches)
{
    using namespace castle::serialization::json;

    document<2U, 64U, 2U> nodes;
    result r = parse(nodes, string_view("{\"a\":1,\"b\":2}"));
    EXPECT_EQ(r.status, castle::status::full);
    EXPECT_EQ(r.code, error_code::capacity);

    document<16U, 64U, 1U> depth;
    r = parse(depth, string_view("[[0]]"));
    EXPECT_EQ(r.code, error_code::depth_exceeded);

    const char* invalid[] = {
        "truex", "fals", "nulx",
        "[1 2]", "[1,]", "{\"a\":1 \"b\":2}"
    };
    for (const char* text : invalid)
    {
        document<32U, 128U, 8U> doc;
        r = parse(doc, string_view(text));
        if (text[0U] == '{' || text[0U] == '[' || text[0U] == 't' || text[0U] == 'f' || text[0U] == 'n')
        {
            EXPECT_FALSE(r.succeeded());
        }
    }

    document<16U, 128U, 8U> literals;
    ASSERT_TRUE(parse(literals, string_view("{\"t\":true,\"f\":false,\"n\":null,\"z\":0}")).succeeded());
    bool boolean = false;
    EXPECT_EQ(literals.get_bool(literals.find(literals.root(), "t"), boolean), castle::status::ok);
    EXPECT_TRUE(boolean);
    EXPECT_EQ(literals.get_bool(literals.find(literals.root(), "f"), boolean), castle::status::ok);
    EXPECT_FALSE(boolean);
}

TEST(JsonParseTest, SerializationSpecialNodesAndOutputFailureEdges)
{
    using namespace castle::serialization::json;

    document<32U, 256U, 8U> doc;
    document<32U, 256U, 8U>::node_id root = document<32U, 256U, 8U>::npos;
    document<32U, 256U, 8U>::node_id child = document<32U, 256U, 8U>::npos;
    ASSERT_EQ(doc.make_object(root), castle::status::ok);
    ASSERT_EQ(doc.make_array(child, root, "a"), castle::status::ok);
    ASSERT_EQ(doc.set_bool(child, false, child), castle::status::ok);
    ASSERT_EQ(doc.set_null(child, child), castle::status::ok);
    ASSERT_EQ(doc.set_string(child, "ok", child), castle::status::ok);

    string<256U> output;
    ASSERT_TRUE(serialize(doc, output, true, 2U).succeeded());
    EXPECT_NE(output.view().find("\n"), string_view::npos);

    string<2U> tiny;
    const result overflow = serialize(doc, tiny);
    EXPECT_EQ(overflow.status, castle::status::full);
    EXPECT_EQ(overflow.code, error_code::output_full);

    document<4U, 32U, 2U> empty_object;
    ASSERT_EQ(empty_object.make_object(root), castle::status::ok);
    string<2U> empty_output;
    ASSERT_TRUE(serialize(empty_object, empty_output).succeeded());
    EXPECT_EQ(empty_output.view(), string_view("{}"));
}

TEST(JsonParseTest, RootScalarsTrailingTokensAndStringStorageEdges)
{
    using namespace castle::serialization::json;

    document<16U, 64U, 8U> scalar;
    EXPECT_TRUE(parse(scalar, string_view("true")).succeeded());
    EXPECT_TRUE(parse(scalar, string_view("false")).succeeded());
    EXPECT_TRUE(parse(scalar, string_view("null")).succeeded());
    EXPECT_TRUE(parse(scalar, string_view("123")).succeeded());
    EXPECT_TRUE(parse(scalar, string_view("\"text\"")).succeeded());
    EXPECT_EQ(parse(scalar, string_view("1 2")).code, error_code::unexpected_token);

    string<8U> invalid_control;
    const char control[] = {'\x01'};
    EXPECT_EQ(detail::append_escaped(invalid_control, string_view(control, 1U)), castle::status::invalid_argument);

    document<4U, 3U, 4U> tight;
    document<4U, 3U, 4U>::node_id node = document<4U, 3U, 4U>::npos;
    EXPECT_EQ(tight.make_object(node), castle::status::ok);
    EXPECT_EQ(tight.set_string(node, "abc", node, "k"), castle::status::full);
}

} // namespace
