#include <gtest/gtest.h>

#include "castle/units/gain.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsGain, UnitDefinitions)
{
    static_assert(is_same<cu::bel_unit::dimension, cu::dim::dimensionless>::value, "dimensionless");
    static_assert(is_same<cu::bel_unit::kind, cu::gain_kind>::value, "bel kind");
    static_assert(is_same<cu::decibel_unit::dimension, cu::dim::dimensionless>::value, "dB dimensionless");
    static_assert(is_same<cu::decibel_unit::kind, cu::gain_kind>::value, "dB kind");
    static_assert(cu::bel_unit::ratio::num == 1 && cu::bel_unit::ratio::den == 1, "B");
    static_assert(cu::decibel_unit::ratio::num == 1 && cu::decibel_unit::ratio::den == 10, "dB");

    // A gain is explicitly constructed and never implicit from a plain number.
    static_assert(!castle::meta::is_convertible<double, cu::decibels<double>>::value, "no implicit from scalar");
    static_assert(!castle::meta::is_constructible<cu::decibels<double>, int, int>::value, "arity");
    static_assert(castle::meta::is_constructible<cu::decibels<double>, double>::value, "explicit ctor");
    static_assert(!castle::meta::is_convertible<cu::decibels<double>, double>::value, "no implicit to scalar");
}

TEST(UnitsGain, BelDecibelConversions)
{
    EXPECT_EQ(cu::decibels<int>(cu::bels<int>(2)).value(), 20);
    EXPECT_EQ(cu::decibels<int64_t>(cu::bels<int64_t>(3)).value(), 30);
    const cu::decibels<int> truncated_source = cu::bels<int>(1);
    EXPECT_EQ(truncated_source.value(), 10);
    const cu::bels<int> coarse(cu::decibels<int>(25));
    EXPECT_EQ(coarse.value(), 2);
    const cu::bels<double> bel = cu::decibels<double>(25.0);
    EXPECT_DOUBLE_EQ(bel.value(), 2.5);
    EXPECT_TRUE(2_B == 20_dB);
    EXPECT_TRUE(1_B < 11_dB);
}

TEST(UnitsGain, Literals)
{
    static_assert(is_same<decltype(1.5_dB), cu::decibels<double>>::value, "dB");
    static_assert(is_same<decltype(15_dB), cu::decibels<int64_t>>::value, "dB int");
    static_assert(is_same<decltype(1.5_B), cu::bels<double>>::value, "B");
    static_assert(is_same<decltype(15_B), cu::bels<int64_t>>::value, "B int");
    EXPECT_DOUBLE_EQ((1.5_dB).value(), 1.5);
    EXPECT_EQ((15_dB).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_B).value(), 1.5);
    EXPECT_EQ((15_B).value(), 15);
}

TEST(UnitsGain, CascadeAndNoScalarMixing)
{
    const cu::decibels<double> total = 20.0_dB + 3.0_dB;
    EXPECT_DOUBLE_EQ(total.value(), 23.0);
    const cu::decibels<double> mixed = 1.0_B + 3.0_dB;
    EXPECT_DOUBLE_EQ(mixed.value(), 13.0);
    const auto doubled = total * 2.0;
    static_assert(is_same<decltype(doubled), const cu::decibels<double>>::value, "scaled gain stays a gain");
    EXPECT_DOUBLE_EQ(doubled.value(), 46.0);
}

TEST(UnitsGain, PowerRatio)
{
    EXPECT_NEAR(cu::power_ratio(3.0_dB), 2.0, 0.01);
    EXPECT_DOUBLE_EQ(cu::power_ratio(0.0_dB), 1.0);
    EXPECT_NEAR(cu::power_ratio(10.0_dB), 10.0, 1e-12);
    EXPECT_NEAR(cu::power_ratio(20.0_dB), 100.0, 1e-10);
    EXPECT_NEAR(cu::power_ratio(-10.0_dB), 0.1, 1e-12);
    EXPECT_NEAR(cu::power_ratio(1.0_B), 10.0, 1e-12);
    EXPECT_NEAR(cu::power_ratio(2.0_B), 100.0, 1e-10);
}

TEST(UnitsGain, AmplitudeRatio)
{
    EXPECT_NEAR(cu::amplitude_ratio(20.0_dB), 10.0, 1e-12);
    EXPECT_NEAR(cu::amplitude_ratio(40.0_dB), 100.0, 1e-10);
    EXPECT_NEAR(cu::amplitude_ratio(6.0_dB), 2.0, 0.01);
    EXPECT_DOUBLE_EQ(cu::amplitude_ratio(0.0_dB), 1.0);
    EXPECT_NEAR(cu::amplitude_ratio(2.0_B), 10.0, 1e-12);
}

