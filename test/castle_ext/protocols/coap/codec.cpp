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

TEST(CoapCodec, DetailHelpersReportCapacityAndInvalidNibbles)
{
    uint8_t buffer[4]{};
    castle::size_type used = 2U;
    EXPECT_EQ(detail::write_byte(buffer, 2U, used, 0xAAU), castle::status::full);
    EXPECT_EQ(used, 2U);
    used = 0U;
    EXPECT_EQ(detail::write_byte(buffer, 2U, used, 0xAAU), castle::status::ok);
    EXPECT_EQ(used, 1U);

    used = 2U;
    EXPECT_EQ(detail::write_extended(buffer, 3U, used, 14U, 300U), castle::status::full);
    used = 1U;
    EXPECT_EQ(detail::write_extended(buffer, 4U, used, 14U, 300U), castle::status::ok);
    EXPECT_EQ(used, 3U);
    EXPECT_EQ(detail::write_extended(buffer, 4U, used, 15U, 0U), castle::status::invalid_argument);

    const uint8_t one_byte[] = {0x07U};
    uint32_t value = 0U;
    used = 0U;
    EXPECT_EQ(detail::decode_nibble(13U, one_byte, sizeof(one_byte), used, value), castle::status::ok);
    EXPECT_EQ(value, 20U);
    used = 1U;
    EXPECT_EQ(detail::decode_nibble(13U, one_byte, sizeof(one_byte), used, value), castle::status::invalid_argument);
    used = 0U;
    EXPECT_EQ(detail::decode_nibble(14U, one_byte, sizeof(one_byte), used, value), castle::status::invalid_argument);
    EXPECT_EQ(detail::decode_nibble(15U, one_byte, sizeof(one_byte), used, value), castle::status::invalid_argument);
}

TEST(CoapCodec, OptionLengthAndRepeatRules)
{
    EXPECT_FALSE(valid_option_length(2U, 0U));
    EXPECT_TRUE(valid_option_length(1U, 8U));
    EXPECT_FALSE(valid_option_length(1U, 9U));
    EXPECT_FALSE(valid_option_length(3U, 0U));
    EXPECT_TRUE(valid_option_length(3U, 255U));
    EXPECT_FALSE(valid_option_length(3U, 256U));
    EXPECT_FALSE(valid_option_length(4U, 0U));
    EXPECT_TRUE(valid_option_length(4U, 8U));
    EXPECT_FALSE(valid_option_length(4U, 9U));
    EXPECT_TRUE(valid_option_length(5U, 0U));
    EXPECT_FALSE(valid_option_length(5U, 1U));
    EXPECT_TRUE(valid_option_length(6U, 3U));
    EXPECT_FALSE(valid_option_length(6U, 4U));
    EXPECT_TRUE(valid_option_length(7U, 2U));
    EXPECT_FALSE(valid_option_length(7U, 3U));
    EXPECT_TRUE(valid_option_length(8U, 255U));
    EXPECT_TRUE(valid_option_length(11U, 255U));
    EXPECT_TRUE(valid_option_length(15U, 255U));
    EXPECT_TRUE(valid_option_length(20U, 255U));
    EXPECT_FALSE(valid_option_length(20U, 256U));
    EXPECT_TRUE(valid_option_length(12U, 2U));
    EXPECT_TRUE(valid_option_length(17U, 0U));
    EXPECT_FALSE(valid_option_length(17U, 3U));
    EXPECT_TRUE(valid_option_length(14U, 4U));
    EXPECT_FALSE(valid_option_length(14U, 5U));
    EXPECT_TRUE(valid_option_length(23U, 3U));
    EXPECT_FALSE(valid_option_length(27U, 4U));
    EXPECT_TRUE(valid_option_length(28U, 4U));
    EXPECT_FALSE(valid_option_length(60U, 5U));
    EXPECT_FALSE(valid_option_length(35U, 0U));
    EXPECT_TRUE(valid_option_length(35U, 1034U));
    EXPECT_FALSE(valid_option_length(35U, 1035U));
    EXPECT_FALSE(valid_option_length(39U, 0U));
    EXPECT_TRUE(valid_option_length(39U, 255U));
    EXPECT_FALSE(valid_option_length(39U, 256U));

    EXPECT_TRUE(is_repeatable_option(1U));
    EXPECT_TRUE(is_repeatable_option(8U));
    EXPECT_TRUE(is_repeatable_option(11U));
    EXPECT_TRUE(is_repeatable_option(15U));
    EXPECT_TRUE(is_repeatable_option(20U));
    EXPECT_FALSE(is_repeatable_option(12U));
    EXPECT_FALSE(is_repeatable_option(3U));
}

