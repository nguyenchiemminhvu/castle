#include <gtest/gtest.h>

#include "castle_ext/geodetic/geo_utils.hpp"

namespace
{

TEST(GeodeticGeoUtils, IsValidLatitude)
{
    static_assert(castle::geodetic::is_valid_latitude(0.0), "zero latitude");
    static_assert(castle::geodetic::is_valid_latitude(90.0), "north pole");
    static_assert(castle::geodetic::is_valid_latitude(-90.0), "south pole");
    static_assert(!castle::geodetic::is_valid_latitude(90.0001), "just above range");
    static_assert(!castle::geodetic::is_valid_latitude(-90.0001), "just below range");

    EXPECT_TRUE(castle::geodetic::is_valid_latitude(45.0));
    EXPECT_FALSE(castle::geodetic::is_valid_latitude(100.0));
    EXPECT_FALSE(castle::geodetic::is_valid_latitude(-100.0));
}

TEST(GeodeticGeoUtils, IsValidLongitude)
{
    static_assert(castle::geodetic::is_valid_longitude(0.0), "zero longitude");
    static_assert(castle::geodetic::is_valid_longitude(180.0), "east antimeridian");
    static_assert(castle::geodetic::is_valid_longitude(-180.0), "west antimeridian");
    static_assert(!castle::geodetic::is_valid_longitude(180.0001), "just above range");

    EXPECT_TRUE(castle::geodetic::is_valid_longitude(105.8342));
    EXPECT_FALSE(castle::geodetic::is_valid_longitude(200.0));
    EXPECT_FALSE(castle::geodetic::is_valid_longitude(-200.0));
}

TEST(GeodeticGeoUtils, ClampLatitude)
{
    static_assert(castle::geodetic::clamp_latitude(45.0) == 45.0, "within range");
    static_assert(castle::geodetic::clamp_latitude(120.0) == 90.0, "above range");
    static_assert(castle::geodetic::clamp_latitude(-120.0) == -90.0, "below range");

    EXPECT_DOUBLE_EQ(castle::geodetic::clamp_latitude(45.0), 45.0);
    EXPECT_DOUBLE_EQ(castle::geodetic::clamp_latitude(120.0), 90.0);
    EXPECT_DOUBLE_EQ(castle::geodetic::clamp_latitude(-120.0), -90.0);
}

TEST(GeodeticGeoUtils, NormalizeLongitude)
{
    EXPECT_NEAR(castle::geodetic::normalize_longitude(0.0), 0.0, 1e-9);
    EXPECT_NEAR(castle::geodetic::normalize_longitude(180.0), -180.0, 1e-9);
    EXPECT_NEAR(castle::geodetic::normalize_longitude(-180.0), -180.0, 1e-9);
    EXPECT_NEAR(castle::geodetic::normalize_longitude(190.0), -170.0, 1e-9);
    EXPECT_NEAR(castle::geodetic::normalize_longitude(-190.0), 170.0, 1e-9);
    EXPECT_NEAR(castle::geodetic::normalize_longitude(360.0), 0.0, 1e-9);
}

} // namespace
