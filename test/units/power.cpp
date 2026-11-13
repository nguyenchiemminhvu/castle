#include <gtest/gtest.h>

#include "castle/units/power.hpp"
#include "castle/units/gain.hpp"
#include "castle/units/energy.hpp"
#include "castle/units/time.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

template <typename A, typename B, typename = void>
struct can_add : castle::meta::false_type
{
};

template <typename A, typename B>
struct can_add<A, B, castle::meta::void_t<decltype(castle::meta::declval<A>() + castle::meta::declval<B>())>>
    : castle::meta::true_type
{
};

template <typename A, typename B, typename = void>
struct can_subtract : castle::meta::false_type
{
};

template <typename A, typename B>
struct can_subtract<A, B, castle::meta::void_t<decltype(castle::meta::declval<A>() - castle::meta::declval<B>())>>
    : castle::meta::true_type
{
};

TEST(UnitsPower, UnitDefinitions)
{
    static_assert(is_same<cu::watt_unit::dimension, cu::dim::power>::value, "dimension");
    static_assert(is_same<cu::dim::power, cu::dimension<2, 1, -3, 0, 0, 0, 0, 0>>::value, "power dim");
    static_assert(cu::watt_unit::ratio::num == 1 && cu::watt_unit::ratio::den == 1, "W coherent");
    static_assert(cu::microwatt_unit::ratio::den == 1000000, "uW");
    static_assert(cu::milliwatt_unit::ratio::den == 1000, "mW");
    static_assert(cu::kilowatt_unit::ratio::num == 1000, "kW");
    static_assert(cu::megawatt_unit::ratio::num == 1000000, "MW");
}

TEST(UnitsPower, ExactConversions)
{
    EXPECT_EQ(cu::milliwatts<int>(cu::watts<int>(3)).value(), 3000);
    EXPECT_EQ(cu::microwatts<int64_t>(cu::milliwatts<int64_t>(2)).value(), 2000);
    EXPECT_EQ(cu::watts<int>(cu::kilowatts<int>(2)).value(), 2000);
    EXPECT_EQ(cu::kilowatts<int>(cu::megawatts<int>(2)).value(), 2000);
    EXPECT_EQ(cu::milliwatts<int>(cu::kilowatts<int>(1)).value(), 1000000);
    EXPECT_EQ(cu::kilowatts<int>(cu::watts<int>(1999)).value(), 1);

    const cu::watts<double> micro = cu::microwatts<double>(2500000.0);
    EXPECT_DOUBLE_EQ(micro.value(), 2.5);
    const cu::watts<double> milli = cu::milliwatts<double>(1500.0);
    EXPECT_DOUBLE_EQ(milli.value(), 1.5);
    const cu::kilowatts<double> mega = cu::megawatts<double>(1.0);
    EXPECT_DOUBLE_EQ(mega.value(), 1000.0);
}

TEST(UnitsPower, Literals)
{
    static_assert(is_same<decltype(1.5_uW), cu::microwatts<double>>::value, "uW");
    static_assert(is_same<decltype(15_uW), cu::microwatts<int64_t>>::value, "uW int");
    static_assert(is_same<decltype(1.5_mW), cu::milliwatts<double>>::value, "mW");
    static_assert(is_same<decltype(15_mW), cu::milliwatts<int64_t>>::value, "mW int");
    static_assert(is_same<decltype(1.5_W), cu::watts<double>>::value, "W");
    static_assert(is_same<decltype(15_W), cu::watts<int64_t>>::value, "W int");
    static_assert(is_same<decltype(1.5_kW), cu::kilowatts<double>>::value, "kW");
    static_assert(is_same<decltype(15_kW), cu::kilowatts<int64_t>>::value, "kW int");
    static_assert(is_same<decltype(1.5_MW), cu::megawatts<double>>::value, "MW");
    static_assert(is_same<decltype(15_MW), cu::megawatts<int64_t>>::value, "MW int");
    EXPECT_DOUBLE_EQ((1.5_uW).value(), 1.5);
    EXPECT_EQ((15_uW).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mW).value(), 1.5);
    EXPECT_EQ((15_mW).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_W).value(), 1.5);
    EXPECT_EQ((15_W).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_kW).value(), 1.5);
    EXPECT_EQ((15_kW).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_MW).value(), 1.5);
    EXPECT_EQ((15_MW).value(), 15);
}