TEST(CoapCodec, EncodeRejectsInconsistentMessages)
{
    using value_type = message<4U, 16U, 32U>;
    uint8_t wire[64U]{};
    castle::size_type written = 0U;
    value_type base;
    base.code_value = static_cast<uint8_t>(code::get);
    EXPECT_EQ(encode_message(base, nullptr, sizeof(wire), written), castle::status::invalid_argument);

    value_type value = base;
    value.version = 2U;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);
    value = base;
    value.token_length = 9U;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);
    value = base;
    value.option_count = 5U;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);
    value = base;
    value.payload_size = 33U;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);
    value = base;
    value.type = static_cast<message_type>(4U);
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);

    value = base;
    value.code_value = 0U;
    value.type = message_type::acknowledgement;
    value.token_length = 1U;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);
    value = base;
    value.code_value = 0U;
    value.type = message_type::acknowledgement;
    value.option_count = 1U;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);
    value = base;
    value.code_value = 0U;
    value.type = message_type::reset;
    value.payload_size = 1U;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);
    value = base;
    value.code_value = 0U;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);
    value = base;
    value.type = message_type::reset;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);
    value = base;
    value.type = message_type::acknowledgement;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);

    value = base;
    value.code_value = 0U;
    value.type = message_type::acknowledgement;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::ok);
    EXPECT_EQ(written, 4U);
    value.type = message_type::reset;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::ok);
    EXPECT_EQ(written, 4U);
}

