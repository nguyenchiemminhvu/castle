#include "sample_support.hpp"
#include "castle_ext/protocols/mqtt/topic.hpp"

using namespace castle::protocols::mqtt;
using castle::container::string_view;

int main()
{
    CASTLE_SAMPLE_CHECK(validate_topic_name("sensors/temperature/livingroom") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(validate_topic_name("sensors/+/temperature") == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(validate_topic_filter("home/+/status") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(validate_topic_filter("devices/#") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(validate_topic_filter("devices/#/state") == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(validate_topic_filter("a+bc") == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(topic_matches("home/+/status", "home/kitchen/status"));
    CASTLE_SAMPLE_CHECK(topic_matches("devices/#", "devices/a/b"));
    CASTLE_SAMPLE_CHECK(topic_matches("devices/#", "devices"));
    CASTLE_SAMPLE_CHECK(!topic_matches("home/+/status", "home/status"));
    CASTLE_SAMPLE_CHECK(!topic_matches("#", "$SYS/broker/uptime"));
    CASTLE_SAMPLE_CHECK(topic_matches("$SYS/#", "$SYS/broker/uptime"));
    CASTLE_SAMPLE_CHECK(!topic_matches(string_view(), "a/b"));
    const char invalid_utf8[] = {static_cast<char>(0xC0), static_cast<char>(0x80)};
    CASTLE_SAMPLE_CHECK(validate_topic_name(string_view(invalid_utf8, sizeof(invalid_utf8))) == castle::status::invalid_argument);
    return 0;
}
