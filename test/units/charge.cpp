#include <gtest/gtest.h>

#include "castle/units/charge.hpp"
#include "castle/units/time.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

// Local ampere unit; current.hpp is a separate header.
using ampere_unit = cu::unit<cu::dim::current>;

TEST(UnitsCharge, UnitDefinitions)
{
    static_assert(is_same<cu::coulomb_unit::dimension, cu::dim::charge>::value, "dimension");
    static_assert(is_same<cu::dim::charge, cu::dimension<0, 0, 1, 1, 0, 0, 0, 0>>::value, "charge");
    static_assert(cu::coulomb_unit::ratio::num == 1 && cu::coulomb_unit::ratio::den == 1, "C");
    static_assert(cu::millicoulomb_unit::ratio::num == 1 && cu::millicoulomb_unit::ratio::den == 1000, "mC");
    static_assert(cu::ampere_hour_unit::ratio::num == 3600 && cu::ampere_hour_unit::ratio::den == 1, "Ah");
    static_assert(cu::milliampere_hour_unit::ratio::num == 18 && cu::milliampere_hour_unit::ratio::den == 5, "mAh");
    static_assert(is_same<cu::ampere_hour_unit::dimension, cu::dim::charge>::value, "Ah dimension");
    static_assert(is_same<cu::milliampere_hour_unit::dimension, cu::dim::charge>::value, "mAh dimension");
    static_assert(is_same<cu::millicoulomb_unit::dimension, cu::dim::charge>::value, "mC dimension");
}

TEST(UnitsCharge, ExactConversions)
{
    EXPECT_EQ(cu::millicoulombs<int>(cu::coulombs<int>(2)).value(), 2000);
    EXPECT_EQ(cu::coulombs<int>(cu::ampere_hours<int>(2)).value(), 7200);
    EXPECT_EQ(cu::millicoulombs<int64_t>(cu::ampere_hours<int64_t>(1)).value(), 3600000);
    EXPECT_EQ(cu::milliampere_hours<int>(cu::ampere_hours<int>(2)).value(), 2000);
    EXPECT_EQ(cu::millicoulombs<int>(cu::milliampere_hours<int>(5)).value(), 18000);
    EXPECT_EQ(cu::coulombs<int>(cu::milliampere_hours<int>(5)).value(), 18);

    const cu::coulombs<double> one_mah = cu::milliampere_hours<double>(1.0);
    EXPECT_DOUBLE_EQ(one_mah.value(), 3.6);
    const cu::ampere_hours<double> from_mah = cu::milliampere_hours<double>(2500.0);
    EXPECT_DOUBLE_EQ(from_mah.value(), 2.5);
    const cu::coulombs<double> from_mc = cu::millicoulombs<double>(1500.0);
    EXPECT_DOUBLE_EQ(from_mc.value(), 1.5);
    const cu::ampere_hours<int> truncated(cu::milliampere_hours<int>(2999));
    EXPECT_EQ(truncated.value(), 2);
    const cu::coulombs<int> coarse(cu::millicoulombs<int>(1999));
    EXPECT_EQ(coarse.value(), 1);
}

TEST(UnitsCharge, Literals)
{
    static_assert(is_same<decltype(1.5_C), cu::coulombs<double>>::value, "C");
    static_assert(is_same<decltype(15_C), cu::coulombs<int64_t>>::value, "C int");
    static_assert(is_same<decltype(1.5_mC), cu::millicoulombs<double>>::value, "mC");
    static_assert(is_same<decltype(15_mC), cu::millicoulombs<int64_t>>::value, "mC int");
    static_assert(is_same<decltype(1.5_Ah), cu::ampere_hours<double>>::value, "Ah");
    static_assert(is_same<decltype(15_Ah), cu::ampere_hours<int64_t>>::value, "Ah int");
    static_assert(is_same<decltype(1.5_mAh), cu::milliampere_hours<double>>::value, "mAh");
    static_assert(is_same<decltype(15_mAh), cu::milliampere_hours<int64_t>>::value, "mAh int");
    EXPECT_DOUBLE_EQ((1.5_C).value(), 1.5);
    EXPECT_EQ((15_C).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mC).value(), 1.5);
    EXPECT_EQ((15_mC).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_Ah).value(), 1.5);
    EXPECT_EQ((15_Ah).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mAh).value(), 1.5);
    EXPECT_EQ((15_mAh).value(), 15);
}

TEST(UnitsCharge, MixedArithmetic)
{
    const cu::milliampere_hours<double> drawn = 0.5_Ah + 250.0_mAh;
    EXPECT_DOUBLE_EQ(drawn.value(), 750.0);
    EXPECT_TRUE(1_Ah == 1000_mAh);
    EXPECT_TRUE(1_C == 1000_mC);
    EXPECT_TRUE(1_mAh > 3_C);
    EXPECT_TRUE(1_Ah > 1_mAh);
}

TEST(UnitsCharge, CurrentTimesDuration)
{
    const cu::quantity<ampere_unit, double> current(2.0);
    const auto charge = current * 3.0_s;
    static_assert(is_same<decltype(charge), const cu::coulombs<double>>::value, "A * s = C");
    EXPECT_DOUBLE_EQ(charge.value(), 6.0);

    // One ampere for one hour is one ampere-hour.
    const cu::quantity<ampere_unit, int64_t> one_amp(1);
    const cu::ampere_hours<int64_t> hour_charge = one_amp * cu::hours<int64_t>(1);
    EXPECT_EQ(hour_charge.value(), 1);
    const cu::coulombs<int64_t> in_coulombs = one_amp * cu::hours<int64_t>(1);
    EXPECT_EQ(in_coulombs.value(), 3600);
}

TEST(UnitsCharge, CoulombsOverDuration)
{
    const auto current = 6.0_C / 3.0_s;
    static_assert(is_same<decltype(current), const cu::quantity<ampere_unit, double>>::value, "C / s = A");
    EXPECT_DOUBLE_EQ(current.value(), 2.0);

    const cu::coulombs<double> charge = 1.0_Ah;
    const cu::quantity<ampere_unit, double> average = charge / cu::hours<double>(1.0);
    EXPECT_DOUBLE_EQ(average.value(), 1.0);
}

} // namespace
