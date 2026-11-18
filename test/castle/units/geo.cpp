#include <gtest/gtest.h>

#include "castle/units/angle.hpp"
#include "castle/units/geo.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsGeo, UnitDefinitions)
{
    static_assert(is_same<cu::geo_radian_unit::dimension, cu::dim::angle>::value, "dimension");
    static_assert(is_same<cu::geo_radian_unit::kind, cu::geo_kind>::value, "radian kind");
    static_assert(is_same<cu::geo_degree_unit::kind, cu::geo_kind>::value, "degree kind");
    static_assert(is_same<cu::geo_degree_e7_unit::kind, cu::geo_kind>::value, "e7 kind");
    static_assert(is_same<cu::geo_semicircle_unit::kind, cu::geo_kind>::value, "semicircle kind");
    static_assert(cu::geo_radian_unit::pi_exponent == 0, "radian pi");
    static_assert(cu::geo_degree_unit::pi_exponent == 1, "degree pi");
    static_assert(cu::geo_degree_e7_unit::pi_exponent == 1, "e7 pi");
    static_assert(cu::geo_semicircle_unit::pi_exponent == 1, "semicircle pi");
    static_assert(cu::geo_degree_unit::ratio::num == 1 && cu::geo_degree_unit::ratio::den == 180, "degree");
    static_assert(cu::geo_degree_e7_unit::ratio::den == 1800000000, "e7");
    static_assert(cu::geo_semicircle_unit::ratio::den == 2147483648LL, "semicircle");
    static_assert(is_same<cu::geo_arcsecond_unit::kind, cu::geo_kind>::value, "arcsecond kind");
    static_assert(is_same<cu::geo_milliarcsecond_unit::kind, cu::geo_kind>::value, "milliarcsecond kind");
}

TEST(UnitsGeo, KindSeparatesFromPlaneAngles)
{
    static_assert(!castle::meta::is_convertible<cu::degrees<double>, cu::geo_degrees<double>>::value,
                  "plane -> geo is not implicit");
    static_assert(!castle::meta::is_convertible<cu::geo_degrees<double>, cu::degrees<double>>::value,
                  "geo -> plane is not implicit");
    static_assert(!castle::meta::is_constructible<cu::geo_degrees<double>, cu::degrees<double>>::value,
                  "plane -> geo is not explicit");
    static_assert(!castle::meta::is_constructible<cu::degrees<double>, cu::geo_degrees<double>>::value,
                  "geo -> plane is not explicit");
    static_assert(castle::meta::is_convertible<cu::geo_degrees_e7<int32_t>, cu::geo_degrees<double>>::value,
                  "e7 -> geo degrees");

    const cu::degrees<double> plane = cu::quantity_cast<cu::degrees<double>>(cu::geo_degrees<double>(90.0));
    EXPECT_DOUBLE_EQ(plane.value(), 90.0);
    const cu::geo_degrees<double> geo = cu::quantity_cast<cu::geo_degrees<double>>(cu::degrees<double>(45.0));
    EXPECT_DOUBLE_EQ(geo.value(), 45.0);
    const cu::degrees<double> from_rad = cu::quantity_cast<cu::degrees<double>>(cu::geo_radians<double>(1.0));
    EXPECT_NEAR(from_rad.value(), 57.29577951308232, 1e-12);
}

