#include <gtest/gtest.h>
#include "castle_ext/protocols/mqtt/codec.hpp"

using namespace castle::protocols::mqtt;

TEST(MqttCodec, RemainingLengthBoundaryRoundTrips)
{
    const uint32_t values[] = {0U, 1U, 127U, 128U, 16383U, 16384U, 2097151U, 2097152U, maximum_remaining_length};
    for (uint32_t value : values)
    {
        uint8_t encoded[4U]{}; castle::size_type size = 0U;
        ASSERT_EQ(encode_remaining_length(value, encoded, size), castle::status::ok);
        uint32_t decoded = 0U; castle::size_type consumed = 0U;
        ASSERT_EQ(decode_remaining_length(encoded, size, decoded, consumed), castle::status::ok);
        EXPECT_EQ(decoded, value);
        EXPECT_EQ(consumed, size);
    }
    uint8_t encoded[4U]{}; castle::size_type size = 0U;
    EXPECT_EQ(encode_remaining_length(maximum_remaining_length + 1U, encoded, size), castle::status::out_of_range);
    EXPECT_EQ(encode_remaining_length(1U, nullptr, size), castle::status::invalid_argument);
}

TEST(MqttCodec, RejectsMalformedAndIncompleteLengths)
{
    uint32_t value = 99U; castle::size_type consumed = 99U;
    uint8_t empty[] = {};
    EXPECT_EQ(decode_remaining_length(empty, 0U, value, consumed), castle::status::empty);
    uint8_t continuation[] = {0x80U};
    EXPECT_EQ(decode_remaining_length(continuation, 1U, value, consumed), castle::status::empty);
    uint8_t malformed[] = {0x80U, 0x80U, 0x80U, 0x80U};
    EXPECT_EQ(decode_remaining_length(malformed, sizeof(malformed), value, consumed), castle::status::invalid_argument);
    EXPECT_EQ(decode_remaining_length(nullptr, 1U, value, consumed), castle::status::invalid_argument);
}

TEST(MqttCodec, FixedHeadersAndStreamInspection)
{
    EXPECT_TRUE(valid_fixed_header(packet_type::publish, 0x0DU));
    EXPECT_FALSE(valid_fixed_header(packet_type::publish, 0x06U));
    EXPECT_TRUE(valid_fixed_header(packet_type::pubrel, 0x02U));
    EXPECT_TRUE(valid_fixed_header(packet_type::pubrel, 0x0AU));
    EXPECT_FALSE(valid_fixed_header(packet_type::pubrel, 0U));
    EXPECT_FALSE(valid_fixed_header(packet_type::pingreq, 1U));

    uint8_t ping[] = {0xC0U, 0x00U}; packet_view packet{};
    EXPECT_EQ(inspect_packet(ping, 1U, 32U, packet), castle::status::empty);
    ASSERT_EQ(inspect_packet(ping, sizeof(ping), 32U, packet), castle::status::ok);
    EXPECT_EQ(packet.type, packet_type::pingreq);
    EXPECT_EQ(packet.packet_size, 2U);
    uint8_t malformed[] = {0xC1U, 0U};
    EXPECT_EQ(inspect_packet(malformed, sizeof(malformed), 32U, packet), castle::status::invalid_argument);
    uint8_t too_large[] = {0x30U, 0x7FU};
    EXPECT_EQ(inspect_packet(too_large, sizeof(too_large), 16U, packet), castle::status::out_of_range);
}

