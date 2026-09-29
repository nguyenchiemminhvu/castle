#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/field_cursor.hpp"

namespace
{

using namespace castle::protocols::nmea;
using castle::container::array_view;
using castle::container::string_view;

TEST(NmeaFieldCursorMirror, AdvancesAndParsesPrimitiveFields)
{
    string_view storage[8U] = {
        string_view("123.5"), string_view("42"), string_view("2A"),
        string_view("Q"), string_view("4717.0"), string_view("S"),
        string_view(""), string_view("17")};

    message_view raw;
    raw.fields = array_view<CASTLE_CONST string_view>(storage, 8U);
    field_cursor cursor(raw);

    EXPECT_EQ(cursor.index(), 0U);
    EXPECT_EQ(cursor.next(), string_view("123.5"));
    double value = 0.0;
    ASSERT_TRUE(cursor.next_double(value));
    EXPECT_DOUBLE_EQ(value, 42.0);

    uint32_t number = 0U;
    ASSERT_TRUE(cursor.next_uint(number, 16));
    EXPECT_EQ(number, 0x2AU);

    char character = '\0';
    ASSERT_TRUE(cursor.next_char(character));
    EXPECT_EQ(character, 'Q');

    ASSERT_TRUE(cursor.next_latlon(value));
    EXPECT_NEAR(value, -(47.0 + 17.0 / 60.0), 1e-12);
    EXPECT_EQ(cursor.index(), 6U);

    EXPECT_EQ(cursor.next_double_or(9.5), 9.5);
    EXPECT_EQ(cursor.next_uint_or(7U), 17U);
    EXPECT_EQ(cursor.index(), 8U);
}

TEST(NmeaFieldCursorMirror, OptionalParsingPreservesFallbackForBadFields)
{
    string_view storage[3U] = {string_view("bad"), string_view(""), string_view("GG")};
    message_view raw;
    raw.fields = array_view<CASTLE_CONST string_view>(storage, 3U);
    field_cursor cursor(raw);

    EXPECT_EQ(cursor.next_double_or(3.25), 3.25);
    EXPECT_EQ(cursor.next_uint_or(8U), 8U);
    EXPECT_EQ(cursor.next_uint_or(9U, 16), 9U);
}

TEST(NmeaFieldCursorMirror, SkipAndOutOfRangeFieldsAreSafe)
{
    string_view storage[1U] = {string_view("1")};
    message_view raw;
    raw.fields = array_view<CASTLE_CONST string_view>(storage, 1U);
    field_cursor cursor(raw);

    cursor.skip(3U);
    EXPECT_EQ(cursor.index(), 3U);
    EXPECT_TRUE(cursor.next().empty());
    EXPECT_TRUE(cursor.next().empty());
    EXPECT_EQ(cursor.index(), 5U);
}

} // namespace
