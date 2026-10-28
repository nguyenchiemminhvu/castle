#include <gtest/gtest.h>
#include "castle_ext/protocols/mqtt/types.hpp"

using namespace castle::protocols::mqtt;

TEST(MqttTypes, VersionsPacketsAndQosHaveWireValues)
{
    EXPECT_EQ(static_cast<uint8_t>(version::v3_1_0), 3U);
    EXPECT_EQ(static_cast<uint8_t>(version::v3_1_1), 4U);
    EXPECT_EQ(static_cast<uint8_t>(version::v5_0), 5U);
    EXPECT_EQ(static_cast<uint8_t>(packet_type::connect), 1U);
    EXPECT_EQ(static_cast<uint8_t>(packet_type::auth), 15U);
    EXPECT_EQ(static_cast<uint8_t>(qos::exactly_once), 2U);
}

TEST(MqttTypes, PropertyListsAreBoundedAndFindFirstMatchingProperty)
{
    property_list<2U, 12U, 4U> properties;
    EXPECT_TRUE(properties.empty());
    EXPECT_EQ(properties.add_integer(property_id::session_expiry_interval, 17U), castle::status::ok);
    EXPECT_EQ(properties.add_string(property_id::reason_string, "ok"), castle::status::ok);
    EXPECT_TRUE(properties.full());
    ASSERT_NE(properties.find(property_id::session_expiry_interval), nullptr);
    EXPECT_EQ(properties.find(property_id::session_expiry_interval)->integer, 17U);
    EXPECT_EQ(properties.add_integer(property_id::receive_maximum, 1U), castle::status::full);
    EXPECT_EQ(properties.add_string(property_id::content_type, "string-too-long"), castle::status::full);
    properties.clear();
    EXPECT_TRUE(properties.empty());
    EXPECT_EQ(properties.add_string(property_id::reason_string, "string-too-long"), castle::status::out_of_range);
    EXPECT_EQ(properties.add_binary(property_id::correlation_data, nullptr, 1U), castle::status::out_of_range);
}

TEST(MqttTypes, UserPropertiesAndReasonMapping)
{
    property_list<1U, 8U, 4U> properties;
    EXPECT_EQ(properties.add_user_property("k", "v"), castle::status::ok);
    EXPECT_STREQ(properties.entries[0U].string_value, "k");
    EXPECT_STREQ(properties.entries[0U].user_value, "v");
    EXPECT_EQ(to_status(reason_code::unsupported_protocol_version), castle::status::invalid_config);
    EXPECT_EQ(to_status(reason_code::packet_identifier_not_found), castle::status::not_found);
    EXPECT_EQ(to_status(reason_code::server_busy), castle::status::time_out);
    EXPECT_EQ(to_status(static_cast<reason_code>(0xEEU)), castle::status::unknown_error);
}

TEST(MqttTypes, PropertyListRejectsInvalidValuesAndCapacity)
{
    property_list<1U, 12U, 4U> single;
    EXPECT_EQ(single.add_string(property_id::reason_string, castle::container::string_view()), castle::status::out_of_range);
    EXPECT_EQ(single.add_integer(property_id::receive_maximum, 1U), castle::status::ok);
    EXPECT_EQ(single.add_binary(property_id::correlation_data, nullptr, 1U), castle::status::full);
    single.clear();
    const uint8_t blob[] = {1U, 2U, 3U, 4U, 5U};
    EXPECT_EQ(single.add_binary(property_id::correlation_data, nullptr, 1U), castle::status::out_of_range);
    EXPECT_EQ(single.add_binary(property_id::correlation_data, blob, sizeof(blob)), castle::status::out_of_range);
    EXPECT_TRUE(single.empty());
    EXPECT_EQ(single.add_binary(property_id::correlation_data, blob, 0U), castle::status::ok);
    EXPECT_EQ(single.entries[0U].binary_size, 0U);
}