TEST(MqttCodec, PacketReaderWriterAndProperties)
{
    uint8_t bytes[64U]{};
    packet_writer writer(bytes, sizeof(bytes));
    EXPECT_EQ(writer.put_u8(7U), castle::status::ok);
    EXPECT_EQ(writer.put_u16(0x1234U), castle::status::ok);
    EXPECT_EQ(writer.put_utf8("ab"), castle::status::ok);
    packet_reader reader(bytes, writer.size());
    uint8_t one = 0U; uint16_t two = 0U; char text[8U]{}; castle::size_type text_size = 0U;
    EXPECT_EQ(reader.get_u8(one), castle::status::ok);
    EXPECT_EQ(reader.get_u16(two), castle::status::ok);
    EXPECT_EQ(reader.get_utf8(text, sizeof(text), text_size), castle::status::ok);
    EXPECT_EQ(one, 7U); EXPECT_EQ(two, 0x1234U); EXPECT_STREQ(text, "ab");
    EXPECT_EQ(reader.get_u8(one), castle::status::invalid_argument);

    property_list<3U, 16U, 8U> properties;
    ASSERT_EQ(properties.add_integer(property_id::session_expiry_interval, 60U), castle::status::ok);
    ASSERT_EQ(properties.add_string(property_id::reason_string, "ok"), castle::status::ok);
    ASSERT_EQ(properties.add_user_property("k", "v"), castle::status::ok);
    uint8_t encoded[64U]{}; castle::size_type used = 0U;
    ASSERT_EQ(encode_properties(properties, encoded, sizeof(encoded), used), castle::status::ok);
    packet_reader prop_reader(encoded, used);
    property_list<3U, 16U, 8U> parsed;
    EXPECT_EQ(decode_properties(prop_reader, parsed), castle::status::ok);
    EXPECT_EQ(parsed.count, 3U);
    EXPECT_EQ(parsed.find(property_id::session_expiry_interval)->integer, 60U);
    EXPECT_STREQ(parsed.entries[2U].user_value, "v");
}

TEST(MqttCodec, CapacityErrorsAndUnknownPropertyPolicy)
{
    uint8_t small[1U]{}; packet_writer writer(small, sizeof(small));
    EXPECT_EQ(writer.put_u16(1U), castle::status::full);
    uint8_t packet[2U]{}; castle::size_type written = 99U;
    EXPECT_EQ(encode_packet(packet_type::pingreq, 0U, nullptr, 0U, packet, 1U, written), castle::status::full);
    EXPECT_EQ(encode_packet(packet_type::pingreq, 1U, nullptr, 0U, packet, sizeof(packet), written), castle::status::invalid_argument);

    uint8_t unknown_property[] = {0x02U, 0x55U, 0x00U};
    packet_reader reader(unknown_property, sizeof(unknown_property));
    property_list<2U, 8U, 8U> props;
    EXPECT_EQ(decode_properties(reader, props, false), castle::status::invalid_argument);
    packet_reader ignore_reader(unknown_property, sizeof(unknown_property));
    EXPECT_EQ(decode_properties(ignore_reader, props, true), castle::status::ok);
}

TEST(MqttCodec, WriterRejectsInvalidStateAndCapacity)
{
    packet_writer invalid_output(nullptr, 4U);
    EXPECT_EQ(invalid_output.status(), castle::status::invalid_argument);
    EXPECT_EQ(invalid_output.put_u8(1U), castle::status::invalid_argument);
    EXPECT_EQ(packet_writer(nullptr, 0U).status(), castle::status::ok);

    uint8_t one[1U]{};
    packet_writer single(one, sizeof(one));
    EXPECT_EQ(single.put_u8(1U), castle::status::ok);
    EXPECT_EQ(single.put_u8(2U), castle::status::full);
    EXPECT_EQ(single.put_utf8("a"), castle::status::full);
    EXPECT_EQ(single.put_binary(one, 1U), castle::status::full);

    uint8_t three[3U]{};
    packet_writer narrow(three, sizeof(three));
    EXPECT_EQ(narrow.put_u32(1U), castle::status::full);

    uint8_t four[4U]{};
    packet_writer bytes_writer(four, sizeof(four));
    packet_writer fresh_bytes(four, sizeof(four));
    EXPECT_EQ(fresh_bytes.put_bytes(nullptr, 0U), castle::status::ok);
    EXPECT_EQ(bytes_writer.put_bytes(nullptr, 1U), castle::status::invalid_argument);
    uint8_t five[5U]{};
    EXPECT_EQ(bytes_writer.put_bytes(five, sizeof(five)), castle::status::full);
    packet_writer varint_writer(four, sizeof(four));
    EXPECT_EQ(varint_writer.put_varint(300U), castle::status::ok);
    EXPECT_EQ(varint_writer.size(), 2U);
    packet_writer overflow_varint(four, sizeof(four));
    EXPECT_EQ(overflow_varint.put_varint(maximum_remaining_length + 1U), castle::status::out_of_range);

    static const char huge_text[65536U]{};
    EXPECT_EQ(bytes_writer.put_utf8(castle::container::string_view(huge_text, sizeof(huge_text))), castle::status::out_of_range);
    static const uint8_t huge_bytes[65536U]{};
    EXPECT_EQ(bytes_writer.put_binary(huge_bytes, sizeof(huge_bytes)), castle::status::out_of_range);
}

