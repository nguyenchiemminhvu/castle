#include <gtest/gtest.h>

#include "castle/serialization/xml.hpp"

namespace
{

namespace xml = castle::serialization::xml;
using castle::container::string;
using castle::container::string_view;

using document = xml::document<64U, 64U, 2048U, 8U>;

struct mode
{
    enum value
    {
        off = 0,
        on = 1
    };
};

TEST(XmlDocumentTest, EmptyDocumentAndClear)
{
    document doc;
    EXPECT_TRUE(doc.empty());
    EXPECT_EQ(doc.root(), document::npos);
    EXPECT_EQ(doc.document_node(), 0U);
    EXPECT_EQ(doc.size(), 0U);
    EXPECT_EQ(doc.bytes_used(), 0U);

    document::node_id root = document::npos;
    EXPECT_EQ(doc.make_element(root, string_view("root")), castle::status::ok);
    EXPECT_FALSE(doc.empty());
    doc.clear();
    EXPECT_TRUE(doc.empty());
    EXPECT_EQ(doc.size(), 0U);
}

TEST(XmlDocumentTest, ParseElementsAttributesEntitiesAndTypedValues)
{
    document doc;
    const xml::result r = xml::parse(
        doc,
        string_view("<?xml version=\"1.0\"?><device id=\"42\" enabled=\"true\" note='a&amp;b'>"
                    "<name>castle &lt; xml</name><count>123</count><ratio>2.5e1</ratio></device>"));

    ASSERT_TRUE(r.succeeded());
    EXPECT_EQ(doc.size(), 8U); // declaration + root + 3 element nodes + 3 text nodes

    const document::node_id root = doc.root();
    ASSERT_NE(root, document::npos);
    EXPECT_EQ(doc.kind(root), xml::type::element);
    EXPECT_EQ(doc.name(root), string_view("device"));

    string_view value;
    ASSERT_EQ(doc.get_attribute(root, string_view("id"), value), castle::status::ok);
    EXPECT_EQ(value, string_view("42"));
    ASSERT_EQ(doc.get_attribute(root, string_view("note"), value), castle::status::ok);
    EXPECT_EQ(value, string_view("a&b"));

    uint32_t id = 0U;
    EXPECT_EQ(doc.get_attribute(root, string_view("id"), id), castle::status::ok);
    EXPECT_EQ(id, 42U);

    bool enabled = false;
    EXPECT_EQ(doc.get_attribute(root, string_view("enabled"), enabled), castle::status::ok);
    EXPECT_TRUE(enabled);

    const document::node_id name = doc.find_child(root, string_view("name"));
    const document::node_id count = doc.find_child(root, string_view("count"));
    const document::node_id ratio = doc.find_child(root, string_view("ratio"));
    ASSERT_NE(name, document::npos);
    ASSERT_NE(count, document::npos);
    ASSERT_NE(ratio, document::npos);

    EXPECT_EQ(doc.value(doc.first_child(name)), string_view("castle < xml"));

    uint32_t count_value = 0U;
    EXPECT_EQ(doc.get_text(count, count_value), castle::status::ok);
    EXPECT_EQ(count_value, 123U);

    double ratio_value = 0.0;
    EXPECT_EQ(doc.get_text(ratio, ratio_value), castle::status::ok);
    EXPECT_DOUBLE_EQ(ratio_value, 25.0);
}

TEST(XmlDocumentTest, ParseBomUnicodeCharacterReferencesAndCrNormalization)
{
    document doc;
    const char source[] = "\xEF\xBB\xBF<root>A\r\nB &#x1F600; &#65;</root>";
    ASSERT_TRUE(xml::parse(doc, string_view(source)).succeeded());

    const document::node_id root = doc.root();
    const document::node_id text = doc.first_child(root);
    ASSERT_NE(text, document::npos);
    EXPECT_EQ(doc.value(text), string_view("A\nB \xF0\x9F\x98\x80 A"));
}

TEST(XmlDocumentTest, ParseCdataCommentAndProcessingInstruction)
{
    document doc;
    ASSERT_TRUE(xml::parse(doc, string_view("<root><![CDATA[a<b&c]]><!--comment--><?pi data?></root>")).succeeded());

    const document::node_id root = doc.root();
    const document::node_id cdata = doc.first_child(root);
    ASSERT_NE(cdata, document::npos);
    EXPECT_EQ(doc.kind(cdata), xml::type::cdata);
    EXPECT_EQ(doc.value(cdata), string_view("a<b&c"));

    const document::node_id comment = doc.next_sibling(cdata);
    ASSERT_NE(comment, document::npos);
    EXPECT_EQ(doc.kind(comment), xml::type::comment);
    EXPECT_EQ(doc.value(comment), string_view("comment"));

    const document::node_id pi = doc.next_sibling(comment);
    ASSERT_NE(pi, document::npos);
    EXPECT_EQ(doc.kind(pi), xml::type::processing_instruction);
    EXPECT_EQ(doc.name(pi), string_view("pi"));
    EXPECT_EQ(doc.value(pi), string_view("data"));
}

TEST(XmlDocumentTest, ParseDoctypeAndTopLevelNodes)
{
    document doc;
    ASSERT_TRUE(xml::parse(doc, string_view("<!DOCTYPE root [<!ELEMENT root ANY>]><!--prologue--><root/><?tail ok?>")).succeeded());

    document::node_id node = doc.first_child(doc.document_node());
    ASSERT_NE(node, document::npos);
    EXPECT_EQ(doc.kind(node), xml::type::doctype);
    node = doc.next_sibling(node);
    ASSERT_NE(node, document::npos);
    EXPECT_EQ(doc.kind(node), xml::type::comment);
    node = doc.next_sibling(node);
    ASSERT_NE(node, document::npos);
    EXPECT_EQ(doc.kind(node), xml::type::element);
    node = doc.next_sibling(node);
    ASSERT_NE(node, document::npos);
    EXPECT_EQ(doc.kind(node), xml::type::processing_instruction);
}

TEST(XmlDocumentTest, FindAndAttributeTraversal)
{
    document doc;
    ASSERT_TRUE(xml::parse(doc, string_view("<root a=\"1\" b=\"2\"><x/><x/></root>")).succeeded());
    const document::node_id root = doc.root();

    const document::attribute_id first = doc.first_attribute(root);
    ASSERT_NE(first, document::attribute_npos);
    EXPECT_EQ(doc.attribute_name(first), string_view("a"));
    EXPECT_EQ(doc.attribute_value(first), string_view("1"));
    EXPECT_EQ(doc.attribute_name(doc.next_attribute(first)), string_view("b"));

    EXPECT_NE(doc.find_attribute(root, string_view("b")), document::attribute_npos);
    EXPECT_EQ(doc.find_attribute(root, string_view("missing")), document::attribute_npos);

    const document::node_id x1 = doc.find_child(root, string_view("x"));
    ASSERT_NE(x1, document::npos);
    const document::node_id x2 = doc.next_sibling(x1);
    ASSERT_NE(x2, document::npos);
    EXPECT_EQ(doc.kind(x2), xml::type::element);
    EXPECT_EQ(doc.find_child(x1, string_view("x")), document::npos);
}

TEST(XmlDocumentTest, ParseErrorsExposeOffsetLineColumnAndClearDocument)
{
    document doc;
    const xml::result r = xml::parse(doc, string_view("<root>\n  <child>"));
    EXPECT_EQ(r.status, castle::status::invalid_argument);
    EXPECT_EQ(r.code, xml::error_code::unexpected_end);
    EXPECT_EQ(r.line, 2U);
    EXPECT_EQ(r.column, 10U);
    EXPECT_EQ(doc.root(), document::npos);
}

TEST(XmlDocumentTest, InvalidRootAndTagSyntax)
{
    const char* inputs[] =
    {
        "",
        "text",
        "<>",
        "<1root/>",
        "<root>",
        "<root></wrong>",
        "</root>",
        "< root/>",
        "<root/><other/>",
        "<?xml-stylesheet data?>"
    };

    for (const char* input : inputs)
    {
        document doc;
        const xml::result r = xml::parse(doc, string_view(input));
        EXPECT_FALSE(r.succeeded());
    }
}

TEST(XmlDocumentTest, DuplicateAndMalformedAttributes)
{
    document doc;
    EXPECT_EQ(xml::parse(doc, string_view("<root a=\"1\" a=\"2\"/>")).code, xml::error_code::duplicate_attribute);
    EXPECT_EQ(xml::parse(doc, string_view("<root a=\"1\"b=\"2\"/>")).code, xml::error_code::invalid_attribute);
    EXPECT_EQ(xml::parse(doc, string_view("<root a\"1\"/>")).code, xml::error_code::invalid_attribute);
    EXPECT_EQ(xml::parse(doc, string_view("<root a=1/>")).code, xml::error_code::invalid_attribute);
    EXPECT_EQ(xml::parse(doc, string_view("<root a=\"1></root>")).code, xml::error_code::unexpected_end);
}

TEST(XmlDocumentTest, InvalidEntitiesAndCharacters)
{
    document doc;
    EXPECT_EQ(xml::parse(doc, string_view("<root>&unknown;</root>")).code, xml::error_code::invalid_entity);
    EXPECT_EQ(xml::parse(doc, string_view("<root>&#0;</root>")).code, xml::error_code::invalid_entity);
    EXPECT_EQ(xml::parse(doc, string_view("<root>&amp</root>")).code, xml::error_code::invalid_entity);
    EXPECT_EQ(xml::parse(doc, string_view("<root>\x01</root>")).code, xml::error_code::invalid_character);
    const char invalid_utf8[] = "<root>\xF0\x28\x8C\x28</root>";
    EXPECT_EQ(xml::parse(doc, string_view(invalid_utf8)).code, xml::error_code::invalid_character);
}

TEST(XmlDocumentTest, CommentCdataProcessingInstructionAndDeclarationErrors)
{
    document doc;
    EXPECT_EQ(xml::parse(doc, string_view("<root><!--bad--comment--></root>")).code, xml::error_code::invalid_comment);
    EXPECT_EQ(xml::parse(doc, string_view("<root><!--bad---></root>")).code, xml::error_code::invalid_comment);
    EXPECT_EQ(xml::parse(doc, string_view("<![CDATA[x]]>")).code, xml::error_code::unexpected_token);
    EXPECT_EQ(xml::parse(doc, string_view("<root><?xml bad?></root>")).code, xml::error_code::invalid_declaration);
    EXPECT_TRUE(xml::parse(doc, string_view("<?xml version=\"1.0\"?> <root/>")).succeeded());
    EXPECT_EQ(xml::parse(doc, string_view(" <?xml version=\"1.0\"?><root/>")).code, xml::error_code::invalid_declaration);
    EXPECT_EQ(xml::parse(doc, string_view("<?xmlx?><root/>")).succeeded(), true);
}

TEST(XmlDocumentTest, DepthAndCapacity)
{
    using tiny_depth = xml::document<16U, 16U, 256U, 2U>;
    tiny_depth depth_doc;
    EXPECT_EQ(xml::parse(depth_doc, string_view("<a><b><c/></b></a>")).code, xml::error_code::depth_exceeded);

    using tiny_strings = xml::document<16U, 16U, 8U, 4U>;
    tiny_strings string_doc;
    EXPECT_EQ(xml::parse(string_doc, string_view("<root>123456789</root>")).code, xml::error_code::capacity);

    using tiny_nodes = xml::document<2U, 16U, 64U, 4U>;
    tiny_nodes node_doc;
    EXPECT_EQ(xml::parse(node_doc, string_view("<a><b><c/></b></a>")).code, xml::error_code::capacity);
}

TEST(XmlDocumentTest, AttributesCommentsCdataAndProcessingInstructions)
{
    document doc;
    document::node_id root = document::npos;
    ASSERT_EQ(doc.make_element(root, string_view("root")), castle::status::ok);

    document::attribute_id attr = document::attribute_npos;
    EXPECT_EQ(doc.add_attribute(attr, root, string_view("x"), string_view("1")), castle::status::ok);
    EXPECT_EQ(doc.add_attribute(attr, root, string_view("x"), string_view("2")), castle::status::already_exists);

    document::node_id cdata = document::npos;
    document::node_id comment = document::npos;
    document::node_id pi = document::npos;
    EXPECT_EQ(doc.append_cdata(cdata, string_view("a<b"), root), castle::status::ok);
    EXPECT_EQ(doc.append_comment(comment, string_view("comment"), root), castle::status::ok);
    EXPECT_EQ(doc.append_processing_instruction(pi, string_view("mode"), string_view("debug"), root), castle::status::ok);
    EXPECT_EQ(doc.append_cdata(cdata, string_view("]]>") , root), castle::status::invalid_argument);
    EXPECT_EQ(doc.append_comment(comment, string_view("bad--comment"), root), castle::status::invalid_argument);
    EXPECT_EQ(doc.append_processing_instruction(pi, string_view("xml"), string_view("bad"), root), castle::status::invalid_argument);
    EXPECT_EQ(doc.append_processing_instruction(pi, string_view("pi"), string_view("?>"), root), castle::status::invalid_argument);
}

TEST(XmlDocumentTest, MutationAndTypedAccessWithoutHeap)
{
    document doc;
    document::node_id root = document::npos;
    ASSERT_EQ(doc.make_element(root, string_view("root")), castle::status::ok);

    document::attribute_id attr = document::attribute_npos;
    EXPECT_EQ(doc.add_attribute(attr, root, string_view("id"), string_view("10")), castle::status::ok);
    EXPECT_EQ(doc.add_attribute(attr, root, string_view("id"), string_view("11")), castle::status::already_exists);
    EXPECT_EQ(doc.set_attribute(root, string_view("id"), 11U), castle::status::ok);
    EXPECT_EQ(doc.set_attribute(root, string_view("enabled"), true), castle::status::ok);
    EXPECT_EQ(doc.set_attribute(root, string_view("gain"), 2.5, 2U), castle::status::ok);
    EXPECT_EQ(doc.set_attribute(root, string_view("mode"), mode::on), castle::status::ok);

    document::node_id child = document::npos;
    ASSERT_EQ(doc.make_element(child, string_view("value"), root), castle::status::ok);
    EXPECT_EQ(doc.set_text(child, 123U), castle::status::ok);
    EXPECT_EQ(doc.set_text(child, false), castle::status::ok);
    EXPECT_EQ(doc.set_text(child, 2.5, 2U), castle::status::ok);
    EXPECT_EQ(doc.set_text(child, mode::on), castle::status::ok);

    document::node_id second_text = document::npos;
    EXPECT_EQ(doc.append_text(second_text, string_view("tail"), child), castle::status::ok);
    EXPECT_EQ(doc.get_text(child, second_text), castle::status::data_loss);

    int id = 0;
    bool enabled = false;
    double gain = 0.0;
    mode::value selected = mode::off;
    EXPECT_EQ(doc.get_attribute(root, string_view("id"), id), castle::status::ok);
    EXPECT_EQ(doc.get_attribute(root, string_view("enabled"), enabled), castle::status::ok);
    EXPECT_EQ(doc.get_attribute(root, string_view("gain"), gain), castle::status::ok);
    EXPECT_EQ(doc.get_attribute(root, string_view("mode"), selected), castle::status::ok);
    EXPECT_EQ(id, 11);
    EXPECT_TRUE(enabled);
    EXPECT_DOUBLE_EQ(gain, 2.5);
    EXPECT_EQ(selected, mode::on);

    EXPECT_EQ(doc.get_attribute(root, string_view("missing"), gain), castle::status::not_found);
    EXPECT_EQ(doc.get_attribute(root, string_view("enabled"), id), castle::status::invalid_argument);

    document::node_id cdata = document::npos;
    document::node_id comment = document::npos;
    document::node_id pi = document::npos;
    EXPECT_EQ(doc.append_cdata(cdata, string_view("a<b"), root), castle::status::ok);
    EXPECT_EQ(doc.append_comment(comment, string_view("comment"), root), castle::status::ok);
    EXPECT_EQ(doc.append_processing_instruction(pi, string_view("mode"), string_view("debug"), root), castle::status::ok);
    EXPECT_EQ(doc.append_cdata(cdata, string_view("]]>") , root), castle::status::invalid_argument);
    EXPECT_EQ(doc.append_comment(comment, string_view("bad--comment"), root), castle::status::invalid_argument);
    EXPECT_EQ(doc.append_processing_instruction(pi, string_view("xml"), string_view("bad"), root), castle::status::invalid_argument);
}

TEST(XmlDocumentTest, InvalidMutationArguments)
{
    document doc;
    document::node_id node = document::npos;
    EXPECT_EQ(doc.make_element(node, string_view()), castle::status::invalid_argument);
    EXPECT_EQ(doc.make_element(node, string_view("1bad")), castle::status::invalid_argument);
    EXPECT_EQ(doc.append_text(node, string_view("x"), document::npos), castle::status::invalid_argument);
    document::attribute_id invalid_attr = document::attribute_npos; EXPECT_EQ(doc.add_attribute(invalid_attr, document::npos, string_view("a"), string_view("b")), castle::status::invalid_argument);

    document::node_id root = document::npos;
    ASSERT_EQ(doc.make_element(root, string_view("root")), castle::status::ok);
    EXPECT_EQ(doc.append_cdata(node, string_view("x"), root), castle::status::ok);
    EXPECT_EQ(doc.append_cdata(node, string_view("]]>") , root), castle::status::invalid_argument);
    EXPECT_EQ(doc.append_processing_instruction(node, string_view("xml")), castle::status::invalid_argument);
    EXPECT_EQ(doc.append_processing_instruction(node, string_view("pi"), string_view("?>"), root), castle::status::invalid_argument);
    EXPECT_EQ(doc.append_declaration(node, string_view("version=\"1.0\"")), castle::status::invalid_argument);

    document meta;
    EXPECT_EQ(meta.append_declaration(node, string_view("version=\"1.0\"")), castle::status::ok);
    EXPECT_EQ(meta.append_declaration(node, string_view("version=\"1.0\"")), castle::status::already_exists);
    EXPECT_EQ(meta.append_doctype(node, string_view(" root")), castle::status::ok);
    EXPECT_EQ(meta.append_doctype(node, string_view(" root")), castle::status::already_exists);
}

TEST(XmlDocumentTest, CompactPrettyMixedContentAndOutputOverflow)
{
    document doc;
    ASSERT_TRUE(xml::parse(doc, string_view("<?xml version=\"1.0\"?><root a=\"1\"><a/><b/></root>")).succeeded());

    castle::container::string<512U> compact;
    ASSERT_TRUE(xml::serialize(doc, compact, false).succeeded());
    EXPECT_EQ(compact.view(), string_view("<?xml version=\"1.0\"?><root a=\"1\"><a/><b/></root>"));

    castle::container::string<512U> pretty;
    ASSERT_TRUE(xml::serialize(doc, pretty, true, 2U).succeeded());
    EXPECT_NE(pretty.find(string_view("\n  <a/>")), string_view::npos);

    document mixed;
    ASSERT_TRUE(xml::parse(mixed, string_view("<root>Hello <b>world</b>!</root>")).succeeded());
    castle::container::string<256U> mixed_output;
    ASSERT_TRUE(xml::serialize(mixed, mixed_output, true, 2U).succeeded());
    EXPECT_EQ(mixed_output.view(), string_view("<root>Hello <b>world</b>!</root>"));

    castle::container::string<16U> tiny;
    const xml::result overflow = xml::serialize(doc, tiny);
    EXPECT_EQ(overflow.status, castle::status::full);
    EXPECT_EQ(overflow.code, xml::error_code::output_full);
    EXPECT_TRUE(tiny.empty());
}

TEST(XmlDocumentTest, EmptyDocument)
{
    document doc;
    castle::container::string<128U> out("not-empty");
    const xml::result r = xml::serialize(doc, out);
    EXPECT_EQ(r.status, castle::status::empty);
    EXPECT_EQ(r.code, xml::error_code::none);
    EXPECT_TRUE(out.empty());
}


TEST(XmlDocumentTest, BoundaryHelpersAndDeclarationValidation)
{
    castle::size_type position = 0U;
    uint32_t codepoint = 0U;
    EXPECT_FALSE(xml::detail::parse_character_reference(string_view("x"), position, codepoint));
    EXPECT_FALSE(xml::detail::parse_character_reference(string_view("&bad;"), position, codepoint));
    position = 0U;
    EXPECT_FALSE(xml::detail::parse_character_reference(string_view("&#x;"), position, codepoint));
    position = 0U;
    EXPECT_FALSE(xml::detail::parse_character_reference(string_view("&#9999999999;"), position, codepoint));
    position = 0U;
    EXPECT_TRUE(xml::detail::parse_character_reference(string_view("&#X41;"), position, codepoint));
    EXPECT_EQ(codepoint, 0x41U);
    position = 0U;
    EXPECT_TRUE(xml::detail::parse_character_reference(string_view("&gt;"), position, codepoint));
    position = 0U;
    EXPECT_TRUE(xml::detail::parse_character_reference(string_view("&quot;"), position, codepoint));
    position = 0U;
    EXPECT_TRUE(xml::detail::parse_character_reference(string_view("&apos;"), position, codepoint));

    EXPECT_TRUE(xml::detail::valid_declaration(string_view("version=\"1.0\"")));
    EXPECT_TRUE(xml::detail::valid_declaration(string_view("version=\"1.1\" encoding=\"UTF-8\" standalone='yes'")));
    EXPECT_FALSE(xml::detail::valid_declaration(string_view("encoding=\"UTF-8\"")));
    EXPECT_FALSE(xml::detail::valid_declaration(string_view("version")));
    EXPECT_FALSE(xml::detail::valid_declaration(string_view("version=1.0")));
    EXPECT_FALSE(xml::detail::valid_declaration(string_view("version=\"1.2\"")));
    EXPECT_FALSE(xml::detail::valid_declaration(string_view("version=\"1.0")));
    EXPECT_FALSE(xml::detail::valid_declaration(string_view("version=\"1.0\" encoding")));
    EXPECT_FALSE(xml::detail::valid_declaration(string_view("version=\"1.0\" encoding=1")));
    EXPECT_FALSE(xml::detail::valid_declaration(string_view("version=\"1.0\" encoding=\"1-0\"")));
    EXPECT_FALSE(xml::detail::valid_declaration(string_view("version=\"1.0\" encoding=\"UTF-8\" encoding=\"ASCII\"")));
    EXPECT_FALSE(xml::detail::valid_declaration(string_view("version=\"1.0\" standalone=\"maybe\"")));
    EXPECT_TRUE(xml::detail::valid_declaration(string_view("version=\"1.0\" standalone=\"no\"")));
    EXPECT_FALSE(xml::detail::valid_declaration(string_view("version=\"1.0\" mode=\"x\"")));

    EXPECT_TRUE(xml::detail::valid_number(string_view("0")));
    EXPECT_TRUE(xml::detail::valid_number(string_view("-0.5")));
    EXPECT_TRUE(xml::detail::valid_number(string_view("10E+2")));
    EXPECT_FALSE(xml::detail::valid_number(string_view("00")));
    EXPECT_FALSE(xml::detail::valid_number(string_view("a1")));
}

TEST(XmlCoverageTest, LowLevelUtf8AndCharacterReferenceBranches)
{
    using namespace castle::serialization::xml;

    uint32_t codepoint = 0U;
    castle::size_type length = 0U;
    const char ascii[] = "A";
    EXPECT_TRUE(detail::decode_utf8(ascii, 1U, codepoint, length));
    EXPECT_EQ(codepoint, static_cast<uint32_t>('A'));
    const char two[] = "\xC2\xA2";
    EXPECT_TRUE(detail::decode_utf8(two, 2U, codepoint, length));
    const char three[] = "\xE2\x82\xAC";
    EXPECT_TRUE(detail::decode_utf8(three, 3U, codepoint, length));
    const char four[] = "\xF0\x9F\x98\x80";
    EXPECT_TRUE(detail::decode_utf8(four, 4U, codepoint, length));
    const char bad_continuation[] = "\xE2\x28\xA1";
    EXPECT_FALSE(detail::decode_utf8(bad_continuation, 3U, codepoint, length));

    char buffer[16] = {};
    castle::size_type used = 0U;
    EXPECT_TRUE(detail::append_utf8('A', buffer, 16U, used));
    EXPECT_TRUE(detail::append_utf8(0x00A2U, buffer, 16U, used));
    EXPECT_TRUE(detail::append_utf8(0x20ACU, buffer, 16U, used));
    EXPECT_TRUE(detail::append_utf8(0x1F600U, buffer, 16U, used));
    EXPECT_FALSE(detail::append_utf8(0x1U, buffer, 1U, used));
    EXPECT_FALSE(detail::append_utf8(0x110000U, buffer, 8U, used));

    uint32_t reference = 0U;
    castle::size_type position = 0U;
    EXPECT_TRUE(detail::parse_character_reference(string_view("&amp;"), position, reference));
    EXPECT_EQ(reference, static_cast<uint32_t>('&'));
    position = 0U;
    EXPECT_TRUE(detail::parse_character_reference(string_view("&#65;"), position, reference));
    EXPECT_EQ(reference, 65U);
    position = 0U;
    EXPECT_TRUE(detail::parse_character_reference(string_view("&#x1F600;"), position, reference));
    EXPECT_EQ(reference, 0x1F600U);
    position = 0U;
    EXPECT_TRUE(detail::parse_character_reference(string_view("&quot;"), position, reference));
    position = 0U;
    EXPECT_TRUE(detail::parse_character_reference(string_view("&apos;"), position, reference));
    position = 0U;
    EXPECT_FALSE(detail::parse_character_reference(string_view("&bad;"), position, reference));
    position = 0U;
    EXPECT_FALSE(detail::parse_character_reference(string_view("&amp"), position, reference));
}

TEST(XmlCoverageTest, DocumentTypedMutationValidationAndSerializerSpecialNodes)
{
    namespace xml = castle::serialization::xml;
    using document = xml::document<32U, 32U, 1024U, 8U>;

    document doc;
    document::node_id node = document::npos;
    ASSERT_EQ(doc.append_declaration(node, "version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\""), castle::status::ok);
    EXPECT_EQ(doc.append_declaration(node, "version=\"1.0\""), castle::status::already_exists);
    ASSERT_EQ(doc.append_doctype(node, " root [<!ELEMENT root ANY>]"), castle::status::ok);
    ASSERT_EQ(doc.append_comment(node, "prologue"), castle::status::ok);
    ASSERT_EQ(doc.append_processing_instruction(node, "tail"), castle::status::ok);

    document::node_id root = document::npos;
    ASSERT_EQ(doc.make_element(root, "root"), castle::status::ok);
    document::attribute_id attr = document::attribute_npos;
    EXPECT_EQ(doc.add_attribute(attr, root, "a", "&<>\"'\n\r\t"), castle::status::ok);

    document::node_id child = document::npos;
    ASSERT_EQ(doc.make_element(child, "child", root), castle::status::ok);
    EXPECT_EQ(doc.append_text(node, "text", child), castle::status::ok);
    EXPECT_EQ(doc.set_text(child, true), castle::status::ok);
    bool boolean = false;
    EXPECT_EQ(doc.get_text(child, boolean), castle::status::ok);
    EXPECT_TRUE(boolean);
    EXPECT_EQ(doc.set_text(child, false), castle::status::ok);
    EXPECT_EQ(doc.get_text(child, boolean), castle::status::ok);
    EXPECT_FALSE(boolean);

    int32_t integer = 0;
    EXPECT_EQ(doc.set_text(child, -42), castle::status::ok);
    EXPECT_EQ(doc.get_text(child, integer), castle::status::ok);
    EXPECT_EQ(integer, -42);

    document::node_id pi = document::npos;
    EXPECT_EQ(doc.append_processing_instruction(pi, "pi", "", root), castle::status::ok);
    document::node_id comment = document::npos;
    EXPECT_EQ(doc.append_comment(comment, "inside", root), castle::status::ok);
    document::node_id cdata = document::npos;
    EXPECT_EQ(doc.append_cdata(cdata, "cdata < & text", root), castle::status::ok);

    string<2048U> output;
    const xml::result r = xml::serialize(doc, output, true, 0U);
    ASSERT_TRUE(r.succeeded());
    EXPECT_NE(output.view().find("<?xml version=\"1.0\""), string_view::npos);
    EXPECT_NE(output.view().find("<!DOCTYPE"), string_view::npos);
    EXPECT_NE(output.view().find("<!--inside-->"), string_view::npos);
    EXPECT_NE(output.view().find("<?pi?>"), string_view::npos);
    EXPECT_NE(output.view().find("&amp;"), string_view::npos);
    EXPECT_NE(output.view().find("&quot;"), string_view::npos);
    EXPECT_NE(output.view().find("&#10;"), string_view::npos);
}

TEST(XmlCoverageTest, SerializerOverflowAtMajorWriteStages)
{
    namespace xml = castle::serialization::xml;
    using document = xml::document<16U, 16U, 256U, 4U>;
    document doc;
    document::node_id root = document::npos;
    ASSERT_EQ(doc.make_element(root, "root"), castle::status::ok);
    document::node_id child = document::npos;
    ASSERT_EQ(doc.make_element(child, "child", root), castle::status::ok);
    ASSERT_EQ(doc.append_text(child, "x", child), castle::status::ok);

    string<1U> tiny;
    const xml::result r = xml::serialize(doc, tiny, false);
    EXPECT_EQ(r.status, castle::status::full);
    EXPECT_EQ(r.code, xml::error_code::output_full);
    EXPECT_TRUE(tiny.empty());

    document empty_element;
    ASSERT_EQ(empty_element.make_element(root, "x"), castle::status::ok);
    string<4U> self_close;
    ASSERT_TRUE(xml::serialize(empty_element, self_close).succeeded());
    EXPECT_EQ(self_close.view(), string_view("<x/>"));
}

TEST(XmlCoverageTest, ParserStateAndMarkupFailureFamilies)
{
    namespace xml = castle::serialization::xml;
    using document = xml::document<32U, 16U, 256U, 4U>;

    const char* invalid_inputs[] =
    {
        "<?xml?>",
        "<?xml version=\"1.0\"",
        "<?pi",
        "<?pi bad?>",
        "<!--unterminated",
        "<![CDATA[unterminated",
        "<!DOCTYPE>",
        "<!DOCTYPE root",
        "<!DOCTYPE root [ 'unterminated]>",
        "<root/ x>",
        "</>",
        "<root><child></",
        "<root>text</root><other/>",
        "<root><?pi ",
        "<root><![CDATA[x"
    };

    for (const char* input : invalid_inputs)
    {
        document doc;
        EXPECT_FALSE(xml::parse(doc, string_view(input)).succeeded());
    }

    document rootless;
    EXPECT_EQ(xml::parse(rootless, string_view("<!--comment-->" )).code, xml::error_code::unexpected_token);
    document whitespace_only;
    EXPECT_EQ(xml::parse(whitespace_only, string_view(" \n\t")).code, xml::error_code::unexpected_token);

    document valid_doctype;
    EXPECT_TRUE(xml::parse(valid_doctype, string_view("<!DOCTYPE root [ <!ELEMENT root ANY> ]><root/>")).succeeded());
}


TEST(XmlCoverageTest, DetailCharacterAndNumericBoundaryPaths)
{
    namespace detail = castle::serialization::xml::detail;

    EXPECT_TRUE(detail::equal_ci(string_view("AbC"), string_view("aBc")));
    EXPECT_FALSE(detail::equal_ci(string_view("AbC"), string_view("abD")));
    EXPECT_FALSE(detail::equal_ci(string_view("ab"), string_view("abc")));
    EXPECT_TRUE(detail::is_space(' '));
    EXPECT_TRUE(detail::is_space('\t'));
    EXPECT_FALSE(detail::is_space('x'));
    EXPECT_TRUE(detail::is_ascii_name_start('A'));
    EXPECT_TRUE(detail::is_ascii_name_start('z'));
    EXPECT_TRUE(detail::is_ascii_name_start('_'));
    EXPECT_TRUE(detail::is_ascii_name_start(':'));
    EXPECT_FALSE(detail::is_ascii_name_start('1'));
    EXPECT_TRUE(detail::is_ascii_name_char('9'));
    EXPECT_TRUE(detail::is_ascii_name_char('-'));
    EXPECT_TRUE(detail::is_ascii_name_char('.'));
    EXPECT_FALSE(detail::is_ascii_name_char('!'));
    EXPECT_TRUE(detail::is_hex('9'));
    EXPECT_TRUE(detail::is_hex('f'));
    EXPECT_TRUE(detail::is_hex('F'));
    EXPECT_FALSE(detail::is_hex('g'));
    EXPECT_EQ(detail::hex_value('9'), 9U);
    EXPECT_EQ(detail::hex_value('a'), 10U);
    EXPECT_EQ(detail::hex_value('A'), 10U);
    EXPECT_TRUE(detail::is_decimal('5'));
    EXPECT_FALSE(detail::is_decimal('x'));

    const char ascii[] = "A";
    const char two[] = "\xC2\xA2";
    const char three[] = "\xE2\x82\xAC";
    const char four[] = "\xF0\x9F\x98\x80";
    const char invalid_lead[] = "\xFF";
    EXPECT_EQ(detail::utf8_sequence_length(ascii, 0U), 0U);
    EXPECT_EQ(detail::utf8_sequence_length(two, 1U), 0U);
    EXPECT_EQ(detail::utf8_sequence_length(three, 2U), 0U);
    EXPECT_EQ(detail::utf8_sequence_length(four, 3U), 0U);
    EXPECT_EQ(detail::utf8_sequence_length(invalid_lead, 1U), 0U);

    uint32_t codepoint = 0U;
    castle::size_type length = 0U;
    EXPECT_FALSE(detail::decode_utf8(two, 1U, codepoint, length));
    const char overlong_three[] = "\xE0\x80\x80";
    const char overlong_four[] = "\xF0\x80\x80\x80";
    const char surrogate[] = "\xED\xA0\x80";
    const char too_large[] = "\xF4\x90\x80\x80";
    EXPECT_FALSE(detail::decode_utf8(overlong_three, 3U, codepoint, length));
    EXPECT_FALSE(detail::decode_utf8(overlong_four, 4U, codepoint, length));
    EXPECT_FALSE(detail::decode_utf8(surrogate, 3U, codepoint, length));
    EXPECT_FALSE(detail::decode_utf8(too_large, 4U, codepoint, length));

    char buffer[8] = {};
    castle::size_type used = 0U;
    EXPECT_FALSE(detail::append_utf8('A', buffer, 0U, used));
    EXPECT_TRUE(detail::append_utf8(0x00A2U, buffer, 8U, used));
    used = 0U;
    EXPECT_FALSE(detail::append_utf8(0x00A2U, buffer, 1U, used));
    EXPECT_FALSE(detail::append_utf8(0x20ACU, buffer, 2U, used));
    EXPECT_FALSE(detail::append_utf8(0x1F600U, buffer, 3U, used));

    EXPECT_FALSE(detail::valid_number(string_view("")));
    EXPECT_FALSE(detail::valid_number(string_view("-")));
    EXPECT_FALSE(detail::valid_number(string_view("01")));
    EXPECT_FALSE(detail::valid_number(string_view("1.")));
    EXPECT_FALSE(detail::valid_number(string_view(".1")));
    EXPECT_FALSE(detail::valid_number(string_view("1e")));
    EXPECT_FALSE(detail::valid_number(string_view("1e+")));
    EXPECT_TRUE(detail::valid_number(string_view("10.25e-2")));

    int32_t integer = 0;
    EXPECT_EQ(detail::parse_integer(string_view(), integer), castle::status::invalid_argument);
    EXPECT_EQ(detail::parse_integer(string_view("+1"), integer), castle::status::invalid_argument);
    EXPECT_EQ(detail::parse_integer(string_view("-"), integer), castle::status::invalid_argument);
    EXPECT_EQ(detail::parse_integer(string_view("12x"), integer), castle::status::invalid_argument);
    uint32_t unsigned_value = 0U;
    EXPECT_EQ(detail::parse_integer(string_view("-1"), unsigned_value), castle::status::invalid_argument);
    EXPECT_EQ(detail::parse_integer(string_view("2147483648"), integer), castle::status::out_of_range);

    double floating = 0.0;
    EXPECT_EQ(detail::parse_floating(string_view("12.50e-2"), floating), castle::status::ok);
    EXPECT_DOUBLE_EQ(floating, 0.125);
    EXPECT_EQ(detail::parse_floating(string_view("1e1025"), floating), castle::status::out_of_range);
}

TEST(XmlCoverageTest, SerializerEscapingCapacityAndCdataSplit)
{
    namespace xml = castle::serialization::xml;

    string<8U> output;
    EXPECT_EQ(xml::detail::append_escaped(output, string_view("&"), false), castle::status::ok);
    output.clear();
    string<2U> amp_full;
    EXPECT_EQ(xml::detail::append_escaped(amp_full, string_view("&"), false), castle::status::full);
    string<2U> lt_full;
    EXPECT_EQ(xml::detail::append_escaped(lt_full, string_view("<"), false), castle::status::full);
    string<2U> gt_full;
    EXPECT_EQ(xml::detail::append_escaped(gt_full, string_view(">"), false), castle::status::full);
    string<2U> quote_full;
    EXPECT_EQ(xml::detail::append_escaped(quote_full, string_view("\""), true), castle::status::full);
    string<2U> apos_full;
    EXPECT_EQ(xml::detail::append_escaped(apos_full, string_view("'"), true), castle::status::full);
    string<2U> lf_full;
    EXPECT_EQ(xml::detail::append_escaped(lf_full, string_view("\n"), true), castle::status::full);
    string<2U> cr_full;
    EXPECT_EQ(xml::detail::append_escaped(cr_full, string_view("\r"), true), castle::status::full);
    string<2U> tab_full;
    EXPECT_EQ(xml::detail::append_escaped(tab_full, string_view("\t"), true), castle::status::full);
    string<1U> ordinary_full;
    EXPECT_EQ(xml::detail::append_escaped(ordinary_full, string_view("x"), false), castle::status::ok);
    ordinary_full.clear();
    EXPECT_EQ(xml::detail::append_escaped(ordinary_full, string_view("xy"), false), castle::status::full);

    using document = xml::document<16U, 16U, 256U, 4U>;
    // The serializer contains a split path for an embedded "]]>" marker, but the
    // public append_cdata API rejects that marker, so that serializer branch is
    // intentionally unreachable without bypassing the public invariant.
    document cdata_doc;
    document::node_id root = document::npos;
    ASSERT_EQ(cdata_doc.make_element(root, "root"), castle::status::ok);
    document::node_id cdata = document::npos;
    ASSERT_EQ(cdata_doc.append_cdata(cdata, "left ]] middle", root), castle::status::ok);
    string<128U> serialized;
    ASSERT_TRUE(xml::serialize(cdata_doc, serialized).succeeded());

    document invalid_cdata;
    ASSERT_EQ(invalid_cdata.make_element(root, "root"), castle::status::ok);
    EXPECT_EQ(invalid_cdata.append_cdata(cdata, "bad ]]> value", root), castle::status::invalid_argument);
}


TEST(XmlCoverageTest, ParserAttributeNodeAndStorageCapacityFailures)
{
    namespace xml = castle::serialization::xml;

    using attr_document = xml::document<8U, 1U, 128U, 4U>;
    attr_document attrs;
    EXPECT_EQ(xml::parse(attrs, string_view("<root a=\"1\" b=\"2\"/>")).code, xml::error_code::capacity);

    using small_document = xml::document<8U, 8U, 2U, 4U>;
    small_document small;
    EXPECT_EQ(xml::parse(small, string_view("<r>abc</r>")).status, castle::status::full);

    small_document entity_small;
    EXPECT_EQ(xml::parse(entity_small, string_view("<r>&amp;</r>")).status, castle::status::full);

    using two_node_document = xml::document<2U, 8U, 128U, 4U>;
    two_node_document text_rollback;
    EXPECT_EQ(xml::parse(text_rollback, string_view("<r><b/>c</r>")).status, castle::status::full);

    using one_node_document = xml::document<1U, 8U, 128U, 4U>;
    one_node_document comment_capacity;
    EXPECT_EQ(xml::parse(comment_capacity, string_view("<r><!--c--></r>")).status, castle::status::full);
    one_node_document cdata_capacity;
    EXPECT_EQ(xml::parse(cdata_capacity, string_view("<r><![CDATA[c]]></r>")).status, castle::status::full);
    one_node_document pi_capacity;
    EXPECT_EQ(xml::parse(pi_capacity, string_view("<r><?p?></r>")).status, castle::status::full);

    one_node_document short_end;
    EXPECT_EQ(xml::parse(short_end, string_view("<r></r")).code, xml::error_code::unexpected_end);
    one_node_document bad_pi;
    EXPECT_EQ(xml::parse(bad_pi, string_view("<?p/bad?>")).code, xml::error_code::invalid_processing_instruction);
    one_node_document bad_pi_data;
    const char nul_pi[] = {'<','?','p',' ','x','\0','?','>'};
    EXPECT_EQ(xml::parse(bad_pi_data, string_view(nul_pi, sizeof(nul_pi))).code, xml::error_code::invalid_processing_instruction);

    one_node_document bad_cdata;
    const char nul_cdata[] = {'<','r','>','<','!','[','C','D','A','T','A','[','x','\0',']',']','>','<','/','r','>'};
    EXPECT_EQ(xml::parse(bad_cdata, string_view(nul_cdata, sizeof(nul_cdata))).code, xml::error_code::invalid_cdata);

    one_node_document declaration_bad;
    EXPECT_EQ(xml::parse(declaration_bad, string_view("<?xml?><r/>")).code, xml::error_code::invalid_declaration);
}

} // namespace
