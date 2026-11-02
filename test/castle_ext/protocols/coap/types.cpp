#include <gtest/gtest.h>
#include "castle_ext/protocols/coap/types.hpp"

using namespace castle::protocols::coap;

TEST(CoapTypes, MessageStoresTokenPayloadAndSortedOptions)
{
    message<4U, 8U, 16U> value;
    const uint8_t token[] = {0xAAU, 0x55U};
    EXPECT_EQ(value.set_token(token, sizeof(token)), castle::status::ok);
    EXPECT_EQ(value.set_token(token, 9U), castle::status::invalid_argument);
    EXPECT_EQ(value.set_token(nullptr, 1U), castle::status::invalid_argument);
    const uint8_t payload[] = {1U, 2U, 3U};
    EXPECT_EQ(value.set_payload(payload, sizeof(payload)), castle::status::ok);
    EXPECT_EQ(value.set_payload(payload, 17U), castle::status::full);
    EXPECT_EQ(value.append_string_option(15U, "q=1"), castle::status::ok);
    EXPECT_EQ(value.append_string_option(11U, "path"), castle::status::ok);
    EXPECT_EQ(value.append_uint_option(12U, 50U), castle::status::ok);
    ASSERT_EQ(value.option_count, 3U);
    EXPECT_EQ(value.options[0U].number, 11U);
    EXPECT_EQ(value.options[1U].number, 12U);
    EXPECT_EQ(value.options[2U].number, 15U);
    EXPECT_NE(value.find_option(12U), nullptr);
    EXPECT_EQ(value.find_option(13U), nullptr);
}

TEST(CoapTypes, URIPathAndQueryHelpers)
{
    message<8U, 16U, 8U> request;
    EXPECT_EQ(append_uri_path(request, "/devices/led/state"), castle::status::ok);
    EXPECT_EQ(append_uri_query(request, "mode=slow&unit=c"), castle::status::ok);
    EXPECT_TRUE(uri_path_matches(request, "devices/led/state"));
    EXPECT_TRUE(uri_path_matches(request, "/devices/led/state/"));
    EXPECT_FALSE(uri_path_matches(request, "devices/led"));
    EXPECT_FALSE(uri_path_matches(request, "devices/led/state/extra"));
    EXPECT_EQ(request.find_option(static_cast<uint16_t>(option_number::uri_query))->length, 9U);
}

TEST(CoapTypes, OptionNumberSemanticsAndCodes)
{
    EXPECT_TRUE(is_critical_option(11U));
    EXPECT_FALSE(is_critical_option(12U));
    EXPECT_TRUE(is_unsafe_option(11U));
    EXPECT_FALSE(is_unsafe_option(12U));
    EXPECT_TRUE(is_no_cache_key_option(28U));
    EXPECT_FALSE(is_no_cache_key_option(14U));
    EXPECT_TRUE(is_request_code(code::delete_));
    EXPECT_FALSE(is_request_code(code::content));
    EXPECT_TRUE(is_response_code(static_cast<uint8_t>(code::created)));
    EXPECT_FALSE(is_response_code(0xC0U));
}

TEST(CoapTypes, CapacityErrorsAndClear)
{
    message<1U, 2U, 2U> value;
    EXPECT_EQ(value.append_string_option(11U, "abc"), castle::status::out_of_range);
    EXPECT_EQ(value.append_string_option(11U, "a"), castle::status::ok);
    EXPECT_EQ(value.append_string_option(12U, "b"), castle::status::full);
    value.clear();
    EXPECT_EQ(value.option_count, 0U);
    EXPECT_EQ(value.payload_size, 0U);
    EXPECT_EQ(value.code_value, 0U);
}

