#include <gtest/gtest.h>

#include "castle/units/angle.hpp"
#include "castle/units/angular_velocity.hpp"
#include "castle/units/frequency.hpp"
#include "castle/units/time.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
namespace cm = castle::math;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsAngularVelocity, UnitDefinitions)
{
    static_assert(is_same<cu::radian_per_second_unit::dimension, cu::dim::angular_velocity>::value, "dimension");
    static_assert(cu::dim::angular_velocity::time == -1 && cu::dim::angular_velocity::angle == 1, "angle / T");
    static_assert(!is_same<cu::dim::angular_velocity, cu::dim::frequency>::value, "distinct from frequency");
    static_assert(cu::radian_per_second_unit::ratio::num == 1 && cu::radian_per_second_unit::pi_exponent == 0,
                  "rad/s");
    static_assert(cu::milliradian_per_second_unit::ratio::den == 1000, "mrad/s");
    static_assert(cu::degree_per_second_unit::ratio::den == 180 && cu::degree_per_second_unit::pi_exponent == 1,
                  "deg/s");
    static_assert(cu::revolution_per_second_unit::ratio::num == 2 &&
                      cu::revolution_per_second_unit::pi_exponent == 1,
                  "rev/s");
    static_assert(cu::revolution_per_minute_unit::ratio::den == 30 &&
                      cu::revolution_per_minute_unit::pi_exponent == 1,
                  "rpm");
    EXPECT_EQ(cu::degree_per_second_unit::ratio::den, 180);
    EXPECT_EQ(cu::revolution_per_minute_unit::ratio::den, 30);
}

TEST(UnitsAngularVelocity, ExactConversions)
{
    // rpm <-> deg/s share the pi factor, so integral conversions are exact.
    EXPECT_EQ(cu::degrees_per_second<int>(cu::revolutions_per_minute<int>(60)).value(), 360);
    EXPECT_EQ(cu::degrees_per_second<int>(cu::revolutions_per_second<int>(2)).value(), 720);
    EXPECT_EQ(cu::revolutions_per_minute<int>(cu::revolutions_per_second<int>(1)).value(), 60);
    EXPECT_EQ(cu::revolutions_per_minute<int>(cu::degrees_per_second<int>(360)).value(), 60);
    EXPECT_EQ(cu::milliradians_per_second<int>(cu::radians_per_second<int>(3)).value(), 3000);
    EXPECT_EQ(cu::radians_per_second<int>(cu::milliradians_per_second<int>(2500)).value(), 2);

    // Coarser integral units truncate and need an explicit conversion.
    EXPECT_EQ(cu::revolutions_per_second<int>(cu::revolutions_per_minute<int>(90)).value(), 1);
    EXPECT_EQ(cu::revolutions_per_minute<int>(cu::degrees_per_second<int>(10)).value(), 1);
}

TEST(UnitsAngularVelocity, PiConversions)
{
    // 60 rpm = 1 rev/s = 2 pi rad/s.
    const cu::radians_per_second<double> from_rpm = cu::revolutions_per_minute<double>(60.0);
    EXPECT_NEAR(from_rpm.value(), 2.0 * cm::pi<double>(), 1e-12);
    const cu::radians_per_second<double> from_degps = cu::degrees_per_second<double>(180.0);
    EXPECT_NEAR(from_degps.value(), cm::pi<double>(), 1e-12);
    const cu::radians_per_second<double> from_rps = cu::revolutions_per_second<double>(1.0);
    EXPECT_NEAR(from_rps.value(), 2.0 * cm::pi<double>(), 1e-12);

    const cu::degrees_per_second<double> to_degps = cu::radians_per_second<double>(cm::pi<double>());
    EXPECT_NEAR(to_degps.value(), 180.0, 1e-9);
    const cu::revolutions_per_minute<double> to_rpm = cu::radians_per_second<double>(cm::pi<double>());
    EXPECT_NEAR(to_rpm.value(), 30.0, 1e-9);
    const cu::milliradians_per_second<double> to_mrad = cu::degrees_per_second<double>(180.0);
    EXPECT_NEAR(to_mrad.value(), 1000.0 * cm::pi<double>(), 1e-9);

    // Integral rad/s needs an explicit cast, evaluated in floating point.
    const auto rad_int = cu::quantity_cast<cu::radians_per_second<int>>(cu::revolutions_per_minute<int>(60));
    EXPECT_EQ(rad_int.value(), 6);
}

