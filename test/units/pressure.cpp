#include <gtest/gtest.h>

#include "castle/units/pressure.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsPressure, UnitDefinitions)
{
    static_assert(is_same<cu::pascal_unit::dimension, cu::dim::pressure>::value, "dimension");
    static_assert(is_same<cu::dim::pressure, cu::dimension<-1, 1, -2, 0, 0, 0, 0, 0>>::value, "exponents");
    static_assert(cu::pascal_unit::ratio::num == 1 && cu::pascal_unit::ratio::den == 1, "Pa");
    static_assert(cu::hectopascal_unit::ratio::num == 100, "hPa");
    static_assert(cu::kilopascal_unit::ratio::num == 1000, "kPa");
    static_assert(cu::megapascal_unit::ratio::num == 1000000, "MPa");
    static_assert(cu::bar_unit::ratio::num == 100000, "bar");
    static_assert(cu::millibar_unit::ratio::num == 100, "mbar");
    static_assert(cu::atmosphere_unit::ratio::num == 101325, "atm");
    static_assert(is_same<cu::hectopascal_unit, cu::millibar_unit>::value, "hPa is mbar");
    // 44482216152605 / 6451600000 reduced.
    EXPECT_EQ(cu::pound_per_square_inch_unit::ratio::num, 8896443230521LL);
    EXPECT_EQ(cu::pound_per_square_inch_unit::ratio::den, 1290320000LL);
}

TEST(UnitsPressure, ExactConversions)
{
    EXPECT_EQ(cu::pascals<int>(cu::hectopascals<int>(1013)).value(), 101300);
    EXPECT_EQ(cu::pascals<int>(cu::kilopascals<int>(3)).value(), 3000);
    EXPECT_EQ(cu::kilopascals<int>(cu::megapascals<int>(2)).value(), 2000);
    EXPECT_EQ(cu::pascals<int64_t>(cu::bars<int64_t>(2)).value(), 200000);
    EXPECT_EQ(cu::pascals<int>(cu::millibars<int>(5)).value(), 500);
    EXPECT_EQ(cu::pascals<int>(cu::atmospheres<int>(1)).value(), 101325);
    EXPECT_EQ(cu::millibars<int>(cu::bars<int>(1)).value(), 1000);
    EXPECT_EQ(cu::hectopascals<int>(cu::millibars<int>(7)).value(), 7);

    const cu::pascals<double> psi = cu::pounds_per_square_inch<double>(1.0);
    EXPECT_NEAR(psi.value(), 6894.757293168, 1e-6);
    const cu::kilopascals<double> tyre = cu::pounds_per_square_inch<double>(32.0);
    EXPECT_NEAR(tyre.value(), 220.6322333814, 1e-6);
    const cu::pounds_per_square_inch<double> back = cu::kilopascals<double>(6.894757293168);
    EXPECT_NEAR(back.value(), 1.0, 1e-12);
    const cu::bars<int> truncated(cu::pascals<int>(199999));
    EXPECT_EQ(truncated.value(), 1);
}

TEST(UnitsPressure, Literals)
{
    static_assert(is_same<decltype(1.5_Pa), cu::pascals<double>>::value, "Pa");
    static_assert(is_same<decltype(15_Pa), cu::pascals<int64_t>>::value, "Pa int");
    EXPECT_DOUBLE_EQ((1.5_Pa).value(), 1.5);
    EXPECT_EQ((15_Pa).value(), 15);
    static_assert(is_same<decltype(1.5_hPa), cu::hectopascals<double>>::value, "hPa");
    static_assert(is_same<decltype(15_hPa), cu::hectopascals<int64_t>>::value, "hPa int");
    EXPECT_DOUBLE_EQ((1.5_hPa).value(), 1.5);
    EXPECT_EQ((15_hPa).value(), 15);
    static_assert(is_same<decltype(1.5_kPa), cu::kilopascals<double>>::value, "kPa");
    static_assert(is_same<decltype(15_kPa), cu::kilopascals<int64_t>>::value, "kPa int");
    EXPECT_DOUBLE_EQ((1.5_kPa).value(), 1.5);
    EXPECT_EQ((15_kPa).value(), 15);
    static_assert(is_same<decltype(1.5_MPa), cu::megapascals<double>>::value, "MPa");
    static_assert(is_same<decltype(15_MPa), cu::megapascals<int64_t>>::value, "MPa int");
    EXPECT_DOUBLE_EQ((1.5_MPa).value(), 1.5);
    EXPECT_EQ((15_MPa).value(), 15);
    static_assert(is_same<decltype(1.5_bar), cu::bars<double>>::value, "bar");
    static_assert(is_same<decltype(15_bar), cu::bars<int64_t>>::value, "bar int");
    EXPECT_DOUBLE_EQ((1.5_bar).value(), 1.5);
    EXPECT_EQ((15_bar).value(), 15);
    static_assert(is_same<decltype(1.5_mbar), cu::millibars<double>>::value, "mbar");
    static_assert(is_same<decltype(15_mbar), cu::millibars<int64_t>>::value, "mbar int");
    EXPECT_DOUBLE_EQ((1.5_mbar).value(), 1.5);
    EXPECT_EQ((15_mbar).value(), 15);
    static_assert(is_same<decltype(1.5_atm), cu::atmospheres<double>>::value, "atm");
    static_assert(is_same<decltype(15_atm), cu::atmospheres<int64_t>>::value, "atm int");
    EXPECT_DOUBLE_EQ((1.5_atm).value(), 1.5);
    EXPECT_EQ((15_atm).value(), 15);
    static_assert(is_same<decltype(1.5_psi), cu::pounds_per_square_inch<double>>::value, "psi");
    static_assert(is_same<decltype(15_psi), cu::pounds_per_square_inch<int64_t>>::value, "psi int");
    EXPECT_DOUBLE_EQ((1.5_psi).value(), 1.5);
    EXPECT_EQ((15_psi).value(), 15);
}

TEST(UnitsPressure, MixedArithmetic)
{
    EXPECT_TRUE(1_hPa == 1_mbar);
    EXPECT_TRUE(1013_hPa == 1013_mbar);
    const cu::hectopascals<double> atm = 1.0_atm;
    EXPECT_DOUBLE_EQ(atm.value(), 1013.25);
    const cu::pascals<int64_t> tyre = 2_bar;
    EXPECT_EQ(tyre.value(), 200000);
    const cu::pascals<int64_t> sum = 2_bar + 150_hPa;
    EXPECT_EQ(sum.value(), 215000);
    EXPECT_TRUE(1_atm > 1_bar);
    EXPECT_TRUE(1_MPa == 10_bar);
    // Force = pressure * area.
    using square_meters = cu::quantity<cu::unit<cu::dimension<2, 0, 0, 0, 0, 0, 0, 0>>, double>;
    using newtons = cu::quantity<cu::unit<cu::dimension<1, 1, -2, 0, 0, 0, 0, 0>>, double>;
    const auto force = 2.0_kPa * square_meters(0.5);
    static_assert(decltype(force)::dimension::length == 1, "force L");
    static_assert(decltype(force)::dimension::mass == 1, "force M");
    static_assert(decltype(force)::dimension::time == -2, "force T");
    const newtons coherent = force;
    EXPECT_DOUBLE_EQ(coherent.value(), 1000.0);
    EXPECT_DOUBLE_EQ(1.0_bar / 1.0_kPa, 100.0);
}

} // namespace
