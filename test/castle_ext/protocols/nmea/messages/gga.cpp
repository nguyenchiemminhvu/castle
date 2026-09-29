#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/gga.hpp"

namespace
{
using namespace castle_ext::protocols::nmea;
using namespace castle_ext::protocols::nmea::messages;
using castle::container::string_view;

TEST(NmeaMessageGgaMirror, DecodesFullFixAndReportsAvailability)
{
    string_view storage[16U]; message_view raw;
    ASSERT_TRUE(decode_sentence(string_view("$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,0.5,123"), storage, 16U, raw));
    gga out;
    ASSERT_TRUE(gga::decode(raw, out));
    EXPECT_TRUE(gga::matches(raw));
    EXPECT_TRUE(out.fix_available());
    EXPECT_EQ(out.fix_quality, gga_fix_quality::gps_sps);
    EXPECT_EQ(out.num_satellites, 8U);
    EXPECT_DOUBLE_EQ(out.dgps_age, 0.5);
    EXPECT_EQ(out.dgps_station_id, 123U);
    EXPECT_NEAR(out.latitude, 47.2852331667, 1e-9);
    EXPECT_NEAR(out.longitude, 8.565265, 1e-9);
}

TEST(NmeaMessageGgaMirror, FailedNumericMandatoryFieldMakesDecodeInvalid)
{
    string_view storage[16U]; message_view raw;
    ASSERT_TRUE(decode_sentence(string_view("GPGGA,bad,4717.11399,N,00833.91590,E,0,08,1.01,499.6,M,48.0,M,,"), storage, 16U, raw));
    gga out;
    EXPECT_FALSE(gga::decode(raw, out));
    EXPECT_FALSE(out.valid);
    EXPECT_FALSE(out.fix_available());
}

} // namespace