TEST(UnitsPower, DerivedArithmetic)
{
    const cu::watts<double> average = 100.0_J / 4.0_s;
    EXPECT_DOUBLE_EQ(average.value(), 25.0);
    const auto rate = 100.0_J / cu::seconds<double>(4.0);
    static_assert(is_same<decltype(rate), const cu::watts<double>>::value, "J / s is watts");

    const cu::watt_hours<double> consumed = 2.0_W * cu::hours<double>(1.5);
    EXPECT_DOUBLE_EQ(consumed.value(), 3.0);
    const cu::joules<double> energy = 2.0_W * cu::hours<double>(1.5);
    EXPECT_DOUBLE_EQ(energy.value(), 10800.0);
    const cu::watt_hours<int64_t> exact = 5_W * cu::hours<int64_t>(2);
    EXPECT_EQ(exact.value(), 10);

    const cu::watts<double> budget = 1.5_W + 250.0_mW;
    EXPECT_DOUBLE_EQ(budget.value(), 1.75);
    EXPECT_TRUE(1_kW == 1000_W);
    EXPECT_TRUE(1_MW > 999_kW);
    EXPECT_TRUE(1_mW > 999_uW);
}

TEST(UnitsPower, DbmIsSeparateKind)
{
    static_assert(is_same<cu::decibel_milliwatt_unit::kind, cu::power_level_kind>::value, "kind");
    static_assert(is_same<cu::decibel_milliwatt_unit::dimension, cu::dim::dimensionless>::value, "dimensionless");
    static_assert(!castle::meta::is_convertible<double, cu::decibel_milliwatts<double>>::value, "no from scalar");
    static_assert(!castle::meta::is_convertible<cu::decibel_milliwatts<double>, double>::value, "no to scalar");
    static_assert(!castle::meta::is_constructible<cu::milliwatts<double>, cu::decibel_milliwatts<double>>::value,
                  "dBm is not linear power");
    static_assert(!castle::meta::is_constructible<cu::decibel_milliwatts<double>, cu::milliwatts<double>>::value,
                  "linear power is not dBm");
    static_assert(!castle::meta::is_constructible<cu::decibels<double>, cu::decibel_milliwatts<double>>::value,
                  "level is not gain");
    static_assert(!castle::meta::is_constructible<cu::decibel_milliwatts<double>, cu::decibels<double>>::value,
                  "gain is not level");

    static_assert(!can_add<cu::decibel_milliwatts<double>, cu::decibel_milliwatts<double>>::value, "dBm + dBm");
    static_assert(!can_add<cu::decibel_milliwatts<double>, cu::milliwatts<double>>::value, "dBm + mW");
    static_assert(!can_add<cu::decibel_milliwatts<double>, double>::value, "dBm + scalar");
    static_assert(!can_subtract<cu::decibels<double>, cu::decibel_milliwatts<double>>::value, "dB - dBm");
    static_assert(can_add<cu::decibel_milliwatts<double>, cu::decibels<double>>::value, "dBm + dB");
    static_assert(can_add<cu::decibels<double>, cu::decibel_milliwatts<double>>::value, "dB + dBm");
    static_assert(can_subtract<cu::decibel_milliwatts<double>, cu::decibels<double>>::value, "dBm - dB");
}

TEST(UnitsPower, DbmLiterals)
{
    static_assert(is_same<decltype(1.5_dBm), cu::decibel_milliwatts<double>>::value, "dBm");
    static_assert(is_same<decltype(30_dBm), cu::decibel_milliwatts<int64_t>>::value, "dBm int");
    EXPECT_DOUBLE_EQ((1.5_dBm).value(), 1.5);
    EXPECT_EQ((30_dBm).value(), 30);
    EXPECT_DOUBLE_EQ((-30.0_dBm).value(), -30.0);
}

