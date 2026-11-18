#include <gtest/gtest.h>

#include "castle/units/temperature.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsTemperature, UnitDefinitions)
{
    static_assert(is_same<cu::kelvin_unit::dimension, cu::dim::temperature>::value, "dimension");
    static_assert(cu::kelvin_unit::ratio::num == 1 && cu::kelvin_unit::offset::num == 0, "K");
    static_assert(cu::millikelvin_unit::ratio::den == 1000 && cu::millikelvin_unit::offset::num == 0, "mK");
    static_assert(cu::celsius_unit::ratio::num == 1 && cu::celsius_unit::ratio::den == 1, "degC scale");
    static_assert(cu::celsius_unit::offset::num == 5463 && cu::celsius_unit::offset::den == 20, "degC offset");
    static_assert(cu::fahrenheit_unit::ratio::num == 5 && cu::fahrenheit_unit::ratio::den == 9, "degF scale");
    static_assert(cu::fahrenheit_unit::offset::num == 45967 && cu::fahrenheit_unit::offset::den == 180,
                  "degF offset");
    static_assert(cu::deci_celsius_unit::ratio::den == 10, "decidegC scale");
    static_assert(cu::deci_celsius_unit::offset::num == cu::celsius_unit::offset::num &&
                      cu::deci_celsius_unit::offset::den == cu::celsius_unit::offset::den,
                  "decidegC keeps the Celsius offset");
    static_assert(is_same<cu::coherent_unit_t<cu::celsius_unit>, cu::kelvin_unit>::value, "coherent");
    EXPECT_EQ(cu::celsius_unit::offset::num, 5463);
    EXPECT_EQ(cu::fahrenheit_unit::offset::den, 180);
}

TEST(UnitsTemperature, ExactIntegerConversions)
{
    // Kelvin steps need no offset arithmetic beyond the origin shift.
    EXPECT_EQ(cu::kelvin<int>(cu::celsius<int>(0)).value(), 273);
    EXPECT_EQ(cu::kelvin<int>(cu::celsius<int>(100)).value(), 373);
    EXPECT_EQ(cu::celsius<int>(cu::kelvin<int>(300)).value(), 26);
    EXPECT_EQ(cu::fahrenheit<int>(cu::celsius<int>(100)).value(), 212);
    EXPECT_EQ(cu::fahrenheit<int>(cu::celsius<int>(0)).value(), 32);
    EXPECT_EQ(cu::celsius<int>(cu::fahrenheit<int>(212)).value(), 100);
    EXPECT_EQ(cu::celsius<int>(cu::fahrenheit<int>(32)).value(), 0);
    EXPECT_EQ(cu::fahrenheit<int>(cu::celsius<int>(-40)).value(), -40);
    EXPECT_EQ(cu::celsius<int>(cu::fahrenheit<int>(-40)).value(), -40);
    EXPECT_EQ(cu::millikelvin<int64_t>(cu::kelvin<int64_t>(2)).value(), 2000);
    EXPECT_EQ(cu::kelvin<int64_t>(cu::millikelvin<int64_t>(2999)).value(), 2);
    EXPECT_EQ(cu::deci_celsius<int>(cu::celsius<int>(25)).value(), 250);
    EXPECT_EQ(cu::celsius<int>(cu::deci_celsius<int>(253)).value(), 25);
}

TEST(UnitsTemperature, FloatingConversions)
{
    const cu::kelvin<double> k = cu::celsius<double>(25.0);
    EXPECT_NEAR(k.value(), 298.15, 1e-9);
    const cu::celsius<double> c = cu::kelvin<double>(273.15);
    EXPECT_NEAR(c.value(), 0.0, 1e-9);
    const cu::fahrenheit<double> f = cu::celsius<double>(37.0);
    EXPECT_NEAR(f.value(), 98.6, 1e-9);
    const cu::celsius<double> from_f = cu::fahrenheit<double>(98.6);
    EXPECT_NEAR(from_f.value(), 37.0, 1e-9);
    const cu::kelvin<double> absolute_zero = cu::fahrenheit<double>(-459.67);
    EXPECT_NEAR(absolute_zero.value(), 0.0, 1e-9);
    const cu::millikelvin<double> mk = cu::celsius<double>(0.0);
    EXPECT_NEAR(mk.value(), 273150.0, 1e-6);

    // An int16 sensor in 0.1 degC steps converts losslessly to floating Celsius.
    const cu::celsius<double> sensor = cu::deci_celsius<int16_t>(253);
    EXPECT_DOUBLE_EQ(sensor.value(), 25.3);
    const cu::celsius<double> negative = cu::deci_celsius<int16_t>(-55);
    EXPECT_DOUBLE_EQ(negative.value(), -5.5);
    const cu::kelvin<double> sensor_k = cu::deci_celsius<int16_t>(253);
    EXPECT_NEAR(sensor_k.value(), 298.45, 1e-9);
    const cu::deci_celsius<double> back = cu::celsius<double>(25.3);
    EXPECT_NEAR(back.value(), 253.0, 1e-9);
}

