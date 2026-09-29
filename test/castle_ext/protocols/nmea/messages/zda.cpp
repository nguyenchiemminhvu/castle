#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/zda.hpp"

namespace
{

using namespace castle_ext::protocols::nmea;
using namespace castle_ext::protocols::nmea::messages;
using castle::container::string_view;

TEST(NmeaMessageZdaMirror, DecodesDateAndNegativeTimezone)
{
    string_view storage[8U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPZDA,092725.00,15,06,2024,-5,30"), storage, 8U, raw));

    zda out;
    ASSERT_TRUE(zda::decode(raw, out));
    EXPECT_TRUE(zda::matches(raw));
    EXPECT_EQ(out.day, 15U);
    EXPECT_EQ(out.month, 6U);
    EXPECT_EQ(out.year, 2024U);
    EXPECT_EQ(out.tz_hour, -5);
    EXPECT_EQ(out.tz_min, 30U);
}

TEST(NmeaMessageZdaMirror, AcceptsMinimalRequiredFields)
{
    string_view storage[6U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(string_view("GPZDA,1,2,3,2024"), storage, 6U, raw));

    zda out;
    ASSERT_TRUE(zda::decode(raw, out));
    EXPECT_EQ(out.year, 2024U);
    EXPECT_EQ(out.tz_hour, 0);
    EXPECT_EQ(out.tz_min, 0U);
}

} // namespace