TEST(UnitsGeo, ExactConversions)
{
    // u-blox UBX int32 latitude/longitude, 1e-7 degree.
    const cu::geo_degrees<double> lat = cu::geo_degrees_e7<int32_t>(375665000);
    EXPECT_DOUBLE_EQ(lat.value(), 37.5665);
    const cu::geo_degrees<double> lon = cu::geo_degrees_e7<int32_t>(-1220000000);
    EXPECT_DOUBLE_EQ(lon.value(), -122.0);
    const cu::geo_degrees<double> edge = cu::geo_degrees_e7<int32_t>(2147483647);
    EXPECT_DOUBLE_EQ(edge.value(), 214.7483647);
    EXPECT_DOUBLE_EQ(cu::geo_degrees<double>(cu::geo_degrees_e7<int32_t>(-2147483647 - 1)).value(), -214.7483648);

    // Garmin FIT semicircles, 180 deg / 2^31.
    const cu::geo_degrees<double> quarter = cu::geo_semicircles<int32_t>(1073741824);
    EXPECT_DOUBLE_EQ(quarter.value(), 90.0);
    const cu::geo_degrees<double> half = cu::geo_semicircles<int32_t>(-2147483647 - 1);
    EXPECT_DOUBLE_EQ(half.value(), -180.0);
    const cu::geo_semicircles<int64_t> semi(cu::geo_degrees<int64_t>(90));
    EXPECT_EQ(semi.value(), 1073741824);

    EXPECT_EQ(cu::geo_degrees_e7<int64_t>(cu::geo_degrees<int64_t>(2)).value(), 20000000);
    const cu::geo_radians<double> rad = cu::geo_degrees<double>(180.0);
    EXPECT_NEAR(rad.value(), 3.141592653589793, 1e-12);
    const cu::geo_degrees<double> deg = cu::geo_radians<double>(3.141592653589793);
    EXPECT_NEAR(deg.value(), 180.0, 1e-9);
    EXPECT_TRUE(cu::geo_degrees<int64_t>(90) == cu::geo_semicircles<int64_t>(1073741824));

    // Arc-seconds (1/3600 deg) and milliarc-seconds (1/3600000 deg).
    EXPECT_EQ(cu::geo_arcseconds<int64_t>(cu::geo_degrees<int64_t>(1)).value(), 3600);
    EXPECT_EQ(cu::geo_milliarcseconds<int64_t>(cu::geo_degrees<int64_t>(-180)).value(), -648000000);
    EXPECT_EQ(cu::geo_milliarcseconds<int64_t>(cu::geo_arcseconds<int64_t>(5)).value(), 5000);
    const cu::geo_degrees<double> mas_lat = cu::geo_milliarcseconds<int32_t>(135239400);
    EXPECT_DOUBLE_EQ(mas_lat.value(), 37.5665);
    const cu::geo_degrees<double> as_lon = cu::geo_arcseconds<int32_t>(-439200);
    EXPECT_DOUBLE_EQ(as_lon.value(), -122.0);
    EXPECT_TRUE(cu::geo_degrees<int64_t>(90) == cu::geo_milliarcseconds<int64_t>(324000000));
    EXPECT_FALSE(cu::is_valid_latitude(cu::geo_milliarcseconds<int32_t>(324000001)));
    EXPECT_TRUE(cu::is_valid_longitude(cu::geo_arcseconds<int32_t>(-648000)));
    // Plane arc-seconds still need an explicit cast.
    EXPECT_EQ(cu::quantity_cast<cu::geo_arcseconds<int64_t>>(cu::arcseconds<int64_t>(42)).value(), 42);
}

TEST(UnitsGeo, Literals)
{
    static_assert(is_same<decltype(1.5_geo_deg), cu::geo_degrees<double>>::value, "geo_deg");
    static_assert(is_same<decltype(15_geo_deg), cu::geo_degrees<int64_t>>::value, "geo_deg int");
    static_assert(is_same<decltype(1.5_geo_rad), cu::geo_radians<double>>::value, "geo_rad");
    static_assert(is_same<decltype(15_geo_rad), cu::geo_radians<int64_t>>::value, "geo_rad int");
    static_assert(is_same<decltype(1.5_geo_arcsec), cu::geo_arcseconds<double>>::value, "geo_arcsec");
    static_assert(is_same<decltype(15_geo_arcsec), cu::geo_arcseconds<int64_t>>::value, "geo_arcsec int");
    static_assert(is_same<decltype(1.5_geo_mas), cu::geo_milliarcseconds<double>>::value, "geo_mas");
    static_assert(is_same<decltype(15_geo_mas), cu::geo_milliarcseconds<int64_t>>::value, "geo_mas int");
    EXPECT_DOUBLE_EQ((1.5_geo_deg).value(), 1.5);
    EXPECT_EQ((15_geo_deg).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_geo_rad).value(), 1.5);
    EXPECT_EQ((15_geo_rad).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_geo_arcsec).value(), 1.5);
    EXPECT_EQ((15_geo_arcsec).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_geo_mas).value(), 1.5);
    EXPECT_EQ((15_geo_mas).value(), 15);
    EXPECT_TRUE(cu::geo_degrees<int64_t>(1) == 3600_geo_arcsec);
    EXPECT_TRUE(1_geo_arcsec == 1000_geo_mas);
}