TEST(UnitsTemperature, Literals)
{
    static_assert(is_same<decltype(1.5_K), cu::kelvin<double>>::value, "K");
    static_assert(is_same<decltype(15_K), cu::kelvin<int64_t>>::value, "K int");
    static_assert(is_same<decltype(1.5_mK), cu::millikelvin<double>>::value, "mK");
    static_assert(is_same<decltype(15_mK), cu::millikelvin<int64_t>>::value, "mK int");
    static_assert(is_same<decltype(1.5_degC), cu::celsius<double>>::value, "degC");
    static_assert(is_same<decltype(15_degC), cu::celsius<int64_t>>::value, "degC int");
    static_assert(is_same<decltype(1.5_degF), cu::fahrenheit<double>>::value, "degF");
    static_assert(is_same<decltype(15_degF), cu::fahrenheit<int64_t>>::value, "degF int");
    EXPECT_DOUBLE_EQ((1.5_K).value(), 1.5);
    EXPECT_EQ((15_K).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mK).value(), 1.5);
    EXPECT_EQ((15_mK).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_degC).value(), 1.5);
    EXPECT_EQ((15_degC).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_degF).value(), 1.5);
    EXPECT_EQ((15_degF).value(), 15);
}

TEST(UnitsTemperature, ComparisonsAcrossUnits)
{
    EXPECT_TRUE(-40_degC == -40_degF);
    EXPECT_TRUE(0.0_degC == 273.15_K);
    EXPECT_TRUE(300_K > 20_degC);
    EXPECT_TRUE(20_degC < 300_K);
    EXPECT_TRUE(100_degC > 211_degF);
    EXPECT_TRUE(100_degC < 213_degF);
    EXPECT_TRUE(100_degC != 100_degF);
    EXPECT_TRUE(0_degC >= 273_K);
    EXPECT_TRUE(0_degC <= 274_K);
    EXPECT_TRUE(cu::deci_celsius<int16_t>(253) > cu::celsius<int32_t>(25));
    EXPECT_TRUE(cu::deci_celsius<int16_t>(250) == cu::celsius<int32_t>(25));
}

TEST(UnitsTemperature, SameUnitAndKelvinArithmetic)
{
    // Same-unit sums and differences keep the unit and its offset.
    const cu::celsius<int64_t> warmer = 20_degC + 5_degC;
    EXPECT_EQ(warmer.value(), 25);
    const cu::celsius<int64_t> cooler = 20_degC - 5_degC;
    EXPECT_EQ(cooler.value(), 15);
    const cu::fahrenheit<double> f = 70.0_degF + 1.5_degF;
    EXPECT_DOUBLE_EQ(f.value(), 71.5);

    // Offset-free units mix freely.
    const cu::millikelvin<int64_t> total = 1_K + 500_mK;
    EXPECT_EQ(total.value(), 1500);

    // Differences between temperatures are best expressed in kelvin.
    const cu::kelvin<double> delta = cu::kelvin<double>(cu::celsius<double>(30.0)) -
                                     cu::kelvin<double>(cu::celsius<double>(20.0));
    EXPECT_NEAR(delta.value(), 10.0, 1e-9);
    const cu::kelvin<double> delta_f = cu::kelvin<double>(cu::fahrenheit<double>(212.0)) -
                                       cu::kelvin<double>(cu::fahrenheit<double>(32.0));
    EXPECT_NEAR(delta_f.value(), 100.0, 1e-9);

    // A scalar scales a value within its own unit; it does not touch the origin.
    const cu::celsius<double> scaled = 2.0 * cu::celsius<double>(10.0);
    EXPECT_DOUBLE_EQ(scaled.value(), 20.0);

    // The operators reject these cases with static_asserts on the traits checked below.
    using celsius_q = cu::celsius<double>;
    using fahrenheit_q = cu::fahrenheit<double>;
    using kelvin_q = cu::kelvin<double>;
    static_assert(cu::detail::is_additive<cu::celsius_unit, cu::celsius_unit>::value, "same unit +/-");
    static_assert(cu::detail::is_additive<cu::kelvin_unit, cu::millikelvin_unit>::value, "offset-free +/-");
    static_assert(!cu::detail::is_additive<cu::celsius_unit, cu::fahrenheit_unit>::value,
                  "degC +/- degF is rejected");
    static_assert(!cu::detail::is_additive<cu::celsius_unit, cu::kelvin_unit>::value,
                  "degC +/- K is rejected");
    static_assert(!cu::detail::is_additive<cu::celsius_unit, cu::deci_celsius_unit>::value,
                  "degC +/- decidegC is rejected");
    static_assert(cu::detail::product<celsius_q, celsius_q>::has_offset, "degC * degC is rejected");
    static_assert(cu::detail::product<celsius_q, fahrenheit_q>::has_offset, "degC * degF is rejected");
    static_assert(cu::detail::quotient<celsius_q, celsius_q>::has_offset, "degC / degC is rejected");
    static_assert(cu::detail::quotient<fahrenheit_q, kelvin_q>::has_offset, "degF / K is rejected");
    static_assert(!cu::detail::product<kelvin_q, kelvin_q>::has_offset, "K * K is allowed");
    static_assert(!cu::detail::quotient<kelvin_q, kelvin_q>::has_offset, "K / K is allowed");
    EXPECT_FALSE((cu::detail::is_additive<cu::celsius_unit, cu::fahrenheit_unit>::value));
    EXPECT_TRUE((cu::detail::product<celsius_q, celsius_q>::has_offset));
    const cu::celsius<double> c = cu::fahrenheit<double>(212.0);
    EXPECT_NEAR((c + cu::celsius<double>(0.0)).value(), 100.0, 1e-9);
    const auto speed = cu::kelvin<double>(2.0) * cu::kelvin<double>(3.0);
    static_assert(decltype(speed)::dimension::temperature == 2, "kelvin may be multiplied");
    EXPECT_DOUBLE_EQ(speed.value(), 6.0);
}

} // namespace
