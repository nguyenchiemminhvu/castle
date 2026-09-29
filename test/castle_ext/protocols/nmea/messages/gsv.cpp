#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/gsv.hpp"

namespace
{

using namespace castle::protocols::nmea;
using namespace castle::protocols::nmea::messages;
using castle::container::string_view;

TEST(NmeaMessageGsvMirror, DecodesSatelliteBlockAndSignalId)
{
    string_view storage[12U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPGSV,2,1,08,01,45,123,40,02,-10,180,-1,7"),
        storage, 12U, raw));

    gsv out;
    ASSERT_TRUE(gsv::decode(raw, out));
    EXPECT_TRUE(gsv::matches(raw));
    EXPECT_EQ(out.num_msgs, 2U);
    EXPECT_EQ(out.msg_num, 1U);
    EXPECT_EQ(out.sats_in_view, 8U);
    EXPECT_EQ(out.sat_count, 2U);
    EXPECT_EQ(out.satellites[0U].sv_id, 1U);
    EXPECT_EQ(out.satellites[0U].elevation, 45);
    EXPECT_EQ(out.satellites[0U].azimuth, 123U);
    EXPECT_EQ(out.satellites[0U].snr, 40);
    EXPECT_EQ(out.signal_id, 7U);
}

TEST(NmeaMessageGsvMirror, InvalidOptionalNumericFieldsKeepDefaults)
{
    string_view storage[8U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPGSV,1,1,01,01,bad,bad,bad"), storage, 8U, raw));

    gsv out;
    ASSERT_TRUE(gsv::decode(raw, out));
    EXPECT_EQ(out.sat_count, 1U);
    EXPECT_EQ(out.satellites[0U].elevation, -1);
    EXPECT_EQ(out.satellites[0U].azimuth, 0U);
    EXPECT_EQ(out.satellites[0U].snr, -1);
}

TEST(NmeaMessageGsvMirror, RejectsInvalidHeaderNumbers)
{
    string_view storage[4U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(string_view("GPGSV,bad,1,1"), storage, 4U, raw));

    gsv out;
    EXPECT_FALSE(gsv::decode(raw, out));
}

} // namespace
