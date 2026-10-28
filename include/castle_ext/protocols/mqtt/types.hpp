// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file types.hpp
 * @brief Fixed-capacity MQTT protocol types and configuration views.
 */
#ifndef CASTLE_EXT_PROTOCOLS_MQTT_TYPES_HPP
#define CASTLE_EXT_PROTOCOLS_MQTT_TYPES_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string_view.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace mqtt
{

/** @brief MQTT wire protocol revision. */
enum class version : uint8_t
{
    v3_1_0 = 3U,
    v3_1_1 = 4U,
    v5_0 = 5U
};

/** @brief MQTT Quality of Service level. */
enum class qos : uint8_t
{
    at_most_once = 0U,
    at_least_once = 1U,
    exactly_once = 2U
};

/** @brief MQTT fixed-header packet type values. */
enum class packet_type : uint8_t
{
    reserved = 0U,
    connect = 1U,
    connack = 2U,
    publish = 3U,
    puback = 4U,
    pubrec = 5U,
    pubrel = 6U,
    pubcomp = 7U,
    subscribe = 8U,
    suback = 9U,
    unsubscribe = 10U,
    unsuback = 11U,
    pingreq = 12U,
    pingresp = 13U,
    disconnect = 14U,
    auth = 15U
};

/** @brief MQTT 5.0 packet reason codes used by CONNECT-family and acknowledgement packets. */
enum class reason_code : uint8_t
{
    success = 0x00U,
    normal_disconnection = 0x00U,
    granted_qos_1 = 0x01U,
    granted_qos_2 = 0x02U,
    disconnect_with_will = 0x04U,
    no_matching_subscribers = 0x10U,
    no_subscription_existed = 0x11U,
    continue_authentication = 0x18U,
    re_authenticate = 0x19U,
    unspecified_error = 0x80U,
    malformed_packet = 0x81U,
    protocol_error = 0x82U,
    implementation_specific_error = 0x83U,
    unsupported_protocol_version = 0x84U,
    client_identifier_not_valid = 0x85U,
    bad_user_name_or_password = 0x86U,
    not_authorized = 0x87U,
    server_unavailable = 0x88U,
    server_busy = 0x89U,
    banned = 0x8AU,
    bad_authentication_method = 0x8CU,
    topic_name_invalid = 0x90U,
    packet_identifier_in_use = 0x91U,
    packet_identifier_not_found = 0x92U,
    receive_maximum_exceeded = 0x93U,
    topic_alias_invalid = 0x94U,
    packet_too_large = 0x95U,
    message_rate_too_high = 0x96U,
    quota_exceeded = 0x97U,
    administrative_action = 0x98U,
    payload_format_invalid = 0x99U,
    retain_not_supported = 0x9AU,
    qos_not_supported = 0x9BU,
    use_another_server = 0x9CU,
    server_moved = 0x9DU,
    shared_subscriptions_not_supported = 0x9EU,
    connection_rate_exceeded = 0x9FU,
    maximum_connect_time = 0xA0U,
    subscription_identifiers_not_supported = 0xA1U,
    wildcard_subscriptions_not_supported = 0xA2U
};

/** @brief MQTT 5.0 property identifiers. */
enum class property_id : uint8_t
{
    payload_format_indicator = 0x01U,
    message_expiry_interval = 0x02U,
    content_type = 0x03U,
    response_topic = 0x08U,
    correlation_data = 0x09U,
    subscription_identifier = 0x0BU,
    session_expiry_interval = 0x11U,
    assigned_client_identifier = 0x12U,
    server_keep_alive = 0x13U,
    authentication_method = 0x15U,
    authentication_data = 0x16U,
    request_problem_information = 0x17U,
    will_delay_interval = 0x18U,
    request_response_information = 0x19U,
    response_information = 0x1AU,
    server_reference = 0x1CU,
    reason_string = 0x1FU,
    receive_maximum = 0x21U,
    topic_alias_maximum = 0x22U,
    topic_alias = 0x23U,
    maximum_qos = 0x24U,
    retain_available = 0x25U,
    user_property = 0x26U,
    maximum_packet_size = 0x27U,
    wildcard_subscription_available = 0x28U,
    subscription_identifier_available = 0x29U,
    shared_subscription_available = 0x2AU
};

/** @brief Property value representation; every field has inline, compile-time capacity. */
template <size_type StringCapacity = 64U, size_type BinaryCapacity = 32U>
struct property
{
    property_id id = property_id::reason_string;
    uint32_t integer = 0U;
    char string_value[StringCapacity]{};
    uint8_t binary_value[BinaryCapacity]{};
    size_type binary_size = 0U;
    char user_value[StringCapacity]{}; // second UTF-8 value for User Property
};

/** @brief Fixed-capacity MQTT 5.0 property collection. */
template <size_type Capacity = 8U, size_type StringCapacity = 64U, size_type BinaryCapacity = 32U>
struct property_list
{
    static_assert(Capacity > 0U, "MQTT property capacity must be non-zero");
    static_assert(StringCapacity > 0U, "MQTT property string capacity must be non-zero");
    static_assert(BinaryCapacity > 0U, "MQTT property binary capacity must be non-zero");
    property<StringCapacity, BinaryCapacity> entries[Capacity]{};
    size_type count = 0U;

    void clear() CASTLE_NOEXCEPT { count = 0U; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return count == 0U; }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return count == Capacity; }
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return count; }
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return Capacity; }

    castle::status add_integer(property_id id, uint32_t value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return castle::status::full;
        }
        entries[count].id = id;
        entries[count].integer = value;
        ++count;
        return castle::status::ok;
    }

    castle::status add_string(property_id id, castle::container::string_view value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return castle::status::full;
        }
        if (value.data() == nullptr || value.size() >= StringCapacity)
        {
            return castle::status::out_of_range;
        }
        property<StringCapacity, BinaryCapacity>& target = entries[count];
        target.id = id;
        for (size_type i = 0U; i < value.size(); ++i)
        {
            target.string_value[i] = value[i];
        }
        target.string_value[value.size()] = '\0';
        ++count;
        return castle::status::ok;
    }

    castle::status add_binary(property_id id, uint8_t CASTLE_CONST* data, size_type size) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return castle::status::full;
        }
        if ((data == nullptr && size != 0U) || size > BinaryCapacity)
        {
            return castle::status::out_of_range;
        }
        property<StringCapacity, BinaryCapacity>& target = entries[count];
        target.id = id;
        target.binary_size = size;
        for (size_type i = 0U; i < size; ++i)
        {
            target.binary_value[i] = data[i];
        }
        ++count;
        return castle::status::ok;
    }

    castle::status add_user_property(castle::container::string_view key,
                                     castle::container::string_view value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return castle::status::full;
        }
        if (key.data() == nullptr || value.data() == nullptr ||
            key.size() >= StringCapacity || value.size() >= StringCapacity)
        {
            return castle::status::out_of_range;
        }
        property<StringCapacity, BinaryCapacity>& target = entries[count];
        target.id = property_id::user_property;
        for (size_type i = 0U; i < key.size(); ++i)
        {
            target.string_value[i] = key[i];
        }
        target.string_value[key.size()] = '\0';
        for (size_type i = 0U; i < value.size(); ++i)
        {
            target.user_value[i] = value[i];
        }
        target.user_value[value.size()] = '\0';
        ++count;
        return castle::status::ok;
    }

    property<StringCapacity, BinaryCapacity> CASTLE_CONST* find(property_id id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < count; ++i)
        {
            if (entries[i].id == id)
            {
                return &entries[i];
            }
        }
        return nullptr;
    }
};

