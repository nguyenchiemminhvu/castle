#include <gtest/gtest.h>

#include "castle/core/types.hpp"
#include "castle/utility/string_builder.hpp"
#include "castle_ext/geodetic/coord.hpp"

namespace
{

using castle::geodetic::geo_point;

TEST(GeodeticCoord, GeoPointConstructionAndAccessors)
{
    constexpr geo_point<double> origin;
    constexpr geo_point<double> hanoi(21.0278, 105.8342);

    static_assert(origin.latitude() == 0.0 && origin.longitude() == 0.0, "default point");
    static_assert(hanoi.latitude() == 21.0278, "latitude accessor");
    static_assert(hanoi.longitude() == 105.8342, "longitude accessor");

    EXPECT_DOUBLE_EQ(hanoi.latitude(), 21.0278);
    EXPECT_DOUBLE_EQ(hanoi.longitude(), 105.8342);
}

TEST(GeodeticCoord, GeoPointComparison)
{
    constexpr geo_point<double> a(10.0, 20.0);
    constexpr geo_point<double> b(10.0, 20.0);
    constexpr geo_point<double> c(10.0, 21.0);

    static_assert(a == b, "equal points");
    static_assert(a != c, "different points");

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(GeodeticCoord, DegreesToDmsRoundTrip)
{
    constexpr castle::geodetic::dms<double> value = castle::geodetic::degrees_to_dms(51.4779);
    static_assert(!value.negative, "north");
    static_assert(value.degrees == 51U, "whole degrees");
    static_assert(value.minutes == 28U, "whole minutes");

    EXPECT_FALSE(value.negative);
    EXPECT_EQ(value.degrees, 51U);
    EXPECT_EQ(value.minutes, 28U);
    EXPECT_NEAR(value.seconds, 40.44, 1e-2);

    constexpr double round_trip = castle::geodetic::dms_to_degrees(value);
    EXPECT_NEAR(round_trip, 51.4779, 1e-9);
}

TEST(GeodeticCoord, DegreesToDmsNegative)
{
    constexpr castle::geodetic::dms<double> value = castle::geodetic::degrees_to_dms(-21.0278);
    static_assert(value.negative, "south");

    EXPECT_TRUE(value.negative);
    EXPECT_EQ(value.degrees, 21U);

    constexpr double round_trip = castle::geodetic::dms_to_degrees(value);
    EXPECT_NEAR(round_trip, -21.0278, 1e-9);
}

TEST(GeodeticCoord, DegreesToDdmRoundTrip)
{
    constexpr castle::geodetic::ddm<double> value = castle::geodetic::degrees_to_ddm(105.8342);
    static_assert(!value.negative, "east");
    static_assert(value.degrees == 105U, "whole degrees");

    EXPECT_FALSE(value.negative);
    EXPECT_EQ(value.degrees, 105U);
    EXPECT_NEAR(value.minutes, 50.052, 1e-3);

    constexpr double round_trip = castle::geodetic::ddm_to_degrees(value);
    EXPECT_NEAR(round_trip, 105.8342, 1e-9);
}

TEST(GeodeticCoord, AngleUnitConversions)
{
    static_assert(castle::geodetic::degrees_to_arcseconds(1.0) == 3600.0, "arcseconds");
    static_assert(castle::geodetic::arcseconds_to_degrees(3600.0) == 1.0, "arcseconds inverse");
    static_assert(castle::geodetic::degrees_to_milliarcseconds(1.0) == 3600000.0, "mas");
    static_assert(castle::geodetic::milliarcseconds_to_degrees(3600000.0) == 1.0, "mas inverse");
    static_assert(castle::geodetic::degrees_to_microarcseconds(1.0) == 3600000000.0, "uas");
    static_assert(castle::geodetic::microarcseconds_to_degrees(3600000000.0) == 1.0, "uas inverse");
    static_assert(castle::geodetic::degrees_to_gradians(90.0) == 100.0, "gradians");
    static_assert(castle::geodetic::gradians_to_degrees(100.0) == 90.0, "gradians inverse");
    static_assert(castle::geodetic::degrees_to_turns(360.0) == 1.0, "turns");
    static_assert(castle::geodetic::turns_to_degrees(1.0) == 360.0, "turns inverse");

    SUCCEED();
}

TEST(GeodeticCoord, AppendDms)
{
    CASTLE_CONST castle::geodetic::dms<double> lat = castle::geodetic::degrees_to_dms(51.4779);

    castle::string_builder<64> builder;
    castle::geodetic::append_dms(builder, lat, 'N', 'S');
    castle::string_builder<64> builder_lat;
    castle::geodetic::append_dms_lat(builder_lat, lat);

    CASTLE_CONST castle::size_type length = builder.size();
    ASSERT_GT(length, 0U);
    EXPECT_EQ(builder.c_str()[length - 1], 'N');
    EXPECT_EQ(builder_lat.c_str()[builder_lat.size() - 1], 'N');
}

TEST(GeodeticCoord, AppendDmsNegativeUsesNegativeSymbol)
{
    CASTLE_CONST castle::geodetic::dms<double> lon = castle::geodetic::degrees_to_dms(-0.001545);

    castle::string_builder<64> builder;
    castle::geodetic::append_dms(builder, lon, 'E', 'W');
    castle::string_builder<64> builder_lon;
    castle::geodetic::append_dms_lon(builder_lon, lon);

    CASTLE_CONST castle::size_type length = builder.size();
    ASSERT_GT(length, 0U);
    EXPECT_EQ(builder.c_str()[length - 1], 'W');
    EXPECT_EQ(builder_lon.c_str()[builder_lon.size() - 1], 'W');
}

} // namespace
