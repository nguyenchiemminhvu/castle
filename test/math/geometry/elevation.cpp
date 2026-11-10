
#include <gtest/gtest.h>

#include "castle/math/geometry/elevation.hpp"

#include <math.h>

namespace
{

using castle::math::point3d;

constexpr double kPi = 3.14159265358979323846;

TEST(GeometryElevation, HorizonZenithAndNadir)
{
    const point3d<double> origin;

    EXPECT_EQ(castle::math::elevation(origin, point3d<double>(1.0, 0.0, 0.0)), 0.0);
    EXPECT_EQ(castle::math::elevation_degrees(origin, point3d<double>(0.0, -7.0, 0.0)), 0.0);

    EXPECT_DOUBLE_EQ(castle::math::elevation(origin, point3d<double>(0.0, 0.0, 10.0)), kPi * 0.5);
    EXPECT_DOUBLE_EQ(castle::math::elevation_degrees(origin, point3d<double>(0.0, 0.0, 10.0)), 90.0);

    EXPECT_DOUBLE_EQ(castle::math::elevation(origin, point3d<double>(0.0, 0.0, -10.0)), -kPi * 0.5);
    EXPECT_DOUBLE_EQ(castle::math::elevation_degrees(origin, point3d<double>(0.0, 0.0, -10.0)), -90.0);
}

TEST(GeometryElevation, UsesFullHorizontalDistance)
{
    const point3d<double> origin;

    // hypot(3, 4) = 5, so a rise of 5 is 45 degrees regardless of heading.
    EXPECT_DOUBLE_EQ(castle::math::elevation(origin, point3d<double>(3.0, 4.0, 5.0)), kPi * 0.25);
    EXPECT_DOUBLE_EQ(castle::math::elevation_degrees(origin, point3d<double>(-3.0, 4.0, 5.0)), 45.0);
    EXPECT_DOUBLE_EQ(castle::math::elevation_degrees(origin, point3d<double>(4.0, -3.0, -5.0)), -45.0);

    EXPECT_NEAR(castle::math::elevation_degrees(origin, point3d<double>(sqrt(3.0), 0.0, 1.0)), 30.0, 1.0e-12);
    EXPECT_NEAR(castle::math::elevation(origin, point3d<double>(0.0, 1.0, sqrt(3.0))), kPi / 3.0, 1.0e-12);
}

TEST(GeometryElevation, DependsOnlyOnDisplacementAndIsAntisymmetric)
{
    const point3d<double> from(100.0, 200.0, 50.0);
    const point3d<double> to(103.0, 204.0, 55.0);

    EXPECT_DOUBLE_EQ(castle::math::elevation_degrees(from, to), 45.0);
    EXPECT_DOUBLE_EQ(castle::math::elevation_degrees(to, from), -45.0);
    EXPECT_DOUBLE_EQ(castle::math::elevation(to, from), -castle::math::elevation(from, to));
}

TEST(GeometryElevation, CoincidentPointsReturnZero)
{
    const point3d<double> p(1.5, -2.5, 3.5);
    EXPECT_EQ(castle::math::elevation(p, p), 0.0);
    EXPECT_EQ(castle::math::elevation_degrees(p, p), 0.0);

    const point3d<float> q(-4.0F, 0.0F, 9.0F);
    EXPECT_EQ(castle::math::elevation(q, q), 0.0F);
}

TEST(GeometryElevation, FullSweepStaysInRangeAndMatchesAngle)
{
    const point3d<double> from(12.0, -3.0, 1.0);
    for (int angle = -90; angle <= 90; ++angle)
    {
        const double expected_radians = static_cast<double>(angle) * kPi / 180.0;
        const point3d<double> to(from.x() + 6.0 * cos(expected_radians) * 0.6,
                                 from.y() + 6.0 * cos(expected_radians) * 0.8,
                                 from.z() + 6.0 * sin(expected_radians));
        const double radians = castle::math::elevation(from, to);
        const double degrees = castle::math::elevation_degrees(from, to);

        EXPECT_GE(radians, -kPi * 0.5);
        EXPECT_LE(radians, kPi * 0.5);
        EXPECT_GE(degrees, -90.0);
        EXPECT_LE(degrees, 90.0);
        EXPECT_NEAR(radians, expected_radians, 1.0e-9) << "angle " << angle;
        EXPECT_NEAR(degrees, static_cast<double>(angle), 1.0e-9) << "angle " << angle;
    }
}

TEST(GeometryElevation, FloatAndLongDoubleOverloads)
{
    EXPECT_NEAR(castle::math::elevation(point3d<float>(0.0F, 0.0F, 0.0F), point3d<float>(1.0F, 0.0F, 1.0F)),
                static_cast<float>(kPi * 0.25), 1.0e-6F);
    EXPECT_NEAR(castle::math::elevation_degrees(point3d<float>(1.0F, 1.0F, 1.0F), point3d<float>(1.0F, 1.0F, 4.0F)),
                90.0F, 1.0e-4F);

    EXPECT_NEAR(static_cast<double>(castle::math::elevation(point3d<long double>(0.0L, 0.0L, 0.0L),
                                                            point3d<long double>(0.0L, 2.0L, -2.0L))),
                -kPi * 0.25, 1.0e-12);
    EXPECT_NEAR(static_cast<double>(castle::math::elevation_degrees(point3d<long double>(0.0L, 0.0L, 0.0L),
                                                                    point3d<long double>(1.0L, 0.0L, 0.0L))),
                0.0, 1.0e-12);
}

TEST(GeometryElevation, RecoversSatelliteElevationFromLocalEnu)
{
    // Satellite at azimuth 120 deg, elevation 35 deg, slant range 20200 km.
    const double azimuth_rad = 120.0 * kPi / 180.0;
    const double elevation_rad = 35.0 * kPi / 180.0;
    const double range_m = 20200000.0;
    const point3d<double> receiver;
    const point3d<double> satellite(range_m * cos(elevation_rad) * sin(azimuth_rad),
                                    range_m * cos(elevation_rad) * cos(azimuth_rad),
                                    range_m * sin(elevation_rad));

    EXPECT_NEAR(castle::math::elevation_degrees(receiver, satellite), 35.0, 1.0e-9);
    EXPECT_NEAR(castle::math::elevation(receiver, satellite), elevation_rad, 1.0e-12);
}

} // namespace
