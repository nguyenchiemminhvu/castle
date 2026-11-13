#include <gtest/gtest.h>

#include "castle/units/current.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsCurrent, UnitDefinitions)
{
    static_assert(is_same<cu::ampere_unit::dimension, cu::dim::current>::value, "dimension");
    static_assert(cu::ampere_unit::ratio::num == 1 && cu::ampere_unit::ratio::den == 1, "A");
    static_assert(cu::kiloampere_unit::ratio::num == 1000, "kA");
    static_assert(cu::milliampere_unit::ratio::den == 1000, "mA");
    static_assert(cu::microampere_unit::ratio::den == 1000000, "uA");
    static_assert(cu::dim::current::current == 1, "I exponent");
    EXPECT_EQ(cu::kiloampere_unit::ratio::den, 1);
    EXPECT_EQ(cu::milliampere_unit::ratio::num, 1);
}

TEST(UnitsCurrent, ExactConversions)
{
    EXPECT_EQ(cu::amperes<int>(cu::kiloamperes<int>(2)).value(), 2000);
    EXPECT_EQ(cu::milliamperes<int>(cu::amperes<int>(3)).value(), 3000);
    EXPECT_EQ(cu::microamperes<int64_t>(cu::amperes<int64_t>(2)).value(), 2000000);
    EXPECT_EQ(cu::microamperes<int>(cu::milliamperes<int>(7)).value(), 7000);

    const cu::amperes<double> from_milli = cu::milliamperes<double>(250.0);
    EXPECT_DOUBLE_EQ(from_milli.value(), 0.25);
    const cu::amperes<double> from_micro = cu::microamperes<double>(5.0);
    EXPECT_DOUBLE_EQ(from_micro.value(), 0.000005);
    const cu::amperes<double> from_kilo = cu::kiloamperes<double>(1.5);
    EXPECT_DOUBLE_EQ(from_kilo.value(), 1500.0);
    const cu::amperes<int> truncated(cu::milliamperes<int>(1999));
    EXPECT_EQ(truncated.value(), 1);
}

TEST(UnitsCurrent, Literals)
{
    static_assert(is_same<decltype(1.5_A), cu::amperes<double>>::value, "A");
    static_assert(is_same<decltype(15_A), cu::amperes<int64_t>>::value, "A int");
    EXPECT_DOUBLE_EQ((1.5_A).value(), 1.5);
    EXPECT_EQ((15_A).value(), 15);
    static_assert(is_same<decltype(1.5_mA), cu::milliamperes<double>>::value, "mA");
    static_assert(is_same<decltype(15_mA), cu::milliamperes<int64_t>>::value, "mA int");
    EXPECT_DOUBLE_EQ((1.5_mA).value(), 1.5);
    EXPECT_EQ((15_mA).value(), 15);
    static_assert(is_same<decltype(1.5_uA), cu::microamperes<double>>::value, "uA");
    static_assert(is_same<decltype(15_uA), cu::microamperes<int64_t>>::value, "uA int");
    EXPECT_DOUBLE_EQ((1.5_uA).value(), 1.5);
    EXPECT_EQ((15_uA).value(), 15);
    static_assert(is_same<decltype(1.5_kA), cu::kiloamperes<double>>::value, "kA");
    static_assert(is_same<decltype(15_kA), cu::kiloamperes<int64_t>>::value, "kA int");
    EXPECT_DOUBLE_EQ((1.5_kA).value(), 1.5);
    EXPECT_EQ((15_kA).value(), 15);
}

TEST(UnitsCurrent, MixedArithmetic)
{
    const cu::milliamperes<int64_t> load = 1_A + 250_mA;
    EXPECT_EQ(load.value(), 1250);
    EXPECT_TRUE(1_A == 1000_mA);
    EXPECT_TRUE(1_mA == 1000_uA);
    EXPECT_TRUE(1_kA > 999_A);
    const auto charge = 2.0_A * castle::chrono::duration<double, castle::math::ratio<1>>(3.0);
    static_assert(decltype(charge)::dimension::current == 1, "charge I");
    static_assert(decltype(charge)::dimension::time == 1, "charge T");
    EXPECT_DOUBLE_EQ(charge.value(), 6.0);
    EXPECT_DOUBLE_EQ(2.0_A / 500.0_mA, 4.0);
}

} // namespace