TEST(UnitsAngularVelocity, Literals)
{
    static_assert(is_same<decltype(1.5_radps), cu::radians_per_second<double>>::value, "radps");
    static_assert(is_same<decltype(15_radps), cu::radians_per_second<int64_t>>::value, "radps int");
    static_assert(is_same<decltype(1.5_mradps), cu::milliradians_per_second<double>>::value, "mradps");
    static_assert(is_same<decltype(15_mradps), cu::milliradians_per_second<int64_t>>::value, "mradps int");
    static_assert(is_same<decltype(1.5_degps), cu::degrees_per_second<double>>::value, "degps");
    static_assert(is_same<decltype(15_degps), cu::degrees_per_second<int64_t>>::value, "degps int");
    static_assert(is_same<decltype(1.5_rps), cu::revolutions_per_second<double>>::value, "rps");
    static_assert(is_same<decltype(15_rps), cu::revolutions_per_second<int64_t>>::value, "rps int");
    static_assert(is_same<decltype(1.5_rpm), cu::revolutions_per_minute<double>>::value, "rpm");
    static_assert(is_same<decltype(15_rpm), cu::revolutions_per_minute<int64_t>>::value, "rpm int");
    EXPECT_DOUBLE_EQ((1.5_radps).value(), 1.5);
    EXPECT_EQ((15_radps).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mradps).value(), 1.5);
    EXPECT_EQ((15_mradps).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_degps).value(), 1.5);
    EXPECT_EQ((15_degps).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_rps).value(), 1.5);
    EXPECT_EQ((15_rps).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_rpm).value(), 1.5);
    EXPECT_EQ((15_rpm).value(), 15);
}

TEST(UnitsAngularVelocity, ComparisonAndArithmetic)
{
    EXPECT_TRUE(1_rps == 60_rpm);
    EXPECT_TRUE(360_degps == 1_rps);
    EXPECT_TRUE(6_degps == 1_rpm);
    EXPECT_TRUE(1_rps > 59_rpm);
    EXPECT_TRUE(500_mradps < 1_radps);
    const cu::degrees_per_second<int64_t> sum = 1_rpm + 6_degps;
    EXPECT_EQ(sum.value(), 12);
    const cu::revolutions_per_minute<int64_t> diff = 2_rps - 60_rpm;
    EXPECT_EQ(diff.value(), 60);
}

TEST(UnitsAngularVelocity, AngleOverDuration)
{
    const auto rate = cu::degrees<double>(90.0) / cu::seconds<double>(2.0);
    static_assert(is_same<decltype(rate), const cu::degrees_per_second<double>>::value, "deg / s");
    EXPECT_DOUBLE_EQ(rate.value(), 45.0);

    const auto turns = cu::revolutions<double>(2.0) / cu::seconds<double>(4.0);
    static_assert(is_same<decltype(turns), const cu::revolutions_per_second<double>>::value, "rev / s");
    EXPECT_DOUBLE_EQ(turns.value(), 0.5);

    const auto spin = cu::revolutions<double>(120.0) / cu::minutes<double>(1.0);
    static_assert(is_same<decltype(spin), const cu::revolutions_per_minute<double>>::value, "rev / min");
    EXPECT_DOUBLE_EQ(spin.value(), 120.0);

    const auto sweep = cu::radians<double>(cm::pi<double>()) / cu::seconds<double>(1.0);
    static_assert(is_same<decltype(sweep), const cu::radians_per_second<double>>::value, "rad / s");
    EXPECT_NEAR(cu::degrees_per_second<double>(sweep).value(), 180.0, 1e-9);
}

TEST(UnitsAngularVelocity, VelocityTimesDuration)
{
    const auto turned = cu::degrees_per_second<double>(45.0) * cu::seconds<double>(2.0);
    static_assert(is_same<decltype(turned), const cu::degrees<double>>::value, "deg/s * s");
    EXPECT_DOUBLE_EQ(turned.value(), 90.0);

    const auto swept = 30.0_rpm * cu::seconds<double>(4.0);
    static_assert(is_same<decltype(swept), const cu::quantity<cu::unit<cu::dim::angle, cm::ratio<1, 30>, 1>, double>>::value,
                  "rpm * s");
    const cu::revolutions<double> swept_revs = swept;
    EXPECT_DOUBLE_EQ(swept_revs.value(), 2.0);

    const cu::revolutions<double> in_minute = 120.0_rpm * cu::minutes<double>(1.0);
    EXPECT_DOUBLE_EQ(in_minute.value(), 120.0);
}

TEST(UnitsAngularVelocity, NotAFrequency)
{
    static_assert(!castle::meta::is_constructible<cu::hertz<double>, cu::revolutions_per_minute<double>>::value,
                  "rpm must not convert to Hz");
    static_assert(!castle::meta::is_constructible<cu::revolutions_per_minute<double>, cu::hertz<double>>::value,
                  "Hz must not convert to rpm");
    static_assert(!castle::meta::is_constructible<cu::hertz<int>, cu::radians_per_second<int>>::value,
                  "rad/s must not convert to Hz");
    static_assert(castle::meta::is_constructible<cu::degrees_per_second<double>,
                                                 cu::revolutions_per_minute<double>>::value,
                  "rpm converts to deg/s");
    EXPECT_FALSE((castle::meta::is_constructible<cu::hertz<double>, cu::revolutions_per_minute<double>>::value));
    EXPECT_TRUE((castle::meta::is_constructible<cu::degrees_per_second<double>,
                                                cu::revolutions_per_minute<double>>::value));
}

} // namespace
