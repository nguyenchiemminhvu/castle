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

TEST(HttpTypes, SharedValidationHelpers)
{
    EXPECT_TRUE(detail::is_token("GET"));
    EXPECT_FALSE(detail::is_token(""));
    EXPECT_FALSE(detail::is_token("a b"));
    EXPECT_TRUE(detail::is_visible_text("api.test:80"));
    EXPECT_FALSE(detail::is_visible_text(""));
    EXPECT_FALSE(detail::is_visible_text("a b"));
    EXPECT_FALSE(detail::is_visible_text("a\r\nb"));
    uint64_t value = 0U;
    EXPECT_TRUE(detail::parse_decimal("18446744073709551615", value));
    EXPECT_EQ(value, 18446744073709551615ULL);
    EXPECT_FALSE(detail::parse_decimal("18446744073709551616", value));
    EXPECT_FALSE(detail::parse_decimal("12a", value));
    EXPECT_FALSE(detail::parse_decimal("", value));
}

TEST(HttpTypes, TokenAndLowercaseCharactersOverEveryByte)
{
    char const special[] = "!#$%&'*+-.^_`|~";
    for (int i = 0; i < 256; ++i)
    {
        char c = static_cast<char>(i);
        bool in_special = false;
        for (castle::size_type k = 0U; special[k] != '\0'; ++k)
        {
            in_special = in_special || special[k] == c;
        }
        bool alnum = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        EXPECT_EQ(detail::token_char(c), alnum || in_special) << "byte " << i;
        bool lowercase = i > 32 && i < 127 && !(i >= 'A' && i <= 'Z');
        EXPECT_EQ(ascii_lowercase(string_view(&c, 1U)), lowercase) << "byte " << i;
    }
}

TEST(HttpTypes, SharedTextHelpersOverEveryByte)
{
    for (int i = 0; i < 256; ++i)
    {
        char c = static_cast<char>(i);
        unsigned char u = static_cast<unsigned char>(i);
        EXPECT_EQ(detail::is_visible_text(string_view(&c, 1U)), u > 32U && u != 127U) << "byte " << i;
        uint64_t value = 0U;
        bool digit = c >= '0' && c <= '9';
        EXPECT_EQ(detail::parse_decimal(string_view(&c, 1U), value), digit) << "byte " << i;
        if (digit)
        {
            EXPECT_EQ(value, static_cast<uint64_t>(c - '0'));
        }
    }
    EXPECT_TRUE(detail::is_token("Token-1"));
    EXPECT_FALSE(detail::is_token(""));
    EXPECT_FALSE(detail::is_visible_text(""));
}