TEST(CoapCodec, EncodeReportsCapacityAndOptionFailures)
{
    using value_type = message<4U, 16U, 32U>;
    uint8_t wire[64U]{};
    castle::size_type written = 0U;

    value_type token_value;
    token_value.code_value = static_cast<uint8_t>(code::get);
    token_value.token_length = 2U;
    EXPECT_EQ(encode_message(token_value, wire, 5U, written), castle::status::full);

    value_type zero_number;
    zero_number.code_value = static_cast<uint8_t>(code::get);
    zero_number.options[0].number = 0U;
    zero_number.option_count = 1U;
    EXPECT_EQ(encode_message(zero_number, wire, sizeof(wire), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);

    value_type unsorted;
    unsorted.code_value = static_cast<uint8_t>(code::get);
    unsorted.options[0].number = 11U;
    unsorted.options[0].length = 1U;
    unsorted.options[1].number = 5U;
    unsorted.options[1].length = 0U;
    unsorted.option_count = 2U;
    EXPECT_EQ(encode_message(unsorted, wire, sizeof(wire), written), castle::status::invalid_argument);

    value_type long_value;
    long_value.code_value = static_cast<uint8_t>(code::get);
    long_value.options[0].number = 100U;
    long_value.options[0].length = 17U;
    long_value.option_count = 1U;
    EXPECT_EQ(encode_message(long_value, wire, sizeof(wire), written), castle::status::invalid_argument);

    value_type unsupported_length;
    unsupported_length.code_value = static_cast<uint8_t>(code::get);
    unsupported_length.options[0].number = 12U;
    unsupported_length.options[0].length = 3U;
    unsupported_length.option_count = 1U;
    EXPECT_EQ(encode_message(unsupported_length, wire, sizeof(wire), written), castle::status::invalid_argument);

    value_type header_full;
    header_full.code_value = static_cast<uint8_t>(code::get);
    header_full.options[0].number = 11U;
    header_full.options[0].length = 1U;
    header_full.options[0].value[0U] = 'a';
    header_full.option_count = 1U;
    EXPECT_EQ(encode_message(header_full, wire, 4U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    value_type delta_extension_full;
    delta_extension_full.code_value = static_cast<uint8_t>(code::get);
    delta_extension_full.options[0].number = 13U;
    delta_extension_full.option_count = 1U;
    EXPECT_EQ(encode_message(delta_extension_full, wire, 5U, written), castle::status::full);

    value_type wide_delta_full;
    wide_delta_full.code_value = static_cast<uint8_t>(code::get);
    wide_delta_full.options[0].number = 300U;
    wide_delta_full.option_count = 1U;
    EXPECT_EQ(encode_message(wide_delta_full, wire, 5U, written), castle::status::full);

    value_type length_extension_full;
    length_extension_full.code_value = static_cast<uint8_t>(code::get);
    length_extension_full.options[0].number = 11U;
    length_extension_full.options[0].length = 13U;
    length_extension_full.option_count = 1U;
    EXPECT_EQ(encode_message(length_extension_full, wire, 5U, written), castle::status::full);

    value_type value_no_fit;
    value_no_fit.code_value = static_cast<uint8_t>(code::get);
    value_no_fit.options[0].number = 11U;
    value_no_fit.options[0].length = 3U;
    value_no_fit.option_count = 1U;
    EXPECT_EQ(encode_message(value_no_fit, wire, 7U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    value_type payload_no_fit;
    payload_no_fit.code_value = static_cast<uint8_t>(code::get);
    payload_no_fit.payload_size = 2U;
    EXPECT_EQ(encode_message(payload_no_fit, wire, 5U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    value_type fits;
    fits.code_value = static_cast<uint8_t>(code::get);
    fits.options[0].number = 11U;
    fits.options[0].length = 1U;
    fits.option_count = 1U;
    EXPECT_EQ(encode_message(fits, wire, 6U, written), castle::status::ok);
    EXPECT_EQ(written, 6U);
}

TEST(CoapCodec, EncodeRejectsOptionLongerThanWireLimit)
{
    static message<1U, 66000U, 1U> value;
    value.code_value = static_cast<uint8_t>(code::get);
    value.options[0].number = 100U;
    value.options[0].length = 65805U;
    value.option_count = 1U;
    uint8_t wire[64U]{};
    castle::size_type written = 0U;
    EXPECT_EQ(encode_message(value, wire, sizeof(wire), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(CoapCodec, DecodeRejectsMalformedDatagrams)
{
    message<> parsed;
    decode_error reason = decode_error::none;

    EXPECT_EQ(decode_message(nullptr, 4U, parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(nullptr, 4U, parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_header);

    const uint8_t short_header[] = {0x40U, 0x01U, 0x00U};
    EXPECT_EQ(decode_message(short_header, sizeof(short_header), parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(short_header, sizeof(short_header), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_header);

    const uint8_t wrong_version[] = {0x00U, 0x01U, 0x00U, 0x01U};
    EXPECT_EQ(decode_message(wrong_version, sizeof(wrong_version), parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(wrong_version, sizeof(wrong_version), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::unsupported_version);

    const uint8_t long_token[] = {0x49U, 0x01U, 0x00U, 0x01U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    EXPECT_EQ(decode_message(long_token, sizeof(long_token), parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(long_token, sizeof(long_token), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::invalid_token_length);

    const uint8_t truncated_token[] = {0x42U, 0x01U, 0x00U, 0x01U, 0xAAU};
    EXPECT_EQ(decode_message(truncated_token, sizeof(truncated_token), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::invalid_token_length);

    const uint8_t empty_with_token[] = {0x41U, 0x00U, 0x00U, 0x01U, 0x05U};
    EXPECT_EQ(decode_message(empty_with_token, sizeof(empty_with_token), parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(empty_with_token, sizeof(empty_with_token), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_header);

    const uint8_t empty_ack_with_option[] = {0x60U, 0x00U, 0x00U, 0x01U, 0x00U};
    EXPECT_EQ(decode_message(empty_ack_with_option, sizeof(empty_ack_with_option), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_header);

    const uint8_t ack_with_request_code[] = {0x60U, 0x01U, 0x00U, 0x01U};
    EXPECT_EQ(decode_message(ack_with_request_code, sizeof(ack_with_request_code), parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(ack_with_request_code, sizeof(ack_with_request_code), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_header);

    const uint8_t truncated_delta[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xD0U};
    EXPECT_EQ(decode_message(truncated_delta, sizeof(truncated_delta), parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(truncated_delta, sizeof(truncated_delta), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_option);

    const uint8_t truncated_wide_delta[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xE0U, 0x01U};
    EXPECT_EQ(decode_message(truncated_wide_delta, sizeof(truncated_wide_delta), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_option);

    const uint8_t delta_overflow[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xE0U, 0xFFU, 0xFFU};
    EXPECT_EQ(decode_message(delta_overflow, sizeof(delta_overflow), parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(delta_overflow, sizeof(delta_overflow), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_option);

    const uint8_t length_overflow[] = {0x40U, 0x01U, 0x00U, 0x01U, 0x03U, 0xAAU};
    EXPECT_EQ(decode_message(length_overflow, sizeof(length_overflow), parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(length_overflow, sizeof(length_overflow), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_option);

    const uint8_t zero_number[] = {0x40U, 0x01U, 0x00U, 0x01U, 0x00U};
    EXPECT_EQ(decode_message(zero_number, sizeof(zero_number), parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(zero_number, sizeof(zero_number), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_option);

    const uint8_t reserved_length[] = {0x40U, 0x01U, 0x00U, 0x01U, 0x0FU};
    EXPECT_EQ(decode_message(reserved_length, sizeof(reserved_length), parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(reserved_length, sizeof(reserved_length), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_option);

    const uint8_t payload_marker_only[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xFFU};
    EXPECT_EQ(decode_message(payload_marker_only, sizeof(payload_marker_only), parsed), castle::status::invalid_argument);
    EXPECT_EQ(decode_message(payload_marker_only, sizeof(payload_marker_only), parsed, &reason), castle::status::invalid_argument);
    EXPECT_EQ(reason, decode_error::malformed_option);

    const uint8_t critical_unknown[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xD0U, 0x56U};
    EXPECT_EQ(decode_message(critical_unknown, sizeof(critical_unknown), parsed), castle::status::not_configured);
    EXPECT_EQ(decode_message(critical_unknown, sizeof(critical_unknown), parsed, &reason), castle::status::not_configured);
    EXPECT_EQ(reason, decode_error::unknown_critical_option);
}

TEST(CoapCodec, DecodeSkipsIgnorableAndKeepsRepeatableOptions)
{
    message<> parsed;

    const uint8_t empty_ack[] = {0x60U, 0x00U, 0x00U, 0x01U};
    EXPECT_EQ(decode_message(empty_ack, sizeof(empty_ack), parsed), castle::status::ok);
    EXPECT_EQ(parsed.type, message_type::acknowledgement);

    const uint8_t empty_reset[] = {0x70U, 0x00U, 0x00U, 0x02U};
    EXPECT_EQ(decode_message(empty_reset, sizeof(empty_reset), parsed), castle::status::ok);
    EXPECT_EQ(parsed.type, message_type::reset);
    EXPECT_EQ(parsed.message_id, 2U);

    const uint8_t piggyback[] = {0x60U, 0x45U, 0x00U, 0x03U};
    EXPECT_EQ(decode_message(piggyback, sizeof(piggyback), parsed), castle::status::ok);
    EXPECT_EQ(parsed.code_value, static_cast<uint8_t>(code::content));

    const uint8_t bad_etag[] = {0x40U, 0x01U, 0x00U, 0x01U, 0x49U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    EXPECT_EQ(decode_message(bad_etag, sizeof(bad_etag), parsed), castle::status::ok);
    EXPECT_EQ(parsed.option_count, 0U);

    const uint8_t repeated_format[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xC1U, 0x00U, 0x01U, 0x00U};
    EXPECT_EQ(decode_message(repeated_format, sizeof(repeated_format), parsed), castle::status::ok);
    EXPECT_EQ(parsed.option_count, 1U);

    const uint8_t empty_max_age[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xD0U, 0x01U};
    EXPECT_EQ(decode_message(empty_max_age, sizeof(empty_max_age), parsed), castle::status::ok);
    ASSERT_EQ(parsed.option_count, 1U);
    EXPECT_EQ(parsed.options[0].number, 14U);
    EXPECT_EQ(parsed.options[0].length, 0U);

    const uint8_t two_paths[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xB1U, 'a', 0x01U, 'b'};
    EXPECT_EQ(decode_message(two_paths, sizeof(two_paths), parsed), castle::status::ok);
    EXPECT_EQ(parsed.option_count, 2U);
}

TEST(CoapCodec, DecodeReportsCapacityFailures)
{
    decode_error reason = decode_error::none;
    message<1U, 16U, 32U> one_option;
    const uint8_t two_paths[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xB1U, 'a', 0x01U, 'b'};
    EXPECT_EQ(decode_message(two_paths, sizeof(two_paths), one_option), castle::status::full);
    EXPECT_EQ(decode_message(two_paths, sizeof(two_paths), one_option, &reason), castle::status::full);
    EXPECT_EQ(reason, decode_error::capacity_exceeded);

    message<4U, 16U, 2U> small_payload;
    const uint8_t payload[] = {0x40U, 0x01U, 0x00U, 0x01U, 0xFFU, 1U, 2U, 3U};
    EXPECT_EQ(decode_message(payload, sizeof(payload), small_payload, &reason), castle::status::full);
    EXPECT_EQ(reason, decode_error::capacity_exceeded);
}

TEST(CoapCodec, UintOptionRejectsOversizedAndNullValues)
{
    uint32_t value = 0U;
    const uint8_t five[] = {1U, 2U, 3U, 4U, 5U};
    EXPECT_EQ(decode_uint_option(five, sizeof(five), value), castle::status::invalid_argument);
    EXPECT_EQ(decode_uint_option(nullptr, 1U, value), castle::status::invalid_argument);
    const uint8_t word[] = {0x01U, 0x02U};
    EXPECT_EQ(decode_uint_option(word, sizeof(word), value), castle::status::ok);
    EXPECT_EQ(value, 0x0102U);
}
