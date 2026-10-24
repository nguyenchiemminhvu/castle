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
