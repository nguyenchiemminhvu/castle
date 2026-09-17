/**
 * Castle 2.0 sample: <castle/serialization/xml.hpp>
 *
 * Scenario: parse a small device configuration, read typed values, modify one
 * attribute, then serialize it again without using dynamic memory.
 */
#include "sample_support.hpp"

#include "castle/serialization/xml.hpp"

#include <stdint.h>

int main()
{
    using xml_document = castle::serialization::xml::document<32U, 32U, 1024U, 8U>;
    using xml_string = castle::container::string<1024U>;
    using xml_view = castle::container::string_view;

    const char* source =
        "<?xml version=\"1.0\"?>"
        "<device id=\"42\" enabled=\"true\">"
            "<name>castle</name>"
            "<sample-rate>1000</sample-rate>"
        "</device>";

    xml_document document;
    const castle::serialization::xml::result parsed =
        castle::serialization::xml::parse(document, xml_view(source));
    CASTLE_SAMPLE_CHECK(parsed.succeeded());

    const xml_document::node_id root = document.root();
    CASTLE_SAMPLE_CHECK(root != xml_document::npos);

    uint32_t id = 0U;
    bool enabled = false;
    CASTLE_SAMPLE_CHECK(document.get_attribute(root, xml_view("id"), id) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(document.get_attribute(root, xml_view("enabled"), enabled) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(id == 42U);
    CASTLE_SAMPLE_CHECK(enabled);

    const xml_document::node_id name_node = document.find_child(root, xml_view("name"));
    CASTLE_SAMPLE_CHECK(name_node != xml_document::npos);

    const xml_document::node_id rate_node = document.find_child(root, xml_view("sample-rate"));
    CASTLE_SAMPLE_CHECK(rate_node != xml_document::npos);

    uint32_t sample_rate = 0U;
    CASTLE_SAMPLE_CHECK(document.get_text(rate_node, sample_rate) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(sample_rate == 1000U);

    CASTLE_SAMPLE_CHECK(document.set_attribute(root, xml_view("id"), 43U) == castle::status::ok);

    xml_string output;
    const castle::serialization::xml::result serialized =
        castle::serialization::xml::serialize(document, output, true, 2U);
    CASTLE_SAMPLE_CHECK(serialized.succeeded());

    // Keep the local view alive to make the intended zero-copy access explicit.
    const xml_view result_view = output.view();
    CASTLE_SAMPLE_CHECK(!result_view.empty());
    return 0;
}