/** @brief Non-owning message delivered to an MQTT application callback. */
struct message_view
{
    castle::container::string_view topic{};
    uint8_t CASTLE_CONST* payload = nullptr;
    size_type payload_size = 0U;
    qos quality = qos::at_most_once;
    bool duplicate = false;
    bool retain = false;
    uint16_t packet_identifier = 0U;
};

/** @brief Optional Last Will and Testament configuration (all data is borrowed until CONNECT completes). */
struct will_options
{
    castle::container::string_view topic{};
    uint8_t CASTLE_CONST* payload = nullptr;
    size_type payload_size = 0U;
    qos quality = qos::at_most_once;
    bool retain = false;
};

/** @brief CONNECT parameters shared by MQTT 3.x and 5.0. */
template <size_type PropertyCapacity = 8U, size_type PropertyStringCapacity = 64U,
          size_type PropertyBinaryCapacity = 32U>
struct connect_options
{
    version protocol_version = version::v3_1_1;
    castle::container::string_view client_id{"castle-client"};
    bool clean_session = true; // MQTT 5 calls this Clean Start
    uint16_t keep_alive_seconds = 30U;
    bool has_will = false;
    will_options will{};
    castle::container::string_view username{};
    castle::container::string_view password{};
    bool has_username = false;
    bool has_password = false;
    property_list<PropertyCapacity, PropertyStringCapacity, PropertyBinaryCapacity> properties{};
    property_list<PropertyCapacity, PropertyStringCapacity, PropertyBinaryCapacity> will_properties{};
};

/** @brief Maps common MQTT 5.0 failure reasons onto CASTLE's compact status domain. */
CASTLE_CONSTEXPR castle::status to_status(reason_code reason) CASTLE_NOEXCEPT
{
    switch (reason)
    {
        case reason_code::success:
        case reason_code::granted_qos_1:
        case reason_code::granted_qos_2:
        case reason_code::no_matching_subscribers:
        case reason_code::no_subscription_existed:
            return castle::status::ok;
        case reason_code::unsupported_protocol_version:
            return castle::status::invalid_config;
        case reason_code::client_identifier_not_valid:
        case reason_code::topic_name_invalid:
        case reason_code::malformed_packet:
        case reason_code::protocol_error:
            return castle::status::invalid_argument;
        case reason_code::packet_too_large:
            return castle::status::out_of_range;
        case reason_code::packet_identifier_in_use:
        case reason_code::receive_maximum_exceeded:
            return castle::status::full;
        case reason_code::packet_identifier_not_found:
            return castle::status::not_found;
        case reason_code::server_unavailable:
        case reason_code::server_busy:
            return castle::status::time_out;
        default:
            return castle::status::unknown_error;
    }
}

} // namespace mqtt
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_MQTT_TYPES_HPP
