#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/rmc.hpp"

namespace
{

using namespace castle_ext::protocols::nmea;
using namespace castle_ext::protocols::nmea::messages;
using castle::container::string_view;

TEST(NmeaMessageRmcMirror, DecodesStatusVariationAndMode)
{
    string_view storage[16U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPRMC,092725.00,A,4717.11399,N,00833.91590,E,22.4,84.4,230394,003.1,W,A"),
        storage, 16U, raw));

    rmc out;
    ASSERT_TRUE(rmc::decode(raw, out));
    EXPECT_TRUE(rmc::matches(raw));
    EXPECT_TRUE(out.status_active);
    EXPECT_EQ(out.date, 230394U);
    EXPECT_DOUBLE_EQ(out.mag_variation, -3.1);
    EXPECT_EQ(out.mode, rmc_mode::autonomous);
}

TEST(NmeaMessageRmcMirror, InvalidMandatoryFieldMakesDecodeInvalid)
{
    string_view storage[16U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPRMC,bad,A,4717.0,N,00833.0,E,1.0,2.0,230394,,,"),
        storage, 16U, raw));

    rmc out;
    EXPECT_FALSE(rmc::decode(raw, out));
    EXPECT_FALSE(out.valid);
}

} // namespace
