#include <gtest/gtest.h>
#include "castle_ext/protocols/http/types.hpp"

using namespace castle::protocols::http;

TEST(HttpTypes, MethodTokensAndCustomMethod)
{
    EXPECT_EQ(method_token(method::get).size(), 3U);
    EXPECT_EQ(method_token(method::post).size(), 4U);
    EXPECT_EQ(method_token(method::put).size(), 3U);
    EXPECT_EQ(method_token(method::delete_).size(), 6U);
    EXPECT_EQ(method_token(method::head).size(), 4U);
    EXPECT_EQ(method_token(method::options).size(), 7U);
    EXPECT_EQ(method_token(method::patch).size(), 5U);
    EXPECT_TRUE(method_token(method::custom).empty());
}

TEST(HttpTypes, AsciiHelpersAndStatusMapping)
{
    EXPECT_TRUE(ascii_iequal("Content-Type", "content-type"));
    EXPECT_FALSE(ascii_iequal("content-type", "content-length"));
    EXPECT_TRUE(ascii_lowercase("x-header-1"));
    EXPECT_FALSE(ascii_lowercase("X-Header"));
    EXPECT_FALSE(ascii_lowercase("two words"));
    EXPECT_EQ(to_status(error_code::ok), castle::status::ok);
    EXPECT_EQ(to_status(error_code::header_limit), castle::status::out_of_range);
    EXPECT_EQ(to_status(error_code::connection_closed), castle::status::data_loss);
    EXPECT_EQ(to_status(error_code::request_in_progress), castle::status::invalid_config);
    EXPECT_EQ(to_status(error_code::chunked_not_supported), castle::status::invalid_config);
    EXPECT_EQ(to_status(error_code::protocol_error), castle::status::invalid_argument);
    EXPECT_EQ(to_status(error_code::stream_reset), castle::status::data_loss);
}
