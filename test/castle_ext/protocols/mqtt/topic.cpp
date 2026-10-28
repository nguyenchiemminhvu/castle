#include <gtest/gtest.h>
#include "castle_ext/protocols/mqtt/topic.hpp"

using namespace castle::protocols::mqtt;
using castle::container::string_view;

TEST(MqttTopic, ValidatesTopicNames)
{
    EXPECT_EQ(validate_topic_name("a"), castle::status::ok);
    EXPECT_EQ(validate_topic_name("a/b/c"), castle::status::ok);
    EXPECT_EQ(validate_topic_name(""), castle::status::invalid_argument);
    EXPECT_EQ(validate_topic_name("a/+"), castle::status::invalid_argument);
    EXPECT_EQ(validate_topic_name("a/#"), castle::status::invalid_argument);
    const char nul[] = {'a', '\0', 'b'};
    EXPECT_EQ(validate_topic_name(string_view(nul, sizeof(nul))), castle::status::invalid_argument);
    const char invalid[] = {static_cast<char>(0xF0), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80)};
    EXPECT_EQ(validate_topic_name(string_view(invalid, sizeof(invalid))), castle::status::invalid_argument);
}

TEST(MqttTopic, ValidatesWholeLevelWildcards)
{
    EXPECT_EQ(validate_topic_filter("+"), castle::status::ok);
    EXPECT_EQ(validate_topic_filter("a/+"), castle::status::ok);
    EXPECT_EQ(validate_topic_filter("a/#"), castle::status::ok);
    EXPECT_EQ(validate_topic_filter("#"), castle::status::ok);
    EXPECT_EQ(validate_topic_filter("a/+/b/#"), castle::status::ok);
    EXPECT_EQ(validate_topic_filter("a+b"), castle::status::invalid_argument);
    EXPECT_EQ(validate_topic_filter("a/#/b"), castle::status::invalid_argument);
    EXPECT_EQ(validate_topic_filter("a/b#"), castle::status::invalid_argument);
    EXPECT_EQ(validate_topic_filter(""), castle::status::invalid_argument);
}

TEST(MqttTopic, MatchesHierarchicalFiltersAndSystemTopics)
{
    EXPECT_TRUE(topic_matches("home/+/status", "home/kitchen/status"));
    EXPECT_FALSE(topic_matches("home/+/status", "home/kitchen/sensor/status"));
    EXPECT_TRUE(topic_matches("devices/#", "devices"));
    EXPECT_TRUE(topic_matches("devices/#", "devices/a/b"));
    EXPECT_TRUE(topic_matches("#", "ordinary/topic"));
    EXPECT_FALSE(topic_matches("#", "$SYS/broker/uptime"));
    EXPECT_TRUE(topic_matches("$SYS/#", "$SYS/broker/uptime"));
    EXPECT_TRUE(topic_matches("a/+", "a/"));
    EXPECT_FALSE(topic_matches("a/b", "a/c"));
    EXPECT_FALSE(topic_matches("a/#/b", "a/x/b"));
}

