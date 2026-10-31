#include <gtest/gtest.h>
#include "castle_ext/protocols/coap/codec.hpp"

using namespace castle::protocols::coap;

TEST(CoapCodec, RoundTripsHeaderTokenOptionsAndPayload)
{
    message<8U, 32U, 64U> request;
    request.type = message_type::confirmable;
    request.code_value = static_cast<uint8_t>(code::post);
    request.message_id = 0xBEEF;
    const uint8_t token[] = {1U, 2U, 3U, 4U};
    ASSERT_EQ(request.set_token(token, sizeof(token)), castle::status::ok);
    ASSERT_EQ(append_uri_path(request, "sensors/temperature"), castle::status::ok);
    ASSERT_EQ(request.append_uint_option(static_cast<uint16_t>(option_number::content_format), 50U), castle::status::ok);
    const uint8_t payload[] = {0U, 0xFFU, 0x7FU};
    ASSERT_EQ(request.set_payload(payload, sizeof(payload)), castle::status::ok);
    uint8_t wire[128U]{}; castle::size_type used = 0U;
    ASSERT_EQ(encode_message(request, wire, sizeof(wire), used), castle::status::ok);
    EXPECT_EQ((wire[0U] >> 6U) & 3U, 1U);
    EXPECT_EQ(castle::read_be16(wire + 2U), 0xBEEFU);
    message<8U, 32U, 64U> decoded;
    ASSERT_EQ(decode_message(wire, used, decoded), castle::status::ok);
    EXPECT_EQ(decoded.type, message_type::confirmable);
    EXPECT_EQ(decoded.code_value, static_cast<uint8_t>(code::post));
    EXPECT_EQ(decoded.message_id, request.message_id);
    EXPECT_EQ(decoded.token_length, 4U);
    EXPECT_EQ(decoded.payload_size, 3U);
    EXPECT_EQ(decoded.payload[1U], 0xFFU);
    EXPECT_TRUE(uri_path_matches(decoded, "sensors/temperature"));
    uint32_t format = 0U;
    const auto* option = decoded.find_option(static_cast<uint16_t>(option_number::content_format));
    ASSERT_NE(option, nullptr);
    EXPECT_EQ(decode_uint_option(option->value, option->length, format), castle::status::ok);
    EXPECT_EQ(format, 50U);
}

TEST(CoapCodec, HandlesExtendedDeltaAndLengthFields)
{
    message<4U, 300U, 8U> value;
    value.code_value = static_cast<uint8_t>(code::get);
    const uint8_t bytes[270U]{};
    ASSERT_EQ(value.append_option(300U, bytes, sizeof(bytes)), castle::status::ok);
    uint8_t wire[400U]{}; castle::size_type size = 0U;
    ASSERT_EQ(encode_message(value, wire, sizeof(wire), size), castle::status::ok);
    message<4U, 300U, 8U> parsed;
    ASSERT_EQ(decode_message(wire, size, parsed), castle::status::ok);
    EXPECT_EQ(parsed.option_count, 0U); // option 300 is elective and deliberately ignored by the decoder
}

TEST(CoapCodec, SkipsElectiveAndRejectsCriticalUnknownOptions)
{
    // CON GET, MID=1, unknown elective option number 100 with empty value.
    const uint8_t elective[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xD0U, 0x57U};
    message<> parsed; decode_error error = decode_error::none;
    EXPECT_EQ(decode_message(elective, sizeof(elective), parsed, &error), castle::status::ok);
    EXPECT_EQ(parsed.option_count, 0U);

    // CON GET, MID=1, unknown critical option number 99 with empty value.
    const uint8_t critical[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xD0U, 0x56U};
    EXPECT_EQ(decode_message(critical, sizeof(critical), parsed, &error), castle::status::not_configured);
    EXPECT_EQ(error, decode_error::unknown_critical_option);
}

TEST(CoapCodec, RejectsMalformedHeadersOptionsAndPayloadMarker)
{
    message<> parsed; decode_error error = decode_error::none;
    const uint8_t short_header[] = {0x40U, 0x01U, 0x00U};
    EXPECT_EQ(decode_message(short_header, sizeof(short_header), parsed, &error), castle::status::invalid_argument);
    const uint8_t wrong_version[] = {0x00U, 0x01U, 0x00U, 0x01U};
    EXPECT_EQ(decode_message(wrong_version, sizeof(wrong_version), parsed, &error), castle::status::invalid_argument);
    const uint8_t long_token[] = {0x49U, 0x01U, 0x00U, 0x01U, 1U,2U,3U,4U,5U,6U,7U,8U,9U};
    EXPECT_EQ(decode_message(long_token, sizeof(long_token), parsed, &error), castle::status::invalid_argument);
    const uint8_t reserved_nibble[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xF0U};
    EXPECT_EQ(decode_message(reserved_nibble, sizeof(reserved_nibble), parsed, &error), castle::status::invalid_argument);
    const uint8_t empty_payload[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xFFU};
    EXPECT_EQ(decode_message(empty_payload, sizeof(empty_payload), parsed, &error), castle::status::invalid_argument);
    EXPECT_EQ(error, decode_error::malformed_option);
}

TEST(CoapCodec, CapacityAndEmptyMessageRules)
{
    message<1U, 2U, 1U> value;
    value.code_value = static_cast<uint8_t>(code::content);
    const uint8_t too_long[] = {1U, 2U};
    EXPECT_EQ(value.set_payload(too_long, sizeof(too_long)), castle::status::full);
    value.code_value = 0U;
    value.token_length = 1U;
    uint8_t wire[16U]{}; castle::size_type size = 0U;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), size), castle::status::invalid_argument);
    value.clear();
    value.type = message_type::reset;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), size), castle::status::ok);
    EXPECT_EQ(size, 4U);
    value.type = message_type::confirmable;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), size), castle::status::invalid_argument);
    value.type = message_type::acknowledgement;
    value.code_value = static_cast<uint8_t>(code::get);
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), size), castle::status::invalid_argument);
    message<> parsed; decode_error error = decode_error::none;
    const uint8_t empty_confirmable[] = {0x40U, 0x00U, 0x00U, 0x01U};
    EXPECT_EQ(decode_message(empty_confirmable, sizeof(empty_confirmable), parsed, &error), castle::status::invalid_argument);
    const uint8_t reset_with_code[] = {0x70U, 0x45U, 0x00U, 0x01U};
    EXPECT_EQ(decode_message(reset_with_code, sizeof(reset_with_code), parsed, &error), castle::status::invalid_argument);
    uint32_t integer = 99U;
    EXPECT_EQ(decode_uint_option(nullptr, 0U, integer), castle::status::ok);
    EXPECT_EQ(integer, 0U);
}
