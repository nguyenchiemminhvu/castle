#include "castle/serialization/json.hpp"

#include <gtest/gtest.h>

namespace
{
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

} // namespace