TEST(MqttTopic, ValidatesMultiByteUtf8Sequences)
{
    const char two_byte[] = {static_cast<char>(0xC3), static_cast<char>(0xA9)};
    const char three_byte[] = {static_cast<char>(0xE2), static_cast<char>(0x82), static_cast<char>(0xAC)};
    const char four_byte[] = {static_cast<char>(0xF0), static_cast<char>(0x9F), static_cast<char>(0x98), static_cast<char>(0x80)};
    EXPECT_EQ(validate_topic_name(string_view(two_byte, sizeof(two_byte))), castle::status::ok);
    EXPECT_EQ(validate_topic_name(string_view(three_byte, sizeof(three_byte))), castle::status::ok);
    EXPECT_EQ(validate_topic_name(string_view(four_byte, sizeof(four_byte))), castle::status::ok);
    EXPECT_EQ(validate_topic_filter(string_view(three_byte, sizeof(three_byte))), castle::status::ok);

    const char truncated[] = {static_cast<char>(0xC3)};
    EXPECT_EQ(validate_topic_name(string_view(truncated, sizeof(truncated))), castle::status::invalid_argument);
    const char bad_continuation[] = {static_cast<char>(0xC3), 'A'};
    EXPECT_EQ(validate_topic_name(string_view(bad_continuation, sizeof(bad_continuation))), castle::status::invalid_argument);
    const char stray_continuation[] = {static_cast<char>(0x80)};
    EXPECT_EQ(validate_topic_name(string_view(stray_continuation, sizeof(stray_continuation))), castle::status::invalid_argument);
    const char invalid_lead[] = {static_cast<char>(0xFF)};
    EXPECT_EQ(validate_topic_name(string_view(invalid_lead, sizeof(invalid_lead))), castle::status::invalid_argument);
    const char overlong[] = {static_cast<char>(0xC0), static_cast<char>(0x80)};
    EXPECT_EQ(validate_topic_name(string_view(overlong, sizeof(overlong))), castle::status::invalid_argument);
    const char surrogate[] = {static_cast<char>(0xED), static_cast<char>(0xA0), static_cast<char>(0x80)};
    EXPECT_EQ(validate_topic_name(string_view(surrogate, sizeof(surrogate))), castle::status::invalid_argument);
    const char beyond_unicode[] = {static_cast<char>(0xF4), static_cast<char>(0x90), static_cast<char>(0x80), static_cast<char>(0x80)};
    EXPECT_EQ(validate_topic_name(string_view(beyond_unicode, sizeof(beyond_unicode))), castle::status::invalid_argument);
    const char fdd0[] = {static_cast<char>(0xEF), static_cast<char>(0xB7), static_cast<char>(0x90)};
    EXPECT_EQ(validate_topic_name(string_view(fdd0, sizeof(fdd0))), castle::status::invalid_argument);
    const char fffe[] = {static_cast<char>(0xEF), static_cast<char>(0xBF), static_cast<char>(0xBE)};
    EXPECT_EQ(validate_topic_name(string_view(fffe, sizeof(fffe))), castle::status::invalid_argument);
    const char ffff[] = {static_cast<char>(0xEF), static_cast<char>(0xBF), static_cast<char>(0xBF)};
    EXPECT_EQ(validate_topic_name(string_view(ffff, sizeof(ffff))), castle::status::invalid_argument);
    const char plane_one_fffe[] = {static_cast<char>(0xF0), static_cast<char>(0x9F), static_cast<char>(0xBF), static_cast<char>(0xBE)};
    EXPECT_EQ(validate_topic_name(string_view(plane_one_fffe, sizeof(plane_one_fffe))), castle::status::invalid_argument);

    const char bad_filter[] = {static_cast<char>(0xFF), '/', 'a'};
    EXPECT_EQ(validate_topic_filter(string_view(bad_filter, sizeof(bad_filter))), castle::status::invalid_argument);
    EXPECT_TRUE(detail::valid_utf8(string_view()));
    EXPECT_FALSE(detail::valid_utf8(string_view(nullptr, 2U)));
}

TEST(MqttTopic, MatchesEdgeLevelsAndTrailingSeparators)
{
    EXPECT_FALSE(topic_matches("", "a"));
    EXPECT_FALSE(topic_matches("a/b", "a/+"));
    EXPECT_FALSE(topic_matches("ab", "abc"));
    EXPECT_FALSE(topic_matches("abc", "ab"));
    EXPECT_TRUE(topic_matches("a/", "a/"));
    EXPECT_FALSE(topic_matches("a/", "a"));
    EXPECT_FALSE(topic_matches("a/", "a/b"));
    EXPECT_FALSE(topic_matches("a/b", "a"));
    EXPECT_FALSE(topic_matches("a/b/c", "a"));
    EXPECT_TRUE(topic_matches("a/#", "a"));
    EXPECT_TRUE(topic_matches("+/+", "a/b"));
    EXPECT_FALSE(topic_matches("+/+", "a/b/c"));
}
