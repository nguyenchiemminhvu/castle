#include <gtest/gtest.h>

#include "castle/error/status.hpp"
#include "castle/math/abs.hpp"
#include "castle_ext/geodetic/distance.hpp"

#include <math.h>

namespace
{

using castle_ext::geodetic::geo_point;

// New York City and Los Angeles, used because the great-circle distance
// between them is a commonly published reference value (~3936 km).
CASTLE_CONST geo_point<double> new_york(40.7128, -74.0060);
CASTLE_CONST geo_point<double> los_angeles(34.0522, -118.2437);

TEST(GeodeticDistance, HaversineDistanceKnownRoute)
{
    CASTLE_CONST double meters = castle_ext::geodetic::haversine_distance(new_york, los_angeles);
    EXPECT_NEAR(meters, 3936000.0, 15000.0);
}

TEST(GeodeticDistance, HaversineDistanceIsSymmetric)
{
    CASTLE_CONST double forward = castle_ext::geodetic::haversine_distance(new_york, los_angeles);
    CASTLE_CONST double backward = castle_ext::geodetic::haversine_distance(los_angeles, new_york);
    EXPECT_NEAR(forward, backward, 1e-6);
}

TEST(GeodeticDistance, HaversineDistanceZeroForSamePoint)
{
    EXPECT_NEAR(castle_ext::geodetic::haversine_distance(new_york, new_york), 0.0, 1e-6);
}

TEST(GeodeticDistance, InitialBearingIsNormalized)
{
    CASTLE_CONST double bearing = castle_ext::geodetic::initial_bearing(new_york, los_angeles);
    EXPECT_GE(bearing, 0.0);
    EXPECT_LT(bearing, 360.0);
    // NYC to LA heads broadly west-southwest.
    EXPECT_GT(bearing, 250.0);
    EXPECT_LT(bearing, 280.0);
}

TEST(GeodeticDistance, FinalBearingIsReciprocalOfReversedInitialBearing)
{
    CASTLE_CONST double final_deg = castle_ext::geodetic::final_bearing(new_york, los_angeles);
    CASTLE_CONST double reverse_initial = castle_ext::geodetic::initial_bearing(los_angeles, new_york);
    CASTLE_CONST double expected = ::fmod(reverse_initial + 180.0, 360.0);
    EXPECT_NEAR(final_deg, expected, 1e-6);
}

TEST(GeodeticDistance, VincentyInverseKnownRoute)
{
    castle_ext::geodetic::vincenty_result<double> result{};
    CASTLE_CONST castle::status status = castle_ext::geodetic::vincenty_inverse(new_york, los_angeles, result);

    ASSERT_TRUE(castle::succeeded(status));
    EXPECT_NEAR(result.distance, 3936000.0, 15000.0);
    EXPECT_GE(result.initial_bearing_deg, 0.0);
    EXPECT_LT(result.initial_bearing_deg, 360.0);
    EXPECT_GE(result.final_bearing_deg, 0.0);
    EXPECT_LT(result.final_bearing_deg, 360.0);
}

TEST(GeodeticDistance, VincentyInverseCoincidentPoints)
{
    castle_ext::geodetic::vincenty_result<double> result{};
    CASTLE_CONST castle::status status = castle_ext::geodetic::vincenty_inverse(new_york, new_york, result);

    ASSERT_TRUE(castle::succeeded(status));
    EXPECT_NEAR(result.distance, 0.0, 1e-6);
}

TEST(GeodeticDistance, VincentyDirectReversesInverse)
{
    castle_ext::geodetic::vincenty_result<double> inverse{};
    ASSERT_TRUE(castle::succeeded(castle_ext::geodetic::vincenty_inverse(new_york, los_angeles, inverse)));

    geo_point<double> destination;
    double final_bearing_deg = 0.0;
    CASTLE_CONST castle::status status = castle_ext::geodetic::vincenty_direct(
        new_york, inverse.initial_bearing_deg, inverse.distance, destination, final_bearing_deg);

    ASSERT_TRUE(castle::succeeded(status));
    EXPECT_NEAR(destination.latitude(), los_angeles.latitude(), 1e-4);
    EXPECT_NEAR(destination.longitude(), los_angeles.longitude(), 1e-4);
    EXPECT_NEAR(final_bearing_deg, inverse.final_bearing_deg, 1e-3);
}

TEST(GeodeticDistance, VincentyAndHaversineAgreeClosely)
{
    castle_ext::geodetic::vincenty_result<double> result{};
    ASSERT_TRUE(castle::succeeded(castle_ext::geodetic::vincenty_inverse(new_york, los_angeles, result)));

    CASTLE_CONST double haversine_m = castle_ext::geodetic::haversine_distance(new_york, los_angeles);

    // Vincenty (ellipsoidal) and Haversine (spherical) should agree to well
    // within the ~0.5% error bound of the spherical approximation.
    CASTLE_CONST double relative_error = castle::math::abs(result.distance - haversine_m) / result.distance;
    EXPECT_LT(relative_error, 0.01);
}

} // namespace