TEST(MqttCodec, ReaderBoundsAndNullInputs)
{
    packet_reader null_input(nullptr, 1U);
    EXPECT_EQ(null_input.status(), castle::status::invalid_argument);
    packet_reader null_empty(nullptr, 0U);
    EXPECT_EQ(null_empty.status(), castle::status::ok);
    EXPECT_EQ(null_empty.current(), nullptr);

    uint8_t one_byte[1U]{0x00U};
    packet_reader short_reader(one_byte, 1U);
    uint16_t u16 = 0U; uint32_t u32 = 0U; uint8_t u8 = 0U;
    EXPECT_EQ(short_reader.get_u16(u16), castle::status::invalid_argument);
    uint8_t three[3U]{0x01U, 0x02U, 0x03U};
    packet_reader three_reader(three, sizeof(three));
    EXPECT_EQ(three_reader.get_u32(u32), castle::status::invalid_argument);
    packet_reader fresh_reader(three, sizeof(three));
    EXPECT_EQ(fresh_reader.get_u8(u8), castle::status::ok);
    EXPECT_EQ(u8, 0x01U);

    char text[4U]{};
    castle::size_type length = 0U;
    packet_reader length_reader(one_byte, 1U);
    EXPECT_EQ(length_reader.get_utf8(text, sizeof(text), length), castle::status::invalid_argument);
    uint8_t short_text[] = {0x00U, 0x03U, 'a'};
    packet_reader short_text_reader(short_text, sizeof(short_text));
    EXPECT_EQ(short_text_reader.get_utf8(text, sizeof(text), length), castle::status::invalid_argument);
    uint8_t long_text[] = {0x00U, 0x04U, 'a', 'b', 'c', 'd'};
    packet_reader long_text_reader(long_text, sizeof(long_text));
    EXPECT_EQ(long_text_reader.get_utf8(text, sizeof(text), length), castle::status::out_of_range);
    uint8_t text_ok[] = {0x00U, 0x01U, 'a'};
    packet_reader text_reader(text_ok, sizeof(text_ok));
    EXPECT_EQ(text_reader.get_utf8(text, sizeof(text), length), castle::status::ok);
    EXPECT_STREQ(text, "a");
    EXPECT_EQ(length, 1U);
    packet_reader no_output(text_ok, sizeof(text_ok));
    EXPECT_EQ(no_output.get_utf8(nullptr, sizeof(text), length), castle::status::invalid_argument);

    uint8_t bytes[8U]{};
    packet_reader binary_short(one_byte, 1U);
    EXPECT_EQ(binary_short.get_binary(bytes, sizeof(bytes), length), castle::status::invalid_argument);
    uint8_t big_binary[] = {0x00U, 0x05U, 1U, 2U, 3U, 4U, 5U};
    packet_reader big_binary_reader(big_binary, sizeof(big_binary));
    EXPECT_EQ(big_binary_reader.get_binary(bytes, 4U, length), castle::status::out_of_range);
    uint8_t truncated_binary[] = {0x00U, 0x03U, 1U};
    packet_reader truncated_binary_reader(truncated_binary, sizeof(truncated_binary));
    EXPECT_EQ(truncated_binary_reader.get_binary(bytes, sizeof(bytes), length), castle::status::invalid_argument);
    uint8_t binary_no_output[] = {0x00U, 0x01U, 9U};
    packet_reader binary_no_output_reader(binary_no_output, sizeof(binary_no_output));
    EXPECT_EQ(binary_no_output_reader.get_binary(nullptr, sizeof(bytes), length), castle::status::invalid_argument);
    uint8_t binary_ok[] = {0x00U, 0x02U, 7U, 8U};
    packet_reader binary_reader_ok(binary_ok, sizeof(binary_ok));
    EXPECT_EQ(binary_reader_ok.get_binary(bytes, sizeof(bytes), length), castle::status::ok);
    EXPECT_EQ(length, 2U);
    EXPECT_EQ(bytes[0U], 7U);
    uint8_t empty_binary[] = {0x00U, 0x00U};
    packet_reader empty_binary_reader(empty_binary, sizeof(empty_binary));
    EXPECT_EQ(empty_binary_reader.get_binary(nullptr, 0U, length), castle::status::ok);
    EXPECT_EQ(length, 0U);

    uint32_t varint = 0U;
    EXPECT_EQ(null_empty.get_varint(varint), castle::status::empty);
    uint8_t continuation[] = {0x80U};
    packet_reader continuation_reader(continuation, sizeof(continuation));
    EXPECT_EQ(continuation_reader.get_varint(varint), castle::status::empty);
    EXPECT_EQ(continuation_reader.skip(2U), castle::status::invalid_argument);
    EXPECT_EQ(continuation_reader.skip(1U), castle::status::invalid_argument);
}

