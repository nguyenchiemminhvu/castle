#include <gtest/gtest.h>

#include "castle/units/energy.hpp"
#include "castle/units/force.hpp"
#include "castle/units/distance.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsEnergy, UnitDefinitions)
{
    static_assert(is_same<cu::joule_unit::dimension, cu::dim::energy>::value, "dimension");
    static_assert(is_same<cu::dim::energy, cu::dimension<2, 1, -2, 0, 0, 0, 0, 0>>::value, "energy dim");
    static_assert(cu::joule_unit::ratio::num == 1 && cu::joule_unit::ratio::den == 1, "J coherent");
    static_assert(cu::millijoule_unit::ratio::den == 1000, "mJ");
    static_assert(cu::kilojoule_unit::ratio::num == 1000, "kJ");
    static_assert(cu::megajoule_unit::ratio::num == 1000000, "MJ");
    static_assert(cu::milliwatt_hour_unit::ratio::num == 18 && cu::milliwatt_hour_unit::ratio::den == 5, "mWh");
    static_assert(cu::watt_hour_unit::ratio::num == 3600, "Wh");
    static_assert(cu::kilowatt_hour_unit::ratio::num == 3600000, "kWh");
}

TEST(UnitsEnergy, ExactConversions)
{
    EXPECT_EQ(cu::millijoules<int>(cu::joules<int>(3)).value(), 3000);
    EXPECT_EQ(cu::joules<int>(cu::kilojoules<int>(2)).value(), 2000);
    EXPECT_EQ(cu::kilojoules<int>(cu::megajoules<int>(2)).value(), 2000);
    EXPECT_EQ(cu::joules<int>(cu::watt_hours<int>(2)).value(), 7200);
    EXPECT_EQ(cu::joules<int64_t>(cu::kilowatt_hours<int64_t>(1)).value(), 3600000);
    EXPECT_EQ(cu::milliwatt_hours<int>(cu::watt_hours<int>(2)).value(), 2000);
    EXPECT_EQ(cu::watt_hours<int>(cu::kilowatt_hours<int>(3)).value(), 3000);
    EXPECT_EQ(cu::joules<int>(cu::milliwatt_hours<int>(5)).value(), 18);
    EXPECT_EQ(cu::milliwatt_hours<int>(cu::joules<int>(36)).value(), 10);
    EXPECT_EQ(cu::kilojoules<int>(cu::joules<int>(1999)).value(), 1);

    const cu::joules<double> mwh = cu::milliwatt_hours<double>(1.0);
    EXPECT_DOUBLE_EQ(mwh.value(), 3.6);
    const cu::megajoules<double> kwh = cu::kilowatt_hours<double>(1.0);
    EXPECT_DOUBLE_EQ(kwh.value(), 3.6);
    const cu::joules<double> milli = cu::millijoules<double>(2500.0);
    EXPECT_DOUBLE_EQ(milli.value(), 2.5);
    const cu::watt_hours<double> wh = cu::joules<double>(7200.0);
    EXPECT_DOUBLE_EQ(wh.value(), 2.0);
}

TEST(UnitsEnergy, Literals)
{
    static_assert(is_same<decltype(1.5_J), cu::joules<double>>::value, "J");
    static_assert(is_same<decltype(15_J), cu::joules<int64_t>>::value, "J int");
    static_assert(is_same<decltype(1.5_mJ), cu::millijoules<double>>::value, "mJ");
    static_assert(is_same<decltype(15_mJ), cu::millijoules<int64_t>>::value, "mJ int");
    static_assert(is_same<decltype(1.5_kJ), cu::kilojoules<double>>::value, "kJ");
    static_assert(is_same<decltype(15_kJ), cu::kilojoules<int64_t>>::value, "kJ int");
    static_assert(is_same<decltype(1.5_MJ), cu::megajoules<double>>::value, "MJ");
    static_assert(is_same<decltype(15_MJ), cu::megajoules<int64_t>>::value, "MJ int");
    static_assert(is_same<decltype(1.5_mWh), cu::milliwatt_hours<double>>::value, "mWh");
    static_assert(is_same<decltype(15_mWh), cu::milliwatt_hours<int64_t>>::value, "mWh int");
    static_assert(is_same<decltype(1.5_Wh), cu::watt_hours<double>>::value, "Wh");
    static_assert(is_same<decltype(15_Wh), cu::watt_hours<int64_t>>::value, "Wh int");
    static_assert(is_same<decltype(1.5_kWh), cu::kilowatt_hours<double>>::value, "kWh");
    static_assert(is_same<decltype(15_kWh), cu::kilowatt_hours<int64_t>>::value, "kWh int");
    EXPECT_DOUBLE_EQ((1.5_J).value(), 1.5);
    EXPECT_EQ((15_J).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mJ).value(), 1.5);
    EXPECT_EQ((15_mJ).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_kJ).value(), 1.5);
    EXPECT_EQ((15_kJ).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_MJ).value(), 1.5);
    EXPECT_EQ((15_MJ).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mWh).value(), 1.5);
    EXPECT_EQ((15_mWh).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_Wh).value(), 1.5);
    EXPECT_EQ((15_Wh).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_kWh).value(), 1.5);
    EXPECT_EQ((15_kWh).value(), 15);
}

TEST(UnitsEnergy, DerivedArithmetic)
{
    const cu::joules<double> work = 2.0_N * 3.0_m;
    EXPECT_DOUBLE_EQ(work.value(), 6.0);
    const auto product = 2.0_N * 3.0_m;
    static_assert(is_same<decltype(product), const cu::joules<double>>::value, "N * m is joules");
    const cu::joules<double> kilo_work = 2.0_kN * 3.0_m;
    EXPECT_DOUBLE_EQ(kilo_work.value(), 6000.0);
    const cu::joules<double> total = 1.5_kJ + 500.0_J;
    EXPECT_DOUBLE_EQ(total.value(), 2000.0);
    EXPECT_TRUE(1_kWh == 1000_Wh);
    EXPECT_TRUE(1_Wh == 1000_mWh);
    EXPECT_TRUE(1_MJ < 1_kWh);
    EXPECT_TRUE(1_kJ < 1_Wh);
    EXPECT_TRUE(3600_J == 1_Wh);
    EXPECT_TRUE(1_mJ < 1_J);
}

} // namespace
