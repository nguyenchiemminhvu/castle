#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/gns.hpp"

namespace
{

using namespace castle::protocols::nmea;
using namespace castle::protocols::nmea::messages;
using castle::container::string_view;

TEST(NmeaMessageGnsMirror, DecodesOptionalTrailingFields)
{
    string_view storage[16U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPGNS,092725.00,4717.0,N,00833.0,E,AA,08,1.1,500.0,48.0,0.5,22"),
        storage, 16U, raw));

    gns out;
    ASSERT_TRUE(gns::decode(raw, out));
    EXPECT_TRUE(gns::matches(raw));
    EXPECT_EQ(out.mode_indicator, string_view("AA"));
    EXPECT_EQ(out.num_sats, 8U);
    EXPECT_EQ(out.diff_ref, 22);
    EXPECT_DOUBLE_EQ(out.diff_age, 0.5);
    EXPECT_TRUE(out.valid);
}

TEST(NmeaMessageGnsMirror, UsesDefaultsWhenOptionalFieldsAreAbsent)
{
    string_view storage[12U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPGNS,1,4717.0,N,00833.0,E,A,8,1.0"), storage, 12U, raw));

    gns out;
    ASSERT_TRUE(gns::decode(raw, out));
    EXPECT_DOUBLE_EQ(out.altitude, 0.0);
    EXPECT_DOUBLE_EQ(out.geoid_sep, 0.0);
    EXPECT_DOUBLE_EQ(out.diff_age, -1.0);
    EXPECT_EQ(out.diff_ref, -1);
}

} // namespace
