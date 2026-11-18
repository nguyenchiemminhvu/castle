#include <gtest/gtest.h>

#include "castle/units/force.hpp"
#include "castle/units/mass.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsForce, UnitDefinitions)
{
    static_assert(is_same<cu::newton_unit::dimension, cu::dim::force>::value, "dimension");
    static_assert(is_same<cu::dim::force, cu::dimension<1, 1, -2, 0, 0, 0, 0, 0>>::value, "force dim");
    static_assert(cu::newton_unit::ratio::num == 1 && cu::newton_unit::ratio::den == 1, "N coherent");
    static_assert(cu::millinewton_unit::ratio::den == 1000, "mN");
    static_assert(cu::kilonewton_unit::ratio::num == 1000, "kN");
    // 4.4482216152605 N, stored reduced by the ratio.
    static_assert(cu::pound_force_unit::ratio::num == 8896443230521LL &&
                      cu::pound_force_unit::ratio::den == 2000000000000LL,
                  "lbf");
    EXPECT_DOUBLE_EQ(static_cast<double>(cu::pound_force_unit::ratio::num) /
                         static_cast<double>(cu::pound_force_unit::ratio::den),
                     4.4482216152605);
}

TEST(UnitsForce, ExactConversions)
{
    EXPECT_EQ(cu::millinewtons<int>(cu::newtons<int>(3)).value(), 3000);
    EXPECT_EQ(cu::newtons<int>(cu::kilonewtons<int>(2)).value(), 2000);
    EXPECT_EQ(cu::millinewtons<int64_t>(cu::kilonewtons<int64_t>(1)).value(), 1000000);
    EXPECT_EQ(cu::kilonewtons<int>(cu::newtons<int>(1999)).value(), 1);

    const cu::newtons<double> pound_force = cu::pounds_force<double>(1.0);
    EXPECT_DOUBLE_EQ(pound_force.value(), 4.4482216152605);
    const cu::newtons<double> milli = cu::millinewtons<double>(1500.0);
    EXPECT_DOUBLE_EQ(milli.value(), 1.5);
    const cu::pounds_force<double> back = cu::newtons<double>(4.4482216152605);
    EXPECT_DOUBLE_EQ(back.value(), 1.0);
}

TEST(UnitsForce, Literals)
{
    static_assert(is_same<decltype(1.5_N), cu::newtons<double>>::value, "N");
    static_assert(is_same<decltype(15_N), cu::newtons<int64_t>>::value, "N int");
    static_assert(is_same<decltype(1.5_mN), cu::millinewtons<double>>::value, "mN");
    static_assert(is_same<decltype(15_mN), cu::millinewtons<int64_t>>::value, "mN int");
    static_assert(is_same<decltype(1.5_kN), cu::kilonewtons<double>>::value, "kN");
    static_assert(is_same<decltype(15_kN), cu::kilonewtons<int64_t>>::value, "kN int");
    static_assert(is_same<decltype(1.5_lbf), cu::pounds_force<double>>::value, "lbf");
    static_assert(is_same<decltype(15_lbf), cu::pounds_force<int64_t>>::value, "lbf int");
    EXPECT_DOUBLE_EQ((1.5_N).value(), 1.5);
    EXPECT_EQ((15_N).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mN).value(), 1.5);
    EXPECT_EQ((15_mN).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_kN).value(), 1.5);
    EXPECT_EQ((15_kN).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_lbf).value(), 1.5);
    EXPECT_EQ((15_lbf).value(), 15);
}

TEST(UnitsForce, DerivedArithmetic)
{
    using acceleration = cu::quantity<cu::unit<cu::dimension<1, 0, -2, 0, 0, 0, 0, 0>>, double>;
    const cu::newtons<double> weight = 2.0_kg * acceleration(9.80665);
    EXPECT_DOUBLE_EQ(weight.value(), 19.6133);
    const auto force = 3.0_kg * acceleration(2.0);
    static_assert(is_same<decltype(force), const cu::newtons<double>>::value, "kg * accel is newtons");
    EXPECT_DOUBLE_EQ(force.value(), 6.0);
    const cu::newtons<double> thrust = 1.2_kN + 300.0_N;
    EXPECT_DOUBLE_EQ(thrust.value(), 1500.0);
    EXPECT_TRUE(1_kN == 1000_N);
    EXPECT_TRUE(2_N > 1500_mN);
    EXPECT_TRUE(1.0_lbf > 4.0_N);
    EXPECT_TRUE(1.0_lbf < 5.0_N);
    EXPECT_TRUE(1_lbf > 4_N);
    EXPECT_FALSE(1_lbf > 5_N);
    const cu::newtons<double> pound_weight = 1.0_kg * acceleration(9.80665);
    const cu::pounds_force<double> in_lbf = 0.45359237_kg * acceleration(9.80665);
    EXPECT_DOUBLE_EQ(in_lbf.value(), 1.0);
    EXPECT_DOUBLE_EQ(pound_weight.value(), 9.80665);
}

} // namespace
