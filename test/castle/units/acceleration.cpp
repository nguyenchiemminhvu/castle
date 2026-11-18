#include <gtest/gtest.h>

#include "castle/units/acceleration.hpp"
#include "castle/units/time.hpp"
#include "castle/units/velocity.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsAcceleration, UnitDefinitions)
{
    static_assert(is_same<cu::meter_per_second_squared_unit::dimension, cu::dim::acceleration>::value, "dimension");
    static_assert(is_same<cu::dim::acceleration, cu::dimension<1, 0, -2, 0, 0, 0, 0, 0>>::value, "L/T^2");
    static_assert(cu::standard_gravity_unit::ratio::num == 196133 && cu::standard_gravity_unit::ratio::den == 20000,
                  "gn reduced");
    static_assert(cu::milli_standard_gravity_unit::ratio::num == 196133 &&
                      cu::milli_standard_gravity_unit::ratio::den == 20000000,
                  "mgn reduced");
    static_assert(cu::foot_per_second_squared_unit::ratio::num == 381 &&
                      cu::foot_per_second_squared_unit::ratio::den == 1250,
                  "ft/s2 reduced");
    static_assert(is_same<cu::quantity<cu::standard_gravity_unit, int>, cu::standard_gravities<int>>::value, "alias");
}

TEST(UnitsAcceleration, ExactConversions)
{
    const cu::meters_per_second_squared<double> g = cu::standard_gravities<double>(1.0);
    EXPECT_DOUBLE_EQ(g.value(), 9.80665);
    const cu::meters_per_second_squared<double> mg = cu::milli_standard_gravities<double>(1000.0);
    EXPECT_DOUBLE_EQ(mg.value(), 9.80665);
    const cu::milli_standard_gravities<double> from_g = cu::standard_gravities<double>(2.0);
    EXPECT_DOUBLE_EQ(from_g.value(), 2000.0);
    const cu::meters_per_second_squared<double> fps2 = cu::feet_per_second_squared<double>(1.0);
    EXPECT_DOUBLE_EQ(fps2.value(), 0.3048);
    const cu::feet_per_second_squared<double> from_mps2 = cu::meters_per_second_squared<double>(0.3048);
    EXPECT_DOUBLE_EQ(from_mps2.value(), 1.0);

    // Integral g to milli-g is exact.
    EXPECT_EQ(cu::milli_standard_gravities<int>(cu::standard_gravities<int>(3)).value(), 3000);
    const cu::standard_gravities<int> truncated(cu::milli_standard_gravities<int>(2999));
    EXPECT_EQ(truncated.value(), 2);
}

TEST(UnitsAcceleration, Literals)
{
    static_assert(is_same<decltype(1.5_mps2), cu::meters_per_second_squared<double>>::value, "mps2");
    static_assert(is_same<decltype(15_mps2), cu::meters_per_second_squared<int64_t>>::value, "mps2 int");
    static_assert(is_same<decltype(1.5_gn), cu::standard_gravities<double>>::value, "gn");
    static_assert(is_same<decltype(15_gn), cu::standard_gravities<int64_t>>::value, "gn int");
    static_assert(is_same<decltype(1.5_mgn), cu::milli_standard_gravities<double>>::value, "mgn");
    static_assert(is_same<decltype(15_mgn), cu::milli_standard_gravities<int64_t>>::value, "mgn int");
    EXPECT_DOUBLE_EQ((1.5_mps2).value(), 1.5);
    EXPECT_EQ((15_mps2).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_gn).value(), 1.5);
    EXPECT_EQ((15_gn).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mgn).value(), 1.5);
    EXPECT_EQ((15_mgn).value(), 15);
    EXPECT_TRUE(1000_mgn == 1_gn);
    EXPECT_TRUE(2.0_gn > 10.0_mps2);
}

TEST(UnitsAcceleration, DerivedArithmetic)
{
    // velocity / duration -> meters_per_second_squared
    const auto accel = 10.0_mps / 2.0_s;
    static_assert(is_same<decltype(accel), const cu::meters_per_second_squared<double>>::value, "v / t");
    EXPECT_DOUBLE_EQ(accel.value(), 5.0);
    // Mixed scales keep the quotient in a scaled unit; convert to compare.
    const cu::meters_per_second_squared<double> launch = 36.0_kmph / 5.0_s;
    EXPECT_DOUBLE_EQ(launch.value(), 2.0);
    const auto int_accel = 12_mps / castle::chrono::duration<int64_t, castle::math::ratio<1>>(3);
    static_assert(is_same<decltype(int_accel), const cu::meters_per_second_squared<int64_t>>::value, "int v / t");
    EXPECT_EQ(int_accel.value(), 4);

    // acceleration * duration -> meters_per_second
    const auto gained = 2.0_mps2 * 5.0_s;
    static_assert(is_same<decltype(gained), const cu::meters_per_second<double>>::value, "a * t");
    EXPECT_DOUBLE_EQ(gained.value(), 10.0);
    const cu::meters_per_second<double> fall = 1.0_gn * 2.0_s;
    EXPECT_DOUBLE_EQ(fall.value(), 19.6133);
}

} // namespace