TEST(UnitsPower, DbmToLinearPower)
{
    static_assert(is_same<decltype(cu::milliwatts_from_dbm(30_dBm)), cu::milliwatts<double>>::value, "int -> double");
    static_assert(is_same<decltype(cu::milliwatts_from_dbm(cu::decibel_milliwatts<float>(0.0F))),
                          cu::milliwatts<float>>::value, "float stays float");

    // 30 dBm = 1 W = 1000 mW.
    EXPECT_DOUBLE_EQ(cu::milliwatts_from_dbm(30.0_dBm).value(), 1000.0);
    const cu::watts<double> one_watt = cu::milliwatts_from_dbm(30_dBm);
    EXPECT_DOUBLE_EQ(one_watt.value(), 1.0);

    EXPECT_DOUBLE_EQ(cu::milliwatts_from_dbm(0.0_dBm).value(), 1.0);
    EXPECT_DOUBLE_EQ(cu::milliwatts_from_dbm(10_dBm).value(), 10.0);
    EXPECT_NEAR(cu::milliwatts_from_dbm(3.0_dBm).value(), 1.99526, 1e-5);
    const cu::microwatts<double> weak = cu::milliwatts_from_dbm(-30.0_dBm);
    EXPECT_DOUBLE_EQ(weak.value(), 1.0);
    const cu::kilowatts<double> strong = cu::milliwatts_from_dbm(60.0_dBm);
    EXPECT_DOUBLE_EQ(strong.value(), 1.0);
}

TEST(UnitsPower, LinearPowerToDbm)
{
    static_assert(is_same<decltype(cu::dbm_from_power(1_W)), cu::decibel_milliwatts<double>>::value, "int -> double");
    static_assert(is_same<decltype(cu::dbm_from_power(cu::watts<float>(1.0F))),
                          cu::decibel_milliwatts<float>>::value, "float stays float");

    // 1 W = 1000 mW = 30 dBm, whatever the input scale.
    EXPECT_DOUBLE_EQ(cu::dbm_from_power(1.0_W).value(), 30.0);
    EXPECT_DOUBLE_EQ(cu::dbm_from_power(1_W).value(), 30.0);
    EXPECT_DOUBLE_EQ(cu::dbm_from_power(1000.0_mW).value(), 30.0);
    EXPECT_DOUBLE_EQ(cu::dbm_from_power(1_mW).value(), 0.0);
    EXPECT_DOUBLE_EQ(cu::dbm_from_power(1_uW).value(), -30.0);
    EXPECT_DOUBLE_EQ(cu::dbm_from_power(1_kW).value(), 60.0);
    EXPECT_NEAR(cu::dbm_from_power(2.0_mW).value(), 3.0103, 1e-4);

    EXPECT_NEAR(cu::milliwatts_from_dbm(cu::dbm_from_power(0.25_W)).value(), 250.0, 1e-9);
    EXPECT_NEAR(cu::dbm_from_power(cu::milliwatts_from_dbm(-47.5_dBm)).value(), -47.5, 1e-12);
}

TEST(UnitsPower, DbmGainArithmetic)
{
    // Link budget: 20 dBm transmitter, 3 dB cable loss, 15 dB amplifier.
    const auto rx = 20.0_dBm - 3.0_dB + 15.0_dB;
    static_assert(is_same<decltype(rx), const cu::decibel_milliwatts<double>>::value, "level +- gain is level");
    EXPECT_DOUBLE_EQ(rx.value(), 32.0);

    const auto exact = 10_dBm + 20_dB;
    static_assert(is_same<decltype(exact), const cu::decibel_milliwatts<int64_t>>::value, "integral stays integral");
    EXPECT_EQ(exact.value(), 30);
    EXPECT_EQ((20_dB + 10_dBm).value(), 30);
    EXPECT_EQ((10_dBm + 2_B).value(), 30);  // bels convert to decibels
    EXPECT_EQ((30_dBm - 1_B).value(), 20);

    const auto gain = 30.0_dBm - 10_dBm;
    static_assert(is_same<decltype(gain), const cu::decibels<double>>::value, "level - level is gain");
    EXPECT_DOUBLE_EQ(gain.value(), 20.0);
    EXPECT_DOUBLE_EQ(cu::power_ratio(gain), 100.0);

    EXPECT_TRUE(30_dBm == 30.0_dBm);
    EXPECT_TRUE(-10.0_dBm < 0_dBm);
}

} // namespace