TEST(UnitsGeo, LatitudeValidation)
{
    EXPECT_TRUE(cu::is_valid_latitude(0.0_geo_deg));
    EXPECT_TRUE(cu::is_valid_latitude(90.0_geo_deg));
    EXPECT_TRUE(cu::is_valid_latitude(-90.0_geo_deg));
    EXPECT_TRUE(cu::is_valid_latitude(37.5665_geo_deg));
    EXPECT_FALSE(cu::is_valid_latitude(90.0001_geo_deg));
    EXPECT_FALSE(cu::is_valid_latitude(-90.0001_geo_deg));
    EXPECT_TRUE(cu::is_valid_latitude(45_geo_deg));
    EXPECT_FALSE(cu::is_valid_latitude(91_geo_deg));
    EXPECT_TRUE(cu::is_valid_latitude(cu::geo_degrees_e7<int32_t>(900000000)));
    EXPECT_TRUE(cu::is_valid_latitude(cu::geo_degrees_e7<int32_t>(-900000000)));
    EXPECT_FALSE(cu::is_valid_latitude(cu::geo_degrees_e7<int32_t>(900000001)));
    EXPECT_FALSE(cu::is_valid_latitude(cu::geo_degrees_e7<int32_t>(-2147483647 - 1)));
    EXPECT_TRUE(cu::is_valid_latitude(cu::geo_semicircles<int32_t>(1073741824)));
    EXPECT_FALSE(cu::is_valid_latitude(cu::geo_semicircles<int32_t>(1073741825)));
    EXPECT_TRUE(cu::is_valid_latitude(cu::geo_radians<double>(1.5)));
    EXPECT_FALSE(cu::is_valid_latitude(cu::geo_radians<double>(1.6)));
    static_assert(cu::is_valid_latitude(cu::geo_degrees<double>(10.0)), "constexpr");
}

TEST(UnitsGeo, LongitudeValidation)
{
    EXPECT_TRUE(cu::is_valid_longitude(0.0_geo_deg));
    EXPECT_TRUE(cu::is_valid_longitude(180.0_geo_deg));
    EXPECT_TRUE(cu::is_valid_longitude(-180.0_geo_deg));
    EXPECT_TRUE(cu::is_valid_longitude(127.0_geo_deg));
    EXPECT_FALSE(cu::is_valid_longitude(180.0001_geo_deg));
    EXPECT_FALSE(cu::is_valid_longitude(-180.0001_geo_deg));
    EXPECT_TRUE(cu::is_valid_longitude(170_geo_deg));
    EXPECT_FALSE(cu::is_valid_longitude(181_geo_deg));
    EXPECT_TRUE(cu::is_valid_longitude(cu::geo_degrees_e7<int32_t>(1800000000)));
    EXPECT_TRUE(cu::is_valid_longitude(cu::geo_degrees_e7<int32_t>(-1800000000)));
    EXPECT_FALSE(cu::is_valid_longitude(cu::geo_degrees_e7<int32_t>(1800000001)));
    EXPECT_FALSE(cu::is_valid_longitude(cu::geo_degrees_e7<int32_t>(-2147483647 - 1)));
    EXPECT_TRUE(cu::is_valid_longitude(cu::geo_semicircles<int32_t>(-2147483647 - 1)));
    EXPECT_TRUE(cu::is_valid_longitude(cu::geo_semicircles<int32_t>(2147483647)));
    EXPECT_TRUE(cu::is_valid_longitude(cu::geo_radians<double>(3.0)));
    EXPECT_FALSE(cu::is_valid_longitude(cu::geo_radians<double>(3.2)));
    static_assert(cu::is_valid_longitude(cu::geo_degrees<double>(10.0)), "constexpr");
}

TEST(UnitsGeo, AngleHelpersAcceptGeoQuantities)
{
    // Longitude normalisation keeps the geographic kind.
    static_assert(is_same<decltype(cu::wrap_signed(190.0_geo_deg)), cu::geo_degrees<double>>::value, "kind kept");
    EXPECT_DOUBLE_EQ(cu::wrap_signed(190.0_geo_deg).value(), -170.0);
    EXPECT_DOUBLE_EQ(cu::wrap_signed(-190.0_geo_deg).value(), 170.0);
    EXPECT_DOUBLE_EQ(cu::wrap_signed(10.0_geo_deg).value(), 10.0);
    EXPECT_EQ(cu::wrap_signed(cu::geo_degrees<int32_t>(540)).value(), -180);
    EXPECT_TRUE(cu::is_valid_longitude(cu::wrap_signed(190.0_geo_deg)));
    // Semicircles span a full int32 turn: wrapping is the identity on int32.
    EXPECT_EQ(cu::wrap_signed(cu::geo_semicircles<int32_t>(2147483647)).value(), 2147483647);
    EXPECT_EQ(cu::wrap_signed(cu::geo_semicircles<int32_t>(-2147483647 - 1)).value(), -2147483647 - 1);
    EXPECT_EQ(cu::wrap_signed(cu::geo_semicircles<int32_t>(1073741824)).value(), 1073741824);
    EXPECT_NEAR(cu::sin(30.0_geo_deg), 0.5, 1e-12);
    EXPECT_NEAR(cu::cos(60.0_geo_deg), 0.5, 1e-12);
    EXPECT_NEAR(cu::cos(cu::geo_degrees_e7<int32_t>(600000000)), 0.5, 1e-12);
}

} // namespace
