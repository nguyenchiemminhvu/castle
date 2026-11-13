#include <gtest/gtest.h>

#include "castle/units/distance.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsDistance, UnitDefinitions)
{
    static_assert(is_same<cu::meter_unit::dimension, cu::dim::length>::value, "dimension");
    static_assert(cu::kilometer_unit::ratio::num == 1000, "km");
    static_assert(cu::centimeter_unit::ratio::den == 100, "cm");
    static_assert(cu::millimeter_unit::ratio::den == 1000, "mm");
    static_assert(cu::micrometer_unit::ratio::den == 1000000, "um");
    static_assert(cu::nanometer_unit::ratio::den == 1000000000, "nm");
    static_assert(cu::nautical_mile_unit::ratio::num == 1852, "nmi");
    EXPECT_EQ(cu::mile_unit::ratio::num * 1000, 1609344 * cu::mile_unit::ratio::den);
}

TEST(UnitsDistance, ExactConversions)
{
    EXPECT_EQ(cu::meters<int>(cu::kilometers<int>(3)).value(), 3000);
    EXPECT_EQ(cu::millimeters<int>(cu::centimeters<int>(4)).value(), 40);
    EXPECT_EQ(cu::micrometers<int64_t>(cu::millimeters<int64_t>(2)).value(), 2000);
    EXPECT_EQ(cu::nanometers<int64_t>(cu::micrometers<int64_t>(2)).value(), 2000);
    EXPECT_EQ(cu::inches<int>(cu::feet<int>(2)).value(), 24);
    EXPECT_EQ(cu::feet<int>(cu::yards<int>(2)).value(), 6);
    EXPECT_EQ(cu::feet<int>(cu::miles<int>(1)).value(), 5280);
    EXPECT_EQ(cu::meters<int>(cu::nautical_miles<int>(2)).value(), 3704);

    const cu::meters<double> foot = cu::feet<double>(1.0);
    EXPECT_DOUBLE_EQ(foot.value(), 0.3048);
    const cu::meters<double> inch = cu::inches<double>(1.0);
    EXPECT_DOUBLE_EQ(inch.value(), 0.0254);
    const cu::kilometers<double> mile = cu::miles<double>(1.0);
    EXPECT_DOUBLE_EQ(mile.value(), 1.609344);
    const cu::kilometers<int> truncated(cu::meters<int>(1999));
    EXPECT_EQ(truncated.value(), 1);
}

TEST(UnitsDistance, Literals)
{
    static_assert(is_same<decltype(1.5_m), cu::meters<double>>::value, "m");
    static_assert(is_same<decltype(15_m), cu::meters<int64_t>>::value, "m int");
    EXPECT_DOUBLE_EQ((1.5_m).value(), 1.5);
    EXPECT_EQ((15_m).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_km).value(), 1.5);
    EXPECT_EQ((15_km).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_cm).value(), 1.5);
    EXPECT_EQ((15_cm).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mm).value(), 1.5);
    EXPECT_EQ((15_mm).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_um).value(), 1.5);
    EXPECT_EQ((15_um).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_nm).value(), 1.5);
    EXPECT_EQ((15_nm).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_in).value(), 1.5);
    EXPECT_EQ((15_in).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_ft).value(), 1.5);
    EXPECT_EQ((15_ft).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_yd).value(), 1.5);
    EXPECT_EQ((15_yd).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mi).value(), 1.5);
    EXPECT_EQ((15_mi).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_nmi).value(), 1.5);
    EXPECT_EQ((15_nmi).value(), 15);
}

TEST(UnitsDistance, MixedArithmetic)
{
    const cu::meters<double> total = 1.2_km + 350.0_m;
    EXPECT_DOUBLE_EQ(total.value(), 1550.0);
    EXPECT_TRUE(1_km == 1000_m);
    EXPECT_TRUE(12_in == 1_ft);
    EXPECT_TRUE(1_mi > 1_km);
    const auto area = 2.0_m * 3.0_m;
    static_assert(decltype(area)::dimension::length == 2, "area");
    EXPECT_DOUBLE_EQ(area.value(), 6.0);
}

} // namespace
