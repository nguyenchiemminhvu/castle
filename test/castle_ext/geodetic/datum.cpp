#include <gtest/gtest.h>

#include "castle_ext/geodetic/datum.hpp"

namespace
{

TEST(GeodeticDatum, Wgs84Ellipsoid)
{
    using ellipsoid = castle_ext::geodetic::wgs84_ellipsoid<double>;

    static_assert(ellipsoid::semi_major_axis() == 6378137.0, "wgs84 semi-major axis");

    EXPECT_DOUBLE_EQ(ellipsoid::semi_major_axis(), 6378137.0);
    EXPECT_NEAR(ellipsoid::flattening(), 1.0 / 298.257223563, 1e-12);
    EXPECT_NEAR(ellipsoid::semi_minor_axis(), 6356752.314245, 1e-4);
    EXPECT_GT(ellipsoid::semi_major_axis(), ellipsoid::semi_minor_axis());
    EXPECT_NEAR(ellipsoid::eccentricity_squared(), 0.00669437999014, 1e-12);
}

TEST(GeodeticDatum, KrasovskyEllipsoid)
{
    using ellipsoid = castle_ext::geodetic::krasovsky_ellipsoid<double>;

    static_assert(ellipsoid::semi_major_axis() == 6378245.0, "krasovsky semi-major axis");

    EXPECT_DOUBLE_EQ(ellipsoid::semi_major_axis(), 6378245.0);
    EXPECT_NEAR(ellipsoid::eccentricity_squared(), 0.00669342162296594323, 1e-17);
}

TEST(GeodeticDatum, Pz90Ellipsoid)
{
    using ellipsoid = castle_ext::geodetic::pz90_ellipsoid<double>;

    // Reference values (PZ-90.11 / GLONASS): a = 6378136.0 m, 1/f = 298.257839303.
    CASTLE_CONST double reference_semi_major_axis = 6378136.0;
    CASTLE_CONST double reference_inverse_flattening = 298.257839303;
    CASTLE_CONST double maja_tolerance = 1e-3;   // metres
    CASTLE_CONST double flat_tolerance = 1e-7;   // dimensionless (1/f)

    static_assert(ellipsoid::semi_major_axis() == 6378136.0, "pz90 semi-major axis");

    EXPECT_NEAR(ellipsoid::semi_major_axis(), reference_semi_major_axis, maja_tolerance);
    EXPECT_NEAR(static_cast<double>(1) / ellipsoid::flattening(), reference_inverse_flattening, flat_tolerance);
    EXPECT_NEAR(ellipsoid::semi_minor_axis(), 6356751.361745, 1e-4);
    EXPECT_GT(ellipsoid::semi_major_axis(), ellipsoid::semi_minor_axis());

    // PZ-90 and WGS-84 ellipsoids are very close but distinct.
    EXPECT_NE(ellipsoid::semi_major_axis(), castle_ext::geodetic::wgs84_ellipsoid<double>::semi_major_axis());
}

TEST(GeodeticDatum, Gcj02Region)
{
    using region = castle_ext::geodetic::gcj02_region<double>;

    static_assert(region::min_longitude() < region::max_longitude(), "region longitude ordering");
    static_assert(region::min_latitude() < region::max_latitude(), "region latitude ordering");

    // Beijing lies inside the obfuscated region.
    EXPECT_GE(116.291, region::min_longitude());
    EXPECT_LE(116.291, region::max_longitude());
    EXPECT_GE(39.8946, region::min_latitude());
    EXPECT_LE(39.8946, region::max_latitude());
}

} // namespace
