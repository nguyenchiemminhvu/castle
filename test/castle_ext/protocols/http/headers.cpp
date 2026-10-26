#include <gtest/gtest.h>
#include "castle_ext/protocols/http/headers.hpp"

using namespace castle::protocols::http;

TEST(HttpHeaders, AddLookupReplaceAndRemove)
{
    basic_headers<3U, 16U, 32U> headers;
    EXPECT_TRUE(headers.empty());
    EXPECT_EQ(headers.add("accept", "application/json"), castle::status::ok);
    EXPECT_EQ(headers.add("user-agent", "castle"), castle::status::ok);
    EXPECT_EQ(headers.size(), 2U);
    EXPECT_TRUE(headers.contains("ACCEPT"));
    EXPECT_TRUE(ascii_iequal(headers.get("Accept"), "application/json"));
    EXPECT_EQ(headers.set("ACCEPT", "text/plain"), castle::status::ok);
    EXPECT_TRUE(ascii_iequal(headers.get("accept"), "text/plain"));
    EXPECT_TRUE(headers.remove("user-agent"));
    EXPECT_FALSE(headers.remove("missing"));
    EXPECT_EQ(headers.size(), 1U);
    EXPECT_EQ(headers[99U].name.size(), 0U);
    headers.clear();
    EXPECT_TRUE(headers.empty());
}

TEST(HttpHeaders, ValidatesNamesValuesAndCapacity)
{
    basic_headers<1U, 4U, 4U> headers;
    EXPECT_EQ(headers.add("", "x"), castle::status::invalid_argument);
    EXPECT_EQ(headers.add("bad name", "x"), castle::status::invalid_argument);
    EXPECT_EQ(headers.add("x", "a\r\nb"), castle::status::invalid_argument);
    EXPECT_EQ(headers.add("x", "a\0b"), castle::status::ok); // the C-string ends at NUL
    EXPECT_TRUE(headers.full());
    EXPECT_EQ(headers.add("y", "z"), castle::status::full);
    EXPECT_EQ(headers.set("longer", "x"), castle::status::out_of_range);
    EXPECT_EQ(headers.set("x", "12345"), castle::status::out_of_range);
}

TEST(HttpHeaders, PseudoHeadersAndDuplicatesAreSupported)
{
    basic_headers<3U, 16U, 32U> headers;
    EXPECT_EQ(headers.add(":method", "GET"), castle::status::ok);
    EXPECT_EQ(headers.add("set-cookie", "a=1"), castle::status::ok);
    EXPECT_EQ(headers.add("set-cookie", "b=2"), castle::status::ok);
    EXPECT_EQ(headers.size(), 3U);
    EXPECT_TRUE(ascii_iequal(headers[0U].name, ":method"));
    EXPECT_TRUE(ascii_iequal(headers.get("set-cookie"), "a=1"));
}

TEST(HttpHeaders, AliasUsesDefaultCapacities)
{
    castle::protocols::http::headers<> fields;
    EXPECT_EQ(fields.add("x-id", "1"), castle::status::ok);
    EXPECT_TRUE(fields.contains("X-ID"));
}

TEST(HttpHeaders, SetValidatesAndReplacesOnlyMatchingFields)
{
    basic_headers<3U, 8U, 16U> headers;
    EXPECT_EQ(headers.set("x-a", "1"), castle::status::ok);
    EXPECT_EQ(headers.set("x-b", "2"), castle::status::ok);
    EXPECT_EQ(headers.set("X-A", "3"), castle::status::ok);
    EXPECT_TRUE(ascii_iequal(headers.get("x-a"), "3"));
    EXPECT_TRUE(ascii_iequal(headers.get("x-b"), "2"));
    EXPECT_EQ(headers.set("bad name", "x"), castle::status::invalid_argument);
    EXPECT_EQ(headers.set("x", "a\r\n"), castle::status::invalid_argument);
    EXPECT_EQ(headers.set("x-long-name", "y"), castle::status::out_of_range);
    EXPECT_EQ(headers.set("x-c", "value-that-is-too-long"), castle::status::out_of_range);
}

TEST(HttpHeaders, LookupsAndRemovalCoverEveryPosition)
{
    basic_headers<3U, 8U, 16U> headers;
    ASSERT_EQ(headers.add("a", "1"), castle::status::ok);
    ASSERT_EQ(headers.add("b", "2"), castle::status::ok);
    EXPECT_FALSE(headers.contains("missing"));
    EXPECT_TRUE(headers.get("missing").empty());
    EXPECT_TRUE(headers.remove("a"));
    EXPECT_TRUE(ascii_iequal(headers[0U].name, "b"));
    EXPECT_FALSE(headers.contains("a"));
    EXPECT_EQ(headers.add(":", "x"), castle::status::invalid_argument);
}

TEST(HttpHeaders, NameAndValueBytesFollowSyntaxOverAsciiRange)
{
    basic_headers<1U, 4U, 4U> headers;
    for (int i = 0; i < 128; ++i)
    {
        char c = static_cast<char>(i);
        bool value_ok = c == '\t' || (i >= 32 && i != 127);
        headers.clear();
        EXPECT_EQ(headers.add("x", string_view(&c, 1U)) == castle::status::ok, value_ok) << "value byte " << i;
        headers.clear();
        EXPECT_EQ(headers.add(string_view(&c, 1U), "v") == castle::status::ok, detail::token_char(c)) << "name byte " << i;
    }
}

TEST(Headers, RejectsNamesAndValuesBeyondCapacity)
{
    basic_headers<2U, 8U, 8U> headers;
    EXPECT_EQ(headers.add("123456789", "v"), castle::status::out_of_range);
    EXPECT_EQ(headers.add("n", "123456789"), castle::status::out_of_range);
}
