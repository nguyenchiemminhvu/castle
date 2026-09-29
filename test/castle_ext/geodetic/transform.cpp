#include <gtest/gtest.h>

#include "castle_ext/geodetic/transform.hpp"

namespace
{

using castle::geodetic::geo_point;

TEST(GeodeticTransform, IsInGcj02Region)
{
    constexpr geo_point<double> beijing(39.8946, 116.291);
    constexpr geo_point<double> paris(48.8566, 2.3522);

    static_assert(castle::geodetic::is_in_gcj02_region(beijing), "beijing is inside China");
    static_assert(!castle::geodetic::is_in_gcj02_region(paris), "paris is outside China");

    EXPECT_TRUE(castle::geodetic::is_in_gcj02_region(beijing));
    EXPECT_FALSE(castle::geodetic::is_in_gcj02_region(paris));
}

TEST(GeodeticTransform, Wgs84ToGcj02KnownOffset)
{
    // Reference offset for this coordinate is well documented as being on
    // the order of a few hundred meters (roughly 0.001-0.006 degrees).
    CASTLE_CONST geo_point<double> beijing_wgs84(39.8946, 116.291);
    CASTLE_CONST geo_point<double> beijing_gcj02 = castle::geodetic::wgs84_to_gcj02(beijing_wgs84);

    EXPECT_NE(beijing_gcj02.latitude(), beijing_wgs84.latitude());
    EXPECT_NE(beijing_gcj02.longitude(), beijing_wgs84.longitude());
    EXPECT_NEAR(beijing_gcj02.latitude(), beijing_wgs84.latitude(), 0.01);
    EXPECT_NEAR(beijing_gcj02.longitude(), beijing_wgs84.longitude(), 0.01);

    /**
     * GCJ-02 (China Shifted) Coordinates: Latitude: 39.89584617154926, Longitude: 116.2970566819124
     */
    EXPECT_NEAR(beijing_gcj02.latitude(), 39.89584617154926, 1e-6);
    EXPECT_NEAR(beijing_gcj02.longitude(), 116.2970566819124, 1e-6);
}

TEST(GeodeticTransform, Wgs84ToGcj02OutsideChinaIsIdentity)
{
    CASTLE_CONST geo_point<double> paris(48.8566, 2.3522);
    CASTLE_CONST geo_point<double> transformed = castle::geodetic::wgs84_to_gcj02(paris);

    EXPECT_TRUE(transformed == paris);
}

TEST(GeodeticTransform, Gcj02ToWgs84RoundTrip)
{
    CASTLE_CONST geo_point<double> beijing_wgs84(39.8946, 116.291);
    CASTLE_CONST geo_point<double> beijing_gcj02 = castle::geodetic::wgs84_to_gcj02(beijing_wgs84);
    CASTLE_CONST geo_point<double> round_trip = castle::geodetic::gcj02_to_wgs84(beijing_gcj02);

    EXPECT_NEAR(round_trip.latitude(), beijing_wgs84.latitude(), 1e-5);
    EXPECT_NEAR(round_trip.longitude(), beijing_wgs84.longitude(), 1e-5);
}

TEST(GeodeticTransform, Gcj02ToWgs84OutsideChinaIsIdentity)
{
    CASTLE_CONST geo_point<double> paris(48.8566, 2.3522);
    CASTLE_CONST geo_point<double> transformed = castle::geodetic::gcj02_to_wgs84(paris);

    EXPECT_TRUE(transformed == paris);
}

TEST(GeodeticTransform, Gcj02ToWgs84CustomIterationCount)
{
    CASTLE_CONST geo_point<double> beijing_wgs84(39.8946, 116.291);
    CASTLE_CONST geo_point<double> beijing_gcj02 = castle::geodetic::wgs84_to_gcj02(beijing_wgs84);
    CASTLE_CONST geo_point<double> round_trip = castle::geodetic::gcj02_to_wgs84<double, 5U>(beijing_gcj02);

    EXPECT_NEAR(round_trip.latitude(), beijing_wgs84.latitude(), 1e-5);
    EXPECT_NEAR(round_trip.longitude(), beijing_wgs84.longitude(), 1e-5);
}

} // namespace