TEST(UnitsGain, IntegralRepReturnsDouble)
{
    static_assert(is_same<decltype(cu::power_ratio(20_dB)), double>::value, "power double");
    static_assert(is_same<decltype(cu::amplitude_ratio(20_dB)), double>::value, "amplitude double");
    EXPECT_NEAR(cu::power_ratio(20_dB), 100.0, 1e-10);
    EXPECT_NEAR(cu::amplitude_ratio(20_dB), 10.0, 1e-12);
    EXPECT_NEAR(cu::power_ratio(2_B), 100.0, 1e-10);
    EXPECT_NEAR(cu::amplitude_ratio(cu::decibels<int32_t>(40)), 100.0, 1e-10);
}

TEST(UnitsGain, FloatDoubleLongDoubleOverloads)
{
    const cu::decibels<float> f(20.0F);
    static_assert(is_same<decltype(cu::power_ratio(f)), float>::value, "float power");
    static_assert(is_same<decltype(cu::amplitude_ratio(f)), float>::value, "float amplitude");
    EXPECT_NEAR(cu::power_ratio(f), 100.0F, 1e-3F);
    EXPECT_NEAR(cu::amplitude_ratio(f), 10.0F, 1e-4F);

    const cu::decibels<double> d(20.0);
    static_assert(is_same<decltype(cu::power_ratio(d)), double>::value, "double power");
    EXPECT_NEAR(cu::power_ratio(d), 100.0, 1e-10);
    EXPECT_NEAR(cu::amplitude_ratio(d), 10.0, 1e-12);

    const cu::decibels<long double> l(20.0L);
    static_assert(is_same<decltype(cu::power_ratio(l)), long double>::value, "long double power");
    static_assert(is_same<decltype(cu::amplitude_ratio(l)), long double>::value, "long double amplitude");
    EXPECT_NEAR(static_cast<double>(cu::power_ratio(l)), 100.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(cu::amplitude_ratio(l)), 10.0, 1e-12);
}

TEST(UnitsGain, FromRatios)
{
    static_assert(is_same<decltype(cu::decibels_from_power_ratio(2.0F)), cu::decibels<float>>::value, "float");
    static_assert(is_same<decltype(cu::decibels_from_power_ratio(2.0)), cu::decibels<double>>::value, "double");
    static_assert(is_same<decltype(cu::decibels_from_power_ratio(2.0L)), cu::decibels<long double>>::value,
                  "long double");
    static_assert(is_same<decltype(cu::decibels_from_amplitude_ratio(2.0F)), cu::decibels<float>>::value, "float");
    static_assert(is_same<decltype(cu::decibels_from_amplitude_ratio(2.0)), cu::decibels<double>>::value, "double");
    static_assert(is_same<decltype(cu::decibels_from_amplitude_ratio(2.0L)), cu::decibels<long double>>::value,
                  "long double");

    EXPECT_NEAR(cu::decibels_from_power_ratio(2.0).value(), 3.0103, 1e-4);
    EXPECT_NEAR(cu::decibels_from_power_ratio(100.0).value(), 20.0, 1e-12);
    EXPECT_NEAR(cu::decibels_from_power_ratio(100.0F).value(), 20.0F, 1e-4F);
    EXPECT_NEAR(static_cast<double>(cu::decibels_from_power_ratio(100.0L).value()), 20.0, 1e-12);
    EXPECT_DOUBLE_EQ(cu::decibels_from_power_ratio(1.0).value(), 0.0);
    EXPECT_NEAR(cu::decibels_from_amplitude_ratio(10.0).value(), 20.0, 1e-12);
    EXPECT_NEAR(cu::decibels_from_amplitude_ratio(10.0F).value(), 20.0F, 1e-4F);
    EXPECT_NEAR(static_cast<double>(cu::decibels_from_amplitude_ratio(10.0L).value()), 20.0, 1e-12);
    EXPECT_NEAR(cu::decibels_from_amplitude_ratio(2.0).value(), 6.0206, 1e-4);
}

TEST(UnitsGain, RoundTrips)
{
    EXPECT_NEAR(cu::power_ratio(cu::decibels_from_power_ratio(3.7)), 3.7, 1e-12);
    EXPECT_NEAR(cu::amplitude_ratio(cu::decibels_from_amplitude_ratio(0.25)), 0.25, 1e-12);
    EXPECT_NEAR(cu::decibels_from_power_ratio(cu::power_ratio(-12.5_dB)).value(), -12.5, 1e-12);
    EXPECT_NEAR(cu::decibels_from_amplitude_ratio(cu::amplitude_ratio(7.0_dB)).value(), 7.0, 1e-12);
    EXPECT_NEAR(cu::power_ratio(cu::decibels_from_power_ratio(3.7F)), 3.7F, 1e-5F);

    // Power gain is the square of amplitude gain.
    const double amp = cu::amplitude_ratio(13.0_dB);
    EXPECT_NEAR(amp * amp, cu::power_ratio(13.0_dB), 1e-10);
    // A cascade multiplies the linear power ratios.
    EXPECT_NEAR(cu::power_ratio(10.0_dB + 20.0_dB), cu::power_ratio(10.0_dB) * cu::power_ratio(20.0_dB), 1e-9);
}

} // namespace
