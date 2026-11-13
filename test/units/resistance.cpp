#include <gtest/gtest.h>

#include "castle/units/resistance.hpp"
#include "castle/units/voltage.hpp"
#include "castle/units/current.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsResistance, UnitDefinitions)
{
    static_assert(is_same<cu::ohm_unit::dimension, cu::dim::resistance>::value, "dimension");
    static_assert(is_same<cu::dim::resistance, cu::dimension<2, 1, -3, -2, 0, 0, 0, 0>>::value, "exponents");
    static_assert(cu::ohm_unit::ratio::num == 1 && cu::ohm_unit::ratio::den == 1, "Ohm");
    static_assert(cu::milliohm_unit::ratio::den == 1000, "mOhm");
    static_assert(cu::kiloohm_unit::ratio::num == 1000, "kOhm");
    static_assert(cu::megaohm_unit::ratio::num == 1000000, "MOhm");
    EXPECT_EQ(cu::dim::resistance::current, -2);
    EXPECT_EQ(cu::dim::resistance::time, -3);
}

TEST(UnitsResistance, ExactConversions)
{
    EXPECT_EQ(cu::milliohms<int>(cu::ohms<int>(2)).value(), 2000);
    EXPECT_EQ(cu::ohms<int>(cu::kiloohms<int>(47)).value(), 47000);
    EXPECT_EQ(cu::ohms<int64_t>(cu::megaohms<int64_t>(2)).value(), 2000000);
    EXPECT_EQ(cu::kiloohms<int>(cu::megaohms<int>(1)).value(), 1000);

    const cu::ohms<double> from_milli = cu::milliohms<double>(50.0);
    EXPECT_DOUBLE_EQ(from_milli.value(), 0.05);
    const cu::ohms<double> from_kilo = cu::kiloohms<double>(4.7);
    EXPECT_DOUBLE_EQ(from_kilo.value(), 4700.0);
    const cu::ohms<double> from_mega = cu::megaohms<double>(1.5);
    EXPECT_DOUBLE_EQ(from_mega.value(), 1500000.0);
    const cu::kiloohms<int> truncated(cu::ohms<int>(4700));
    EXPECT_EQ(truncated.value(), 4);
}

TEST(UnitsResistance, Literals)
{
    static_assert(is_same<decltype(1.5_Ohm), cu::ohms<double>>::value, "Ohm");
    static_assert(is_same<decltype(15_Ohm), cu::ohms<int64_t>>::value, "Ohm int");
    EXPECT_DOUBLE_EQ((1.5_Ohm).value(), 1.5);
    EXPECT_EQ((15_Ohm).value(), 15);
    static_assert(is_same<decltype(1.5_mOhm), cu::milliohms<double>>::value, "mOhm");
    static_assert(is_same<decltype(15_mOhm), cu::milliohms<int64_t>>::value, "mOhm int");
    EXPECT_DOUBLE_EQ((1.5_mOhm).value(), 1.5);
    EXPECT_EQ((15_mOhm).value(), 15);
    static_assert(is_same<decltype(1.5_kOhm), cu::kiloohms<double>>::value, "kOhm");
    static_assert(is_same<decltype(15_kOhm), cu::kiloohms<int64_t>>::value, "kOhm int");
    EXPECT_DOUBLE_EQ((1.5_kOhm).value(), 1.5);
    EXPECT_EQ((15_kOhm).value(), 15);
    static_assert(is_same<decltype(1.5_MOhm), cu::megaohms<double>>::value, "MOhm");
    static_assert(is_same<decltype(15_MOhm), cu::megaohms<int64_t>>::value, "MOhm int");
    EXPECT_DOUBLE_EQ((1.5_MOhm).value(), 1.5);
    EXPECT_EQ((15_MOhm).value(), 15);
}

TEST(UnitsResistance, OhmsLaw)
{
    const cu::ohms<double> r = 12.0_V / 2.0_A;
    EXPECT_DOUBLE_EQ(r.value(), 6.0);
    const cu::ohms<double> scaled = 3.3_V / 10.0_mA;
    EXPECT_DOUBLE_EQ(scaled.value(), 330.0);

    const cu::volts<double> v = 4.7_kOhm * 2.0_mA;
    EXPECT_DOUBLE_EQ(v.value(), 9.4);
    const cu::volts<int64_t> vi = 6_Ohm * 2_A;
    EXPECT_EQ(vi.value(), 12);

    const cu::amperes<double> i = 5.0_V / 1.0_kOhm;
    EXPECT_DOUBLE_EQ(i.value(), 0.005);
    const cu::milliamperes<double> i_ma = 5.0_V / 1.0_kOhm;
    EXPECT_DOUBLE_EQ(i_ma.value(), 5.0);

    EXPECT_TRUE(1_kOhm == 1000_Ohm);
    EXPECT_TRUE(1_Ohm == 1000_mOhm);
    EXPECT_TRUE(1_MOhm == 1000_kOhm);
    const cu::ohms<int64_t> divider = 4_kOhm + 700_Ohm;
    EXPECT_EQ(divider.value(), 4700);
}

} // namespace
