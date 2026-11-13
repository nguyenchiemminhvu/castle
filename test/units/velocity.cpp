#include <gtest/gtest.h>

#include "castle/units/distance.hpp"
#include "castle/units/time.hpp"
#include "castle/units/velocity.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsVelocity, UnitDefinitions)
{
    static_assert(is_same<cu::meter_per_second_unit::dimension, cu::dim::velocity>::value, "dimension");
    static_assert(is_same<cu::dim::velocity, cu::dimension<1, 0, -1, 0, 0, 0, 0, 0>>::value, "L/T");
    static_assert(cu::kilometer_per_hour_unit::ratio::num == 5 && cu::kilometer_per_hour_unit::ratio::den == 18,
                  "km/h");
    static_assert(cu::mile_per_hour_unit::ratio::num == 1397 && cu::mile_per_hour_unit::ratio::den == 3125,
                  "mph reduced");
    static_assert(cu::knot_unit::ratio::num == 463 && cu::knot_unit::ratio::den == 900, "kn reduced");
    static_assert(cu::foot_per_second_unit::ratio::num == 381 && cu::foot_per_second_unit::ratio::den == 1250,
                  "ft/s reduced");
    static_assert(cu::centimeter_per_second_unit::ratio::den == 100, "cm/s");
    static_assert(cu::millimeter_per_second_unit::ratio::den == 1000, "mm/s");
    static_assert(is_same<cu::quantity<cu::meter_per_second_unit, int>, cu::meters_per_second<int>>::value, "alias");
}

TEST(UnitsVelocity, ExactConversions)
{
    EXPECT_EQ(cu::millimeters_per_second<int>(cu::meters_per_second<int>(3)).value(), 3000);
    EXPECT_EQ(cu::millimeters_per_second<int>(cu::centimeters_per_second<int>(4)).value(), 40);
    EXPECT_EQ(cu::centimeters_per_second<int>(cu::meters_per_second<int>(2)).value(), 200);

    const cu::meters_per_second<double> kmph = cu::kilometers_per_hour<double>(36.0);
    EXPECT_DOUBLE_EQ(kmph.value(), 10.0);
    const cu::meters_per_second<double> mph = cu::miles_per_hour<double>(1.0);
    EXPECT_DOUBLE_EQ(mph.value(), 0.44704);
    const cu::meters_per_second<double> kn = cu::knots<double>(3600.0);
    EXPECT_DOUBLE_EQ(kn.value(), 1852.0);
    const cu::meters_per_second<double> fps = cu::feet_per_second<double>(1.0);
    EXPECT_DOUBLE_EQ(fps.value(), 0.3048);
    const cu::kilometers_per_hour<double> knot_kmph = cu::knots<double>(1.0);
    EXPECT_DOUBLE_EQ(knot_kmph.value(), 1.852);
    const cu::kilometers_per_hour<double> from_mps = cu::meters_per_second<double>(25.0);
    EXPECT_DOUBLE_EQ(from_mps.value(), 90.0);

    // 1 mph is exactly 0.44704 m/s: exact integral conversion at a fine scale.
    using fine_velocity_unit = cu::scaled_unit_t<cu::meter_per_second_unit, castle::math::ratio<1, 10000>>;
    const cu::quantity<fine_velocity_unit, int64_t> fine(cu::miles_per_hour<int64_t>(10000));
    EXPECT_EQ(fine.value(), 44704000);
    EXPECT_EQ(cu::kilometers_per_hour<int>(cu::meters_per_second<int>(5)).value(), 18);
    const cu::meters_per_second<int> truncated(cu::kilometers_per_hour<int>(10));
    EXPECT_EQ(truncated.value(), 2);
}

TEST(UnitsVelocity, Literals)
{
    static_assert(is_same<decltype(1.5_mps), cu::meters_per_second<double>>::value, "mps");
    static_assert(is_same<decltype(15_mps), cu::meters_per_second<int64_t>>::value, "mps int");
    static_assert(is_same<decltype(15_kmph), cu::kilometers_per_hour<int64_t>>::value, "kmph int");
    static_assert(is_same<decltype(15_mph), cu::miles_per_hour<int64_t>>::value, "mph int");
    static_assert(is_same<decltype(15_kn), cu::knots<int64_t>>::value, "kn int");
    EXPECT_DOUBLE_EQ((1.5_mps).value(), 1.5);
    EXPECT_EQ((15_mps).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_kmph).value(), 1.5);
    EXPECT_EQ((15_kmph).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mph).value(), 1.5);
    EXPECT_EQ((15_mph).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_kn).value(), 1.5);
    EXPECT_EQ((15_kn).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_cmps).value(), 1.5);
    EXPECT_EQ((15_cmps).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mmps).value(), 1.5);
    EXPECT_EQ((15_mmps).value(), 15);
    EXPECT_TRUE(36_kmph == 10_mps);
    EXPECT_TRUE(100_cmps == 1_mps);
    EXPECT_TRUE(1000_mmps == 1_mps);
    EXPECT_TRUE(10.0_kn > 5.0_mps);
}

TEST(UnitsVelocity, DerivedArithmetic)
{
    // distance / duration -> meters_per_second
    const auto speed = 100.0_m / 4.0_s;
    static_assert(is_same<decltype(speed), const cu::meters_per_second<double>>::value, "m / s");
    EXPECT_DOUBLE_EQ(speed.value(), 25.0);
    const auto km_speed = 90.0_km / cu::hours<double>(1.0);
    const cu::meters_per_second<double> km_speed_si = km_speed;
    EXPECT_DOUBLE_EQ(km_speed_si.value(), 25.0);
    const auto int_speed = 10_m / castle::chrono::duration<int64_t, castle::math::ratio<1>>(2);
    static_assert(is_same<decltype(int_speed), const cu::meters_per_second<int64_t>>::value, "int m / s");
    EXPECT_EQ(int_speed.value(), 5);

    // velocity * duration -> meters
    const auto travelled = 25.0_mps * 4.0_s;
    static_assert(is_same<decltype(travelled), const cu::meters<double>>::value, "v * t");
    EXPECT_DOUBLE_EQ(travelled.value(), 100.0);
    // Mixed scales keep the product in a scaled length unit; convert to compare.
    const cu::meters<double> cruise = 36.0_kmph * 10.0_s;
    EXPECT_DOUBLE_EQ(cruise.value(), 100.0);
    const auto leg = 3.0_s * 2.0_mps;
    static_assert(is_same<decltype(leg), const cu::meters<double>>::value, "t * v");
    EXPECT_DOUBLE_EQ(leg.value(), 6.0);

    // length / velocity -> duration
    const auto eta = 100.0_m / 25.0_mps;
    EXPECT_DOUBLE_EQ(eta.count(), 4.0);
}

} // namespace
