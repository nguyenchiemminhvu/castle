#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/gsa.hpp"

namespace
{

using namespace castle_ext::protocols::nmea;
using namespace castle_ext::protocols::nmea::messages;
using castle::container::string_view;

TEST(NmeaMessageGsaMirror, DecodesSatellitesAndSystemId)
{
    string_view storage[20U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.1,0.9,1.2,2"),
        storage, 20U, raw));

    gsa out;
    ASSERT_TRUE(gsa::decode(raw, out));
    EXPECT_TRUE(gsa::matches(raw));
    EXPECT_EQ(out.op_mode, gsa_op_mode::auto_mode);
    EXPECT_EQ(out.nav_mode, gsa_nav_mode::fix_3d);
    EXPECT_EQ(out.sat_ids[0U], 1U);
    EXPECT_EQ(out.sat_ids[11U], 12U);
    EXPECT_DOUBLE_EQ(out.pdop, 1.1);
    EXPECT_EQ(out.system_id, 2U);
}

TEST(NmeaMessageGsaMirror, UnknownOpModeDoesNotCorruptDecode)
{
    string_view storage[20U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPGSA,X,2,,,,,,,,,,,,,1.0,2.0,3.0"), storage, 20U, raw));

    gsa out;
    ASSERT_TRUE(gsa::decode(raw, out));
    EXPECT_EQ(out.op_mode, gsa_op_mode::unknown);
    EXPECT_EQ(out.nav_mode, gsa_nav_mode::fix_2d);
}

TEST(NmeaMessageGsaMirror, RejectsShortSentence)
{
    string_view storage[4U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(string_view("GPGSA,A,3"), storage, 4U, raw));

    gsa out;
    EXPECT_FALSE(gsa::decode(raw, out));
}

} // namespace
