#include "sample_support.hpp"

#include "castle/serialization/json.hpp"

int main()
{
    using document = castle::serialization::json::document<32U, 512U, 8U>;
    using node_id = document::node_id;
    using output_string = castle::container::string<512U>;

    const castle::container::string_view source(
        "{\"device\":{\"name\":\"castle\",\"enabled\":true,\"baud\":115200},\"channels\":[1,2]}");

    document parsed;
    const castle::serialization::json::result parsed_result = castle::serialization::json::parse(parsed, source);
    CASTLE_SAMPLE_CHECK(parsed_result.succeeded());

    const node_id root = parsed.root();
    const node_id device = parsed.find(root, "device");
    const node_id name = parsed.find(device, "name");
    const node_id enabled = parsed.find(device, "enabled");
    const node_id baud = parsed.find(device, "baud");
    const node_id channels = parsed.find(root, "channels");
    CASTLE_SAMPLE_CHECK(device != document::npos);
    CASTLE_SAMPLE_CHECK(channels != document::npos);

    castle::container::string_view name_text;
    bool enabled_value = false;
    int baud_value = 0;
    CASTLE_SAMPLE_CHECK(parsed.get_string(name, name_text) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(parsed.get_bool(enabled, enabled_value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(parsed.get_integer(baud, baud_value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(enabled_value);
    CASTLE_SAMPLE_CHECK(baud_value == 115200);
    CASTLE_SAMPLE_CHECK(parsed.at(channels, 1U) != document::npos);

    document built;
    node_id built_root = document::npos;
    node_id built_device = document::npos;
    node_id built_name = document::npos;
    node_id built_enabled = document::npos;
    node_id built_gain = document::npos;
    node_id built_channels = document::npos;
    node_id built_channel = document::npos;

    CASTLE_SAMPLE_CHECK(built.make_object(built_root) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(built.make_object(built_device, built_root, "device") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(built.set_string(built_name, "castle", built_device, "name") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(built.set_bool(built_enabled, true, built_device, "enabled") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(built.set_floating(built_gain, 1.25, built_device, "gain", 2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(built.make_array(built_channels, built_root, "channels") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(built.set_integer(built_channel, 3, built_channels) == castle::status::ok);

    output_string output;
    const castle::serialization::json::result serialized =
        castle::serialization::json::serialize(built, output, true, 2U);
    CASTLE_SAMPLE_CHECK(serialized.succeeded());
    CASTLE_SAMPLE_CHECK(output.find("\"gain\": 1.25") != castle::container::string_view::npos);
    CASTLE_SAMPLE_CHECK(output.find("\"channels\": [") != castle::container::string_view::npos);
    return 0;
}
