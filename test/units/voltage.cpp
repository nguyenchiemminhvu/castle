#include <gtest/gtest.h>

#include "castle/units/voltage.hpp"
#include "castle/units/current.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsVoltage, UnitDefinitions)
{
    static_assert(is_same<cu::volt_unit::dimension, cu::dim::voltage>::value, "dimension");
    static_assert(is_same<cu::dim::voltage, cu::dimension<2, 1, -3, -1, 0, 0, 0, 0>>::value, "exponents");
    static_assert(cu::volt_unit::ratio::num == 1 && cu::volt_unit::ratio::den == 1, "V");
    static_assert(cu::kilovolt_unit::ratio::num == 1000, "kV");
    static_assert(cu::millivolt_unit::ratio::den == 1000, "mV");
    static_assert(cu::microvolt_unit::ratio::den == 1000000, "uV");
    EXPECT_EQ(cu::dim::voltage::length, 2);
    EXPECT_EQ(cu::dim::voltage::mass, 1);
    EXPECT_EQ(cu::dim::voltage::time, -3);
    EXPECT_EQ(cu::dim::voltage::current, -1);
}

TEST(UnitsVoltage, ExactConversions)
{
    EXPECT_EQ(cu::volts<int>(cu::kilovolts<int>(3)).value(), 3000);
    EXPECT_EQ(cu::millivolts<int>(cu::volts<int>(5)).value(), 5000);
    EXPECT_EQ(cu::microvolts<int64_t>(cu::volts<int64_t>(2)).value(), 2000000);
    EXPECT_EQ(cu::microvolts<int>(cu::millivolts<int>(4)).value(), 4000);

    const cu::volts<double> from_milli = cu::millivolts<double>(1650.0);
    EXPECT_DOUBLE_EQ(from_milli.value(), 1.65);
    const cu::volts<double> from_micro = cu::microvolts<double>(250.0);
    EXPECT_DOUBLE_EQ(from_micro.value(), 0.00025);
    const cu::volts<double> from_kilo = cu::kilovolts<double>(0.4);
    EXPECT_DOUBLE_EQ(from_kilo.value(), 400.0);
    const cu::volts<int> truncated(cu::millivolts<int>(3300));
    EXPECT_EQ(truncated.value(), 3);
}

TEST(UnitsVoltage, Literals)
{
    static_assert(is_same<decltype(1.5_V), cu::volts<double>>::value, "V");
    static_assert(is_same<decltype(15_V), cu::volts<int64_t>>::value, "V int");
    EXPECT_DOUBLE_EQ((1.5_V).value(), 1.5);
    EXPECT_EQ((15_V).value(), 15);
    static_assert(is_same<decltype(1.5_mV), cu::millivolts<double>>::value, "mV");
    static_assert(is_same<decltype(15_mV), cu::millivolts<int64_t>>::value, "mV int");
    EXPECT_DOUBLE_EQ((1.5_mV).value(), 1.5);
    EXPECT_EQ((15_mV).value(), 15);
    static_assert(is_same<decltype(1.5_uV), cu::microvolts<double>>::value, "uV");
    static_assert(is_same<decltype(15_uV), cu::microvolts<int64_t>>::value, "uV int");
    EXPECT_DOUBLE_EQ((1.5_uV).value(), 1.5);
    EXPECT_EQ((15_uV).value(), 15);
    static_assert(is_same<decltype(1.5_kV), cu::kilovolts<double>>::value, "kV");
    static_assert(is_same<decltype(15_kV), cu::kilovolts<int64_t>>::value, "kV int");
    EXPECT_DOUBLE_EQ((1.5_kV).value(), 1.5);
    EXPECT_EQ((15_kV).value(), 15);
}

TEST(UnitsVoltage, MixedArithmetic)
{
    const cu::millivolts<int64_t> rail = 3_V + 300_mV;
    EXPECT_EQ(rail.value(), 3300);
    EXPECT_TRUE(1_V == 1000_mV);
    EXPECT_TRUE(1_mV == 1000_uV);
    EXPECT_TRUE(1_kV > 999_V);
    // Electric power: V * A = M L^2 T^-3.
    const auto power = 12.0_V * 2.0_A;
    static_assert(decltype(power)::dimension::length == 2, "power L");
    static_assert(decltype(power)::dimension::mass == 1, "power M");
    static_assert(decltype(power)::dimension::time == -3, "power T");
    static_assert(decltype(power)::dimension::current == 0, "power I");
    EXPECT_DOUBLE_EQ(power.value(), 24.0);
    EXPECT_DOUBLE_EQ(3.3_V / 1100.0_mV, 3.0);
}

} // namespace
