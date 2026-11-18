
#include <gtest/gtest.h>

#include "castle/math/geometry/azimuth.hpp"

#include <math.h>

namespace
{

using castle::math::point2d;
using castle::math::point3d;

constexpr double kPi = 3.14159265358979323846;

TEST(GeometryAzimuth, WrapAzimuthMapsIntoHalfOpenTurn)
{
    static_assert(castle::math::detail::wrap_azimuth(90.0, 360.0) == 90.0, "positive angle unchanged");
    static_assert(castle::math::detail::wrap_azimuth(0.0, 360.0) == 0.0, "zero unchanged");
    static_assert(castle::math::detail::wrap_azimuth(180.0, 360.0) == 180.0, "half turn unchanged");
    static_assert(castle::math::detail::wrap_azimuth(-90.0, 360.0) == 270.0, "negative angle wrapped");
    static_assert(castle::math::detail::wrap_azimuth(-180.0, 360.0) == 180.0, "negative half turn wrapped");
    static_assert(castle::math::detail::wrap_azimuth(-1.0e-300, 360.0) == 0.0, "rounded full turn folds to zero");

    EXPECT_DOUBLE_EQ(castle::math::detail::wrap_azimuth(-kPi * 0.5, 2.0 * kPi), kPi * 1.5);
    EXPECT_EQ(castle::math::detail::wrap_azimuth(-1.0e-30F, 360.0F), 0.0F);
}

TEST(GeometryAzimuth, SignedAzimuthGuardsZeroDisplacement)
{
    EXPECT_EQ(castle::math::detail::signed_azimuth(0.0, 0.0), 0.0);
    EXPECT_EQ(castle::math::detail::signed_azimuth(-0.0, -0.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::detail::signed_azimuth(0.0, -1.0), kPi);
    EXPECT_DOUBLE_EQ(castle::math::detail::signed_azimuth(-1.0, 0.0), -kPi * 0.5);
    EXPECT_DOUBLE_EQ(castle::math::detail::signed_azimuth(1.0, 0.0), kPi * 0.5);
}

TEST(GeometryAzimuth, MatchesBoostReferenceExample)
{
    const point2d<double> p1(0.0, 0.0);
    const point2d<double> p2(1.0, 1.0);

    EXPECT_NEAR(castle::math::azimuth(p1, p2), 0.785398, 1.0e-6);
    EXPECT_DOUBLE_EQ(castle::math::azimuth(p1, p2), kPi * 0.25);
    EXPECT_DOUBLE_EQ(castle::math::azimuth_degrees(p1, p2), 45.0);
}

TEST(GeometryAzimuth, CompassPointsInRadiansAndDegrees)
{
    const point2d<double> origin;

    EXPECT_DOUBLE_EQ(castle::math::azimuth(origin, point2d<double>(0.0, 1.0)), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::azimuth(origin, point2d<double>(1.0, 0.0)), kPi * 0.5);
    EXPECT_DOUBLE_EQ(castle::math::azimuth(origin, point2d<double>(0.0, -1.0)), kPi);
    EXPECT_DOUBLE_EQ(castle::math::azimuth(origin, point2d<double>(-1.0, 0.0)), kPi * 1.5);

    EXPECT_DOUBLE_EQ(castle::math::azimuth_degrees(origin, point2d<double>(0.0, 1.0)), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::azimuth_degrees(origin, point2d<double>(1.0, 0.0)), 90.0);
    EXPECT_DOUBLE_EQ(castle::math::azimuth_degrees(origin, point2d<double>(0.0, -1.0)), 180.0);
    EXPECT_DOUBLE_EQ(castle::math::azimuth_degrees(origin, point2d<double>(-1.0, 0.0)), 270.0);
}

TEST(GeometryAzimuth, IntercardinalQuadrants)
{
    const point2d<double> origin;

    EXPECT_DOUBLE_EQ(castle::math::azimuth_degrees(origin, point2d<double>(1.0, 1.0)), 45.0);
    EXPECT_DOUBLE_EQ(castle::math::azimuth_degrees(origin, point2d<double>(1.0, -1.0)), 135.0);
    EXPECT_DOUBLE_EQ(castle::math::azimuth_degrees(origin, point2d<double>(-1.0, -1.0)), 225.0);
    EXPECT_DOUBLE_EQ(castle::math::azimuth_degrees(origin, point2d<double>(-1.0, 1.0)), 315.0);

    EXPECT_DOUBLE_EQ(castle::math::azimuth(origin, point2d<double>(1.0, -1.0)), kPi * 0.75);
    EXPECT_DOUBLE_EQ(castle::math::azimuth(origin, point2d<double>(-1.0, -1.0)), kPi * 1.25);
    EXPECT_DOUBLE_EQ(castle::math::azimuth(origin, point2d<double>(-1.0, 1.0)), kPi * 1.75);
}

TEST(GeometryAzimuth, DependsOnlyOnDisplacementAndIsDirectional)
{
    const point2d<double> from(10.0, -5.0);
    const point2d<double> to(11.0, -5.0 + sqrt(3.0));

    EXPECT_NEAR(castle::math::azimuth_degrees(from, to), 30.0, 1.0e-12);
    EXPECT_NEAR(castle::math::azimuth(from, to), kPi / 6.0, 1.0e-12);

    // The reverse azimuth points the opposite way.
    EXPECT_NEAR(castle::math::azimuth_degrees(to, from), 210.0, 1.0e-12);
    EXPECT_NEAR(castle::math::azimuth(to, from), kPi * 7.0 / 6.0, 1.0e-12);
}

TEST(GeometryAzimuth, CoincidentPointsReturnZero)
{
    const point2d<double> p(3.5, -2.25);
    EXPECT_EQ(castle::math::azimuth(p, p), 0.0);
    EXPECT_EQ(castle::math::azimuth_degrees(p, p), 0.0);

    // -0 - (+0) yields negative zeros; atan2(-0, -0) would return -pi.
    const point2d<double> positive_zero(0.0, 0.0);
    const point2d<double> negative_zero(-0.0, -0.0);
    EXPECT_EQ(castle::math::azimuth(positive_zero, negative_zero), 0.0);
    EXPECT_EQ(castle::math::azimuth_degrees(positive_zero, negative_zero), 0.0);
}

TEST(GeometryAzimuth, JustWestOfNorthStaysBelowFullTurn)
{
    const point2d<float> origin;

    // atan2(-1e-30, 1) + 2*pi rounds to exactly 2*pi in float; result must fold to 0.
    const point2d<float> tiny_west(-1.0e-30F, 1.0F);
    EXPECT_EQ(castle::math::azimuth(origin, tiny_west), 0.0F);
    EXPECT_EQ(castle::math::azimuth_degrees(origin, tiny_west), 0.0F);

    const point2d<double> small_west(-1.0e-3, 1.0);
    const double radians = castle::math::azimuth(point2d<double>(), small_west);
    const double degrees = castle::math::azimuth_degrees(point2d<double>(), small_west);
    EXPECT_LT(radians, 2.0 * kPi);
    EXPECT_GT(radians, 2.0 * kPi - 2.0e-3);
    EXPECT_LT(degrees, 360.0);
    EXPECT_GT(degrees, 359.9);
}

TEST(GeometryAzimuth, FullSweepStaysInRangeAndMatchesHeading)
{
    const point2d<double> from(-4.0, 7.0);
    for (int heading = 0; heading < 360; ++heading)
    {
        const double expected_radians = static_cast<double>(heading) * kPi / 180.0;
        const point2d<double> to(from.x() + 5.0 * sin(expected_radians),
                                 from.y() + 5.0 * cos(expected_radians));
        const double radians = castle::math::azimuth(from, to);
        const double degrees = castle::math::azimuth_degrees(from, to);

        EXPECT_GE(radians, 0.0);
        EXPECT_LT(radians, 2.0 * kPi);
        EXPECT_GE(degrees, 0.0);
        EXPECT_LT(degrees, 360.0);
        EXPECT_NEAR(radians, expected_radians, 1.0e-9) << "heading " << heading;
        EXPECT_NEAR(degrees, static_cast<double>(heading), 1.0e-9) << "heading " << heading;
    }
}

TEST(GeometryAzimuth, FloatAndLongDoubleOverloads)
{
    EXPECT_NEAR(castle::math::azimuth(point2d<float>(1.0F, 1.0F), point2d<float>(0.0F, 2.0F)),
                static_cast<float>(kPi * 1.75), 1.0e-6F);
    EXPECT_NEAR(castle::math::azimuth_degrees(point2d<float>(1.0F, 1.0F), point2d<float>(0.0F, 2.0F)),
                315.0F, 1.0e-4F);

    EXPECT_NEAR(static_cast<double>(castle::math::azimuth(point2d<long double>(0.0L, 0.0L),
                                                          point2d<long double>(2.0L, 0.0L))),
                kPi * 0.5, 1.0e-12);
    EXPECT_NEAR(static_cast<double>(castle::math::azimuth_degrees(point2d<long double>(0.0L, 0.0L),
                                                                  point2d<long double>(0.0L, -3.0L))),
                180.0, 1.0e-12);
}

TEST(GeometryAzimuth, ThreeDimensionalIgnoresVerticalOffset)
{
    const point3d<double> from(1.0, 2.0, 3.0);
    const point3d<double> to(2.0, 1.0, 100.0);

    EXPECT_DOUBLE_EQ(castle::math::azimuth(from, to), kPi * 0.75);
    EXPECT_DOUBLE_EQ(castle::math::azimuth_degrees(from, to), 135.0);
    EXPECT_DOUBLE_EQ(castle::math::azimuth(from, point3d<double>(2.0, 1.0, -50.0)), kPi * 0.75);

    const point3d<double> west(0.0, 2.0, 3.0);
    EXPECT_DOUBLE_EQ(castle::math::azimuth_degrees(from, west), 270.0);
    EXPECT_DOUBLE_EQ(castle::math::azimuth(from, west), kPi * 1.5);
}

TEST(GeometryAzimuth, ThreeDimensionalVerticalTargetReturnsZero)
{
    const point3d<float> observer(5.0F, 6.0F, 0.0F);
    EXPECT_EQ(castle::math::azimuth(observer, point3d<float>(5.0F, 6.0F, 20.0F)), 0.0F);
    EXPECT_EQ(castle::math::azimuth_degrees(observer, point3d<float>(5.0F, 6.0F, -20.0F)), 0.0F);
    EXPECT_EQ(castle::math::azimuth(observer, observer), 0.0F);
}

TEST(GeometryAzimuth, RecoversSatelliteAzimuthFromLocalEnu)
{
    // Satellite at azimuth 120 deg, elevation 35 deg, slant range 20200 km.
    const double azimuth_rad = 120.0 * kPi / 180.0;
    const double elevation_rad = 35.0 * kPi / 180.0;
    const double range_m = 20200000.0;
    const point3d<double> receiver;
    const point3d<double> satellite(range_m * cos(elevation_rad) * sin(azimuth_rad),
                                    range_m * cos(elevation_rad) * cos(azimuth_rad),
                                    range_m * sin(elevation_rad));

    EXPECT_NEAR(castle::math::azimuth_degrees(receiver, satellite), 120.0, 1.0e-9);
    EXPECT_NEAR(castle::math::azimuth(receiver, satellite), azimuth_rad, 1.0e-12);
}

} // namespace