TEST(MqttTypes, PropertyBinaryValuesAreCopied)
{
    property_list<2U, 12U, 4U> properties;
    const uint8_t blob[] = {7U, 8U, 9U};
    ASSERT_EQ(properties.add_binary(property_id::correlation_data, blob, sizeof(blob)), castle::status::ok);
    EXPECT_EQ(properties.count, 1U);
    EXPECT_EQ(properties.entries[0U].binary_size, 3U);
    EXPECT_EQ(properties.entries[0U].binary_value[2U], 9U);
    EXPECT_EQ(properties.find(property_id::correlation_data)->binary_value[0U], 7U);
    property_list<1U, 12U, 4U> empty_binary;
    EXPECT_EQ(empty_binary.add_binary(property_id::correlation_data, nullptr, 0U), castle::status::ok);
    EXPECT_EQ(empty_binary.entries[0U].binary_size, 0U);
}

TEST(MqttTypes, UserPropertyRejectsNullAndOversizedFields)
{
    property_list<2U, 8U, 4U> properties;
    EXPECT_EQ(properties.add_user_property(castle::container::string_view(), "v"), castle::status::out_of_range);
    EXPECT_EQ(properties.add_user_property("k", castle::container::string_view()), castle::status::out_of_range);
    EXPECT_EQ(properties.add_user_property("key-too-long", "v"), castle::status::out_of_range);
    EXPECT_EQ(properties.add_user_property("k", "value-too-long"), castle::status::out_of_range);
    EXPECT_TRUE(properties.empty());
    EXPECT_EQ(properties.add_user_property("k", "v"), castle::status::ok);
    EXPECT_EQ(properties.add_user_property("k2", "v2"), castle::status::ok);
    EXPECT_EQ(properties.add_user_property("k3", "v3"), castle::status::full);
    EXPECT_EQ(properties.add_integer(property_id::receive_maximum, 1U), castle::status::full);
}

TEST(MqttTypes, PropertyFindScansEveryEntry)
{
    property_list<3U, 8U, 4U> properties;
    EXPECT_EQ(properties.find(property_id::reason_string), nullptr);
    ASSERT_EQ(properties.add_integer(property_id::session_expiry_interval, 1U), castle::status::ok);
    ASSERT_EQ(properties.add_integer(property_id::receive_maximum, 2U), castle::status::ok);
    ASSERT_NE(properties.find(property_id::receive_maximum), nullptr);
    EXPECT_EQ(properties.find(property_id::receive_maximum)->integer, 2U);
    EXPECT_EQ(properties.find(property_id::maximum_qos), nullptr);
}

TEST(MqttTypes, ReasonCodesMapToEveryStatusClass)
{
    EXPECT_EQ(to_status(reason_code::success), castle::status::ok);
    EXPECT_EQ(to_status(reason_code::granted_qos_1), castle::status::ok);
    EXPECT_EQ(to_status(reason_code::granted_qos_2), castle::status::ok);
    EXPECT_EQ(to_status(reason_code::no_matching_subscribers), castle::status::ok);
    EXPECT_EQ(to_status(reason_code::no_subscription_existed), castle::status::ok);
    EXPECT_EQ(to_status(reason_code::client_identifier_not_valid), castle::status::invalid_argument);
    EXPECT_EQ(to_status(reason_code::topic_name_invalid), castle::status::invalid_argument);
    EXPECT_EQ(to_status(reason_code::malformed_packet), castle::status::invalid_argument);
    EXPECT_EQ(to_status(reason_code::protocol_error), castle::status::invalid_argument);
    EXPECT_EQ(to_status(reason_code::packet_too_large), castle::status::out_of_range);
    EXPECT_EQ(to_status(reason_code::packet_identifier_in_use), castle::status::full);
    EXPECT_EQ(to_status(reason_code::receive_maximum_exceeded), castle::status::full);
    EXPECT_EQ(to_status(reason_code::server_unavailable), castle::status::time_out);
    EXPECT_EQ(to_status(reason_code::unspecified_error), castle::status::unknown_error);
}

