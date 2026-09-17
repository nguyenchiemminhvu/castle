#include "sample_support.hpp"

#include "castle/serialization/ini.hpp"

#include <stdint.h>

// Scenario: parse a fixed-capacity device configuration, read typed values,
// update one setting, and serialize the result without heap allocation.
int main()
{
    using config_t = castle::serialization::ini::document<16U, 24U, 32U, 96U>;

    const castle::container::string_view source(
        "# castle device configuration\n"
        "device = castle-demo\n"
        "[network]\n"
        "enabled = yes\n"
        "port = 1883\n"
        "host = \"broker.example.com\"\n");

    config_t config;
    const castle::serialization::ini::result parse_result =
        castle::serialization::ini::parse(config, source);
    CASTLE_SAMPLE_CHECK(parse_result.succeeded());

    bool enabled = false;
    uint16_t port = 0U;
    CASTLE_SAMPLE_CHECK(config.get_bool("network", "enabled", enabled) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(config.get_integer("network", "port", port) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(enabled);
    CASTLE_SAMPLE_CHECK(port == 1883U);

    CASTLE_SAMPLE_CHECK(
        config.set_integer("network", "port", static_cast<uint16_t>(1884U)) == castle::status::ok);

    castle::container::string<512U> serialized;
    const castle::serialization::ini::result write_result =
        castle::serialization::ini::serialize(config, serialized);
    CASTLE_SAMPLE_CHECK(write_result.succeeded());
    CASTLE_SAMPLE_CHECK(serialized.find("[network]") != castle::container::string_view::npos);
    CASTLE_SAMPLE_CHECK(serialized.find("port = 1884") != castle::container::string_view::npos);

    return 0;
}
