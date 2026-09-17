#include <gtest/gtest.h>

#include "castle/serialization/xml.hpp"

namespace
{

namespace xml = castle::serialization::xml;
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

TEST(XmlErrorsTest, InvalidRootAndTagSyntax)
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

TEST(XmlErrorsTest, DuplicateAndMalformedAttributes)
{
    document doc;
    EXPECT_EQ(xml::parse(doc, string_view("<root a=\"1\" a=\"2\"/>")).code, xml::error_code::duplicate_attribute);
    EXPECT_EQ(xml::parse(doc, string_view("<root a=\"1\"b=\"2\"/>")).code, xml::error_code::invalid_attribute);
    EXPECT_EQ(xml::parse(doc, string_view("<root a\"1\"/>")).code, xml::error_code::invalid_attribute);
    EXPECT_EQ(xml::parse(doc, string_view("<root a=1/>")).code, xml::error_code::invalid_attribute);
    EXPECT_EQ(xml::parse(doc, string_view("<root a=\"1></root>")).code, xml::error_code::unexpected_end);
}

TEST(XmlErrorsTest, InvalidEntitiesAndCharacters)
{
    document doc;
    EXPECT_EQ(xml::parse(doc, string_view("<root>&unknown;</root>")).code, xml::error_code::invalid_entity);
    EXPECT_EQ(xml::parse(doc, string_view("<root>&#0;</root>")).code, xml::error_code::invalid_entity);
    EXPECT_EQ(xml::parse(doc, string_view("<root>&amp</root>")).code, xml::error_code::invalid_entity);
    EXPECT_EQ(xml::parse(doc, string_view("<root>\x01</root>")).code, xml::error_code::invalid_character);
    const char invalid_utf8[] = "<root>\xF0\x28\x8C\x28</root>";
    EXPECT_EQ(xml::parse(doc, string_view(invalid_utf8)).code, xml::error_code::invalid_character);
}

TEST(XmlErrorsTest, CommentCdataProcessingInstructionAndDeclarationErrors)
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

TEST(XmlErrorsTest, DepthAndCapacity)
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

TEST(XmlDocumentMutationTest, AttributesCommentsCdataAndProcessingInstructions)
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

TEST(XmlDocumentMutationTest, MutationAndTypedAccessWithoutHeap)
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

TEST(XmlDocumentMutationTest, InvalidMutationArguments)
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

TEST(XmlSerializeTest, CompactPrettyMixedContentAndOutputOverflow)
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

TEST(XmlSerializeTest, EmptyDocument)
{
    document doc;
    castle::container::string<128U> out("not-empty");
    const xml::result r = xml::serialize(doc, out);
    EXPECT_EQ(r.status, castle::status::empty);
    EXPECT_EQ(r.code, xml::error_code::none);
    EXPECT_TRUE(out.empty());
}

} // namespace