TEST(MqttCodec, PropertyPredicatesClassifyEveryIdentifier)
{
    for (uint32_t raw = 0U; raw <= 0xFFU; ++raw)
    {
        auto id = static_cast<property_id>(raw);
        int classes = static_cast<int>(detail::property_is_byte(id)) + static_cast<int>(detail::property_is_u16(id)) +
                      static_cast<int>(detail::property_is_u32(id)) + static_cast<int>(detail::property_is_varint(id)) +
                      static_cast<int>(detail::property_is_binary(id)) + static_cast<int>(detail::property_is_string(id)) +
                      static_cast<int>(detail::property_is_pair(id));
        EXPECT_LE(classes, 1) << raw;
    }
    EXPECT_TRUE(detail::property_is_byte(property_id::shared_subscription_available));
    EXPECT_TRUE(detail::property_is_u16(property_id::server_keep_alive));
    EXPECT_TRUE(detail::property_is_u32(property_id::maximum_packet_size));
    EXPECT_TRUE(detail::property_is_varint(property_id::subscription_identifier));
    EXPECT_TRUE(detail::property_is_binary(property_id::authentication_data));
    EXPECT_TRUE(detail::property_is_string(property_id::server_reference));
    EXPECT_TRUE(detail::property_is_pair(property_id::user_property));
    EXPECT_FALSE(detail::property_is_string(static_cast<property_id>(0x7FU)));
}