TEST(CoapTypes, NullArgumentsAndOptionBoundaries)
{
    message<4U, 8U, 16U> value;
    EXPECT_EQ(value.set_token(nullptr, 0U), castle::status::ok);
    EXPECT_EQ(value.set_token(nullptr, 1U), castle::status::invalid_argument);
    EXPECT_EQ(value.set_payload(nullptr, 0U), castle::status::ok);
    EXPECT_EQ(value.set_payload(nullptr, 1U), castle::status::invalid_argument);
    EXPECT_EQ(value.append_option(0U, nullptr, 0U), castle::status::invalid_argument);
    EXPECT_EQ(value.append_option(5U, nullptr, 1U), castle::status::invalid_argument);
    EXPECT_EQ(value.append_option(5U, nullptr, 0U), castle::status::ok);
    const uint8_t nine[9] = {};
    EXPECT_EQ(value.append_option(11U, nine, sizeof(nine)), castle::status::out_of_range);
    EXPECT_EQ(value.append_string_option(11U, nullptr), castle::status::invalid_argument);
    EXPECT_EQ(value.append_uint_option(12U, 0U), castle::status::ok);
    EXPECT_EQ(value.append_uint_option(14U, 0x01020304U), castle::status::ok);
    ASSERT_EQ(value.option_count, 3U);
    EXPECT_EQ(value.options[1U].length, 0U);
    EXPECT_EQ(value.options[2U].length, 4U);
}

TEST(CoapTypes, UriHelpersRejectNullAndFullMessages)
{
    message<1U, 16U, 8U> path_target;
    EXPECT_EQ(append_uri_path(path_target, nullptr), castle::status::invalid_argument);
    EXPECT_EQ(append_uri_path(path_target, "a/b"), castle::status::full);
    EXPECT_EQ(path_target.option_count, 1U);

    message<1U, 16U, 8U> query_target;
    EXPECT_EQ(append_uri_query(query_target, nullptr), castle::status::invalid_argument);
    EXPECT_EQ(append_uri_query(query_target, "&a&"), castle::status::ok);
    EXPECT_EQ(query_target.option_count, 1U);
    EXPECT_EQ(append_uri_query(query_target, "b=1&c=2"), castle::status::full);

    message<4U, 8U, 8U> long_segment;
    EXPECT_EQ(append_uri_path(long_segment, "ABCDEFGHI"), castle::status::out_of_range);
    EXPECT_EQ(append_uri_query(long_segment, "ABCDEFGHI"), castle::status::out_of_range);
}

TEST(CoapTypes, UriPathMatchingRejectsMismatches)
{
    message<4U, 8U, 8U> request;
    EXPECT_FALSE(uri_path_matches(request, nullptr));
    EXPECT_FALSE(uri_path_matches(request, ""));
    ASSERT_EQ(append_uri_path(request, "abc"), castle::status::ok);
    EXPECT_FALSE(uri_path_matches(request, "xyz"));
    EXPECT_FALSE(uri_path_matches(request, "/"));
    EXPECT_TRUE(uri_path_matches(request, "abc"));

    message<4U, 8U, 8U> bare;
    EXPECT_FALSE(uri_path_matches(bare, ""));
}

TEST(CoapTypes, CodeClassPredicatesAtBoundaries)
{
    EXPECT_FALSE(is_request_code(static_cast<uint8_t>(0x00U)));
    EXPECT_TRUE(is_request_code(static_cast<uint8_t>(0x01U)));
    EXPECT_FALSE(is_response_code(0x00U));
    EXPECT_FALSE(is_response_code(0x20U));
    EXPECT_TRUE(is_response_code(0x80U));
    EXPECT_TRUE(is_response_code(0xA0U));
}

TEST(CoapTypes, FindOptionHandlesMissingAndConstAccess)
{
    message<4U, 8U, 8U> value;
    const uint8_t byte = 1U;
    ASSERT_EQ(value.append_option(11U, &byte, 1U), castle::status::ok);
    ASSERT_EQ(value.append_option(15U, &byte, 1U), castle::status::ok);
    message<4U, 8U, 8U> const& view = value;
    EXPECT_EQ(view.find_option(13U), nullptr);
    EXPECT_NE(view.find_option(15U), nullptr);
    EXPECT_EQ(value.find_option(9U), nullptr);
    EXPECT_NE(value.find_option(11U), nullptr);
}