TEST(MqttCodec, PropertyEncodingRoundTripsEveryValueClass)
{
    property_list<8U, 16U, 8U> properties;
    const uint8_t blob[] = {0xAAU, 0xBBU};
    ASSERT_EQ(properties.add_integer(property_id::payload_format_indicator, 1U), castle::status::ok);
    ASSERT_EQ(properties.add_integer(property_id::receive_maximum, 10U), castle::status::ok);
    ASSERT_EQ(properties.add_integer(property_id::session_expiry_interval, 60U), castle::status::ok);
    ASSERT_EQ(properties.add_integer(property_id::subscription_identifier, 5U), castle::status::ok);
    ASSERT_EQ(properties.add_binary(property_id::correlation_data, blob, sizeof(blob)), castle::status::ok);
    ASSERT_EQ(properties.add_string(property_id::content_type, "ct"), castle::status::ok);
    ASSERT_EQ(properties.add_user_property("k", "v"), castle::status::ok);

    uint8_t encoded[128U]{};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_properties(properties, encoded, sizeof(encoded), written), castle::status::ok);
    packet_reader reader(encoded, written);
    property_list<8U, 16U, 8U> decoded;
    ASSERT_EQ(decode_properties(reader, decoded), castle::status::ok);
    EXPECT_EQ(reader.remaining(), 0U);
    EXPECT_EQ(decoded.count, 7U);
    EXPECT_EQ(decoded.find(property_id::payload_format_indicator)->integer, 1U);
    EXPECT_EQ(decoded.find(property_id::receive_maximum)->integer, 10U);
    EXPECT_EQ(decoded.find(property_id::session_expiry_interval)->integer, 60U);
    EXPECT_EQ(decoded.find(property_id::subscription_identifier)->integer, 5U);
    EXPECT_EQ(decoded.find(property_id::correlation_data)->binary_size, 2U);
    EXPECT_EQ(decoded.find(property_id::correlation_data)->binary_value[1U], 0xBBU);
    EXPECT_STREQ(decoded.find(property_id::content_type)->string_value, "ct");
    EXPECT_STREQ(decoded.entries[6U].string_value, "k");
    EXPECT_STREQ(decoded.entries[6U].user_value, "v");
}

TEST(MqttCodec, PropertyEncodingRejectsInvalidInputsAndSmallTargets)
{
    property_list<2U, 16U, 8U> properties;
    uint8_t buffer[64U]{};
    castle::size_type written = 99U;
    EXPECT_EQ(encode_properties(properties, nullptr, sizeof(buffer), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);

    properties.count = 3U;
    EXPECT_EQ(encode_properties(properties, buffer, sizeof(buffer), written), castle::status::invalid_argument);
    properties.count = 0U;
    EXPECT_EQ(encode_properties(properties, buffer, 3U, written), castle::status::full);
    EXPECT_EQ(encode_properties(properties, buffer, 4U, written), castle::status::ok);
    EXPECT_EQ(written, 1U);

    ASSERT_EQ(properties.add_string(property_id::content_type, "ab"), castle::status::ok);
    written = 99U;
    EXPECT_EQ(encode_properties(properties, buffer, 4U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    properties.clear();
    properties.entries[0U].id = static_cast<property_id>(0x7FU);
    properties.count = 1U;
    EXPECT_EQ(encode_properties(properties, buffer, sizeof(buffer), written), castle::status::invalid_argument);
}

TEST(MqttCodec, DecodeRejectsMalformedPropertySections)
{
    property_list<4U, 8U, 8U> out;
    packet_reader empty_reader(nullptr, 0U);
    EXPECT_EQ(decode_properties(empty_reader, out), castle::status::empty);

    uint8_t overrun[] = {0x05U, 0x01U};
    packet_reader overrun_reader(overrun, sizeof(overrun));
    EXPECT_EQ(decode_properties(overrun_reader, out), castle::status::invalid_argument);

    uint8_t incomplete_id[] = {0x01U, 0x80U};
    packet_reader incomplete_reader(incomplete_id, sizeof(incomplete_id));
    EXPECT_EQ(decode_properties(incomplete_reader, out), castle::status::invalid_argument);
    packet_reader incomplete_ignored(incomplete_id, sizeof(incomplete_id));
    EXPECT_EQ(decode_properties(incomplete_ignored, out, true), castle::status::ok);

    uint8_t unknown[] = {0x01U, 0x7FU};
    packet_reader unknown_reader(unknown, sizeof(unknown));
    EXPECT_EQ(decode_properties(unknown_reader, out), castle::status::invalid_argument);
    packet_reader unknown_ignored(unknown, sizeof(unknown));
    EXPECT_EQ(decode_properties(unknown_ignored, out, true), castle::status::ok);

    const uint8_t id_values[] = {0x01U, 0x17U, 0x19U, 0x24U, 0x25U, 0x28U, 0x29U, 0x2AU};
    for (uint8_t id : id_values)
    {
        uint8_t accepted[] = {0x02U, id, 0x01U};
        packet_reader accepted_reader(accepted, sizeof(accepted));
        EXPECT_EQ(decode_properties(accepted_reader, out), castle::status::ok) << static_cast<int>(id);
        uint8_t rejected[] = {0x02U, id, 0x02U};
        packet_reader rejected_reader(rejected, sizeof(rejected));
        EXPECT_EQ(decode_properties(rejected_reader, out), castle::status::invalid_argument) << static_cast<int>(id);
    }
    uint8_t truncated_byte[] = {0x01U, 0x01U};
    packet_reader truncated_byte_reader(truncated_byte, sizeof(truncated_byte));
    EXPECT_EQ(decode_properties(truncated_byte_reader, out), castle::status::invalid_argument);

    uint8_t zero_receive[] = {0x03U, 0x21U, 0x00U, 0x00U};
    packet_reader zero_receive_reader(zero_receive, sizeof(zero_receive));
    EXPECT_EQ(decode_properties(zero_receive_reader, out), castle::status::invalid_argument);
    uint8_t zero_alias[] = {0x03U, 0x23U, 0x00U, 0x00U};
    packet_reader zero_alias_reader(zero_alias, sizeof(zero_alias));
    EXPECT_EQ(decode_properties(zero_alias_reader, out), castle::status::invalid_argument);
    uint8_t zero_keep_alive[] = {0x03U, 0x13U, 0x00U, 0x00U};
    packet_reader zero_keep_alive_reader(zero_keep_alive, sizeof(zero_keep_alive));
    EXPECT_EQ(decode_properties(zero_keep_alive_reader, out), castle::status::ok);
    uint8_t receive_ok[] = {0x03U, 0x21U, 0x00U, 0x05U};
    packet_reader receive_ok_reader(receive_ok, sizeof(receive_ok));
    EXPECT_EQ(decode_properties(receive_ok_reader, out), castle::status::ok);
    uint8_t truncated_u16[] = {0x01U, 0x21U};
    packet_reader truncated_u16_reader(truncated_u16, sizeof(truncated_u16));
    EXPECT_EQ(decode_properties(truncated_u16_reader, out), castle::status::invalid_argument);

    uint8_t zero_subscription[] = {0x02U, 0x0BU, 0x00U};
    packet_reader zero_subscription_reader(zero_subscription, sizeof(zero_subscription));
    EXPECT_EQ(decode_properties(zero_subscription_reader, out), castle::status::invalid_argument);
    uint8_t subscription[] = {0x02U, 0x0BU, 0x05U};
    packet_reader subscription_reader(subscription, sizeof(subscription));
    EXPECT_EQ(decode_properties(subscription_reader, out), castle::status::ok);
    uint8_t truncated_varint[] = {0x01U, 0x0BU};
    packet_reader truncated_varint_reader(truncated_varint, sizeof(truncated_varint));
    EXPECT_EQ(decode_properties(truncated_varint_reader, out), castle::status::empty);

    uint8_t correlation[] = {0x04U, 0x09U, 0x00U, 0x01U, 0xAAU};
    packet_reader correlation_reader(correlation, sizeof(correlation));
    EXPECT_EQ(decode_properties(correlation_reader, out), castle::status::ok);
    uint8_t binary_too_big[] = {0x0CU, 0x09U, 0x00U, 0x09U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    packet_reader binary_too_big_reader(binary_too_big, sizeof(binary_too_big));
    EXPECT_EQ(decode_properties(binary_too_big_reader, out), castle::status::out_of_range);
    uint8_t truncated_binary[] = {0x02U, 0x09U, 0x00U};
    packet_reader truncated_binary_reader(truncated_binary, sizeof(truncated_binary));
    EXPECT_EQ(decode_properties(truncated_binary_reader, out), castle::status::invalid_argument);

    property_list<4U, 4U, 8U> narrow_out;
    uint8_t text_ok[] = {0x05U, 0x03U, 0x00U, 0x02U, 'a', 'b'};
    packet_reader text_ok_reader(text_ok, sizeof(text_ok));
    EXPECT_EQ(decode_properties(text_ok_reader, narrow_out), castle::status::ok);
    uint8_t text_too_long[] = {0x08U, 0x03U, 0x00U, 0x05U, 'a', 'b', 'c', 'd', 'e'};
    packet_reader text_too_long_reader(text_too_long, sizeof(text_too_long));
    EXPECT_EQ(decode_properties(text_too_long_reader, narrow_out), castle::status::out_of_range);
    uint8_t truncated_text[] = {0x04U, 0x03U, 0x00U, 0x02U, 'a'};
    packet_reader truncated_text_reader(truncated_text, sizeof(truncated_text));
    EXPECT_EQ(decode_properties(truncated_text_reader, out), castle::status::invalid_argument);

    uint8_t user_ok[] = {0x07U, 0x26U, 0x00U, 0x01U, 'k', 0x00U, 0x01U, 'v'};
    packet_reader user_ok_reader(user_ok, sizeof(user_ok));
    EXPECT_EQ(decode_properties(user_ok_reader, out), castle::status::ok);
    uint8_t user_key_truncated[] = {0x03U, 0x26U, 0x00U, 0x05U};
    packet_reader user_key_reader(user_key_truncated, sizeof(user_key_truncated));
    EXPECT_EQ(decode_properties(user_key_reader, out), castle::status::invalid_argument);
    uint8_t user_value_truncated[] = {0x05U, 0x26U, 0x00U, 0x01U, 'k', 0x00U};
    packet_reader user_value_reader(user_value_truncated, sizeof(user_value_truncated));
    EXPECT_EQ(decode_properties(user_value_reader, out), castle::status::invalid_argument);

    property_list<1U, 8U, 8U> one_slot;
    uint8_t two_users[] = {0x0EU, 0x26U, 0x00U, 0x01U, 'a', 0x00U, 0x01U, 'b',
                           0x26U, 0x00U, 0x01U, 'a', 0x00U, 0x01U, 'b'};
    packet_reader two_users_reader(two_users, sizeof(two_users));
    EXPECT_EQ(decode_properties(two_users_reader, one_slot), castle::status::full);
}

TEST(MqttCodec, EncodePacketValidatesArgumentsAndCapacity)
{
    uint8_t out[300U]{};
    castle::size_type written = 99U;
    uint8_t body[] = {1U, 2U, 3U};
    EXPECT_EQ(encode_packet(packet_type::publish, 0U, body, sizeof(body), nullptr, sizeof(out), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_packet(packet_type::publish, 0U, nullptr, sizeof(body), out, sizeof(out), written), castle::status::invalid_argument);
    uint8_t dummy = 0U;
    EXPECT_EQ(encode_packet(packet_type::publish, 0U, &dummy, maximum_remaining_length + 1U, out, sizeof(out), written), castle::status::out_of_range);
    EXPECT_EQ(encode_packet(packet_type::publish, 0U, body, sizeof(body), out, 1U, written), castle::status::full);
    EXPECT_EQ(encode_packet(packet_type::publish, 0U, body, sizeof(body), out, 3U, written), castle::status::full);
    EXPECT_EQ(encode_packet(packet_type::publish, 0U, body, sizeof(body), out, sizeof(out), written), castle::status::ok);
    EXPECT_EQ(written, 5U);
    EXPECT_EQ(out[0U], 0x30U);
    EXPECT_EQ(out[1U], 3U);
    EXPECT_EQ(out[4U], 3U);

    static uint8_t large[200U]{};
    ASSERT_EQ(encode_packet(packet_type::publish, 0U, large, sizeof(large), out, sizeof(out), written), castle::status::ok);
    EXPECT_EQ(written, 203U);
    EXPECT_EQ(out[1U], 0xC8U);
    EXPECT_EQ(out[2U], 0x01U);
}

TEST(MqttCodec, InspectPacketReportsStreamAndHeaderErrors)
{
    packet_view packet{};
    uint8_t ping[] = {0xC0U, 0x00U};
    EXPECT_EQ(inspect_packet(nullptr, 2U, 32U, packet), castle::status::invalid_argument);
    EXPECT_EQ(inspect_packet(ping, sizeof(ping), 1U, packet), castle::status::invalid_argument);

    uint8_t reserved[] = {0x00U, 0x00U};
    EXPECT_EQ(inspect_packet(reserved, sizeof(reserved), 32U, packet), castle::status::invalid_argument);
    uint8_t connect_bad_flags[] = {0x11U, 0x00U};
    EXPECT_EQ(inspect_packet(connect_bad_flags, sizeof(connect_bad_flags), 32U, packet), castle::status::invalid_argument);

    uint8_t truncated_length[] = {0x10U, 0x80U};
    EXPECT_EQ(inspect_packet(truncated_length, sizeof(truncated_length), 32U, packet), castle::status::empty);
    uint8_t bad_length[] = {0x10U, 0x80U, 0x80U, 0x80U, 0x80U};
    EXPECT_EQ(inspect_packet(bad_length, sizeof(bad_length), 32U, packet), castle::status::invalid_argument);

    uint8_t header_over[] = {0x30U, 0x04U, 0x00U, 0x00U, 0x00U, 0x00U};
    EXPECT_EQ(inspect_packet(header_over, sizeof(header_over), 4U, packet), castle::status::out_of_range);
    uint8_t oversize[] = {0x30U, 0x05U};
    EXPECT_EQ(inspect_packet(oversize, sizeof(oversize), 4U, packet), castle::status::out_of_range);
    uint8_t partial_body[] = {0x30U, 0x05U, 'a'};
    EXPECT_EQ(inspect_packet(partial_body, sizeof(partial_body), 32U, packet), castle::status::empty);

    EXPECT_TRUE(valid_fixed_header(packet_type::subscribe, 0x02U));
    EXPECT_FALSE(valid_fixed_header(packet_type::subscribe, 0U));
    EXPECT_TRUE(valid_fixed_header(packet_type::unsubscribe, 0x0AU));
    EXPECT_FALSE(valid_fixed_header(packet_type::unsubscribe, 0U));
    EXPECT_TRUE(valid_fixed_header(packet_type::publish, 0x00U));
    EXPECT_FALSE(valid_fixed_header(packet_type::publish, 0x08U));
}

TEST(MqttCodec, RemainingLengthRejectsOverlongEncodings)
{
    uint32_t value = 0U; castle::size_type consumed = 0U;
    uint8_t overlong_two_bytes[] = {0x80U, 0x00U};
    EXPECT_EQ(decode_remaining_length(overlong_two_bytes, sizeof(overlong_two_bytes), value, consumed), castle::status::invalid_argument);
    uint8_t canonical_two_bytes[] = {0x80U, 0x01U};
    EXPECT_EQ(decode_remaining_length(canonical_two_bytes, sizeof(canonical_two_bytes), value, consumed), castle::status::ok);
    EXPECT_EQ(value, 128U);
}
