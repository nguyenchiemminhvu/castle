#include <gtest/gtest.h>

#include "castle/units/frequency.hpp"
#include "castle/units/time.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsFrequency, UnitDefinitions)
{
    static_assert(is_same<cu::hertz_unit::dimension, cu::dim::frequency>::value, "dimension");
    static_assert(cu::dim::frequency::time == -1 && cu::dim::frequency::angle == 0, "T^-1");
    static_assert(cu::hertz_unit::ratio::num == 1 && cu::hertz_unit::ratio::den == 1, "Hz");
    static_assert(cu::hertz_unit::offset::num == 0, "no offset");
    static_assert(cu::millihertz_unit::ratio::den == 1000, "mHz");
    static_assert(cu::kilohertz_unit::ratio::num == 1000, "kHz");
    static_assert(cu::megahertz_unit::ratio::num == 1000000, "MHz");
    static_assert(cu::gigahertz_unit::ratio::num == 1000000000, "GHz");
    EXPECT_EQ(cu::kilohertz_unit::ratio::num, 1000);
    EXPECT_EQ(cu::millihertz_unit::ratio::den, 1000);
}

TEST(UnitsFrequency, ExactConversions)
{
    EXPECT_EQ(cu::hertz<int64_t>(cu::kilohertz<int64_t>(2)).value(), 2000);
    EXPECT_EQ(cu::hertz<int64_t>(cu::megahertz<int64_t>(3)).value(), 3000000);
    EXPECT_EQ(cu::hertz<int64_t>(cu::gigahertz<int64_t>(1)).value(), 1000000000);
    EXPECT_EQ(cu::millihertz<int64_t>(cu::hertz<int64_t>(3)).value(), 3000);
    EXPECT_EQ(cu::kilohertz<int64_t>(cu::megahertz<int64_t>(2)).value(), 2000);
    EXPECT_EQ(cu::megahertz<int64_t>(cu::gigahertz<int64_t>(2)).value(), 2000);

    // Coarser integral units truncate and therefore need an explicit conversion.
    EXPECT_EQ(cu::hertz<int64_t>(cu::millihertz<int64_t>(2500)).value(), 2);
    EXPECT_EQ(cu::kilohertz<int64_t>(cu::hertz<int64_t>(1999)).value(), 1);

    const cu::hertz<double> khz = cu::kilohertz<double>(1.5);
    EXPECT_DOUBLE_EQ(khz.value(), 1500.0);
    const cu::megahertz<double> mhz = cu::hertz<double>(2500000.0);
    EXPECT_DOUBLE_EQ(mhz.value(), 2.5);
    const cu::hertz<double> mhz_in_hz = cu::millihertz<double>(250.0);
    EXPECT_DOUBLE_EQ(mhz_in_hz.value(), 0.25);
}

TEST(UnitsFrequency, Literals)
{
    static_assert(is_same<decltype(1.5_Hz), cu::hertz<double>>::value, "Hz");
    static_assert(is_same<decltype(15_Hz), cu::hertz<int64_t>>::value, "Hz int");
    static_assert(is_same<decltype(1.5_mHz), cu::millihertz<double>>::value, "mHz");
    static_assert(is_same<decltype(15_mHz), cu::millihertz<int64_t>>::value, "mHz int");
    static_assert(is_same<decltype(1.5_kHz), cu::kilohertz<double>>::value, "kHz");
    static_assert(is_same<decltype(15_kHz), cu::kilohertz<int64_t>>::value, "kHz int");
    static_assert(is_same<decltype(1.5_MHz), cu::megahertz<double>>::value, "MHz");
    static_assert(is_same<decltype(15_MHz), cu::megahertz<int64_t>>::value, "MHz int");
    static_assert(is_same<decltype(1.5_GHz), cu::gigahertz<double>>::value, "GHz");
    static_assert(is_same<decltype(15_GHz), cu::gigahertz<int64_t>>::value, "GHz int");
    EXPECT_DOUBLE_EQ((1.5_Hz).value(), 1.5);
    EXPECT_EQ((15_Hz).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mHz).value(), 1.5);
    EXPECT_EQ((15_mHz).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_kHz).value(), 1.5);
    EXPECT_EQ((15_kHz).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_MHz).value(), 1.5);
    EXPECT_EQ((15_MHz).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_GHz).value(), 1.5);
    EXPECT_EQ((15_GHz).value(), 15);
}

TEST(UnitsFrequency, MixedArithmeticAndComparison)
{
    const cu::hertz<int64_t> total = 1_kHz + 500_Hz;
    EXPECT_EQ(total.value(), 1500);
    EXPECT_TRUE(1_kHz == 1000_Hz);
    EXPECT_TRUE(1_MHz > 999_kHz);
    EXPECT_TRUE(500_mHz < 1_Hz);
    EXPECT_TRUE(2_GHz >= 2000_MHz);
}

TEST(UnitsFrequency, PeriodAndCycles)
{
    // The reciprocal of a period is a frequency in the reciprocal scale (1 / ms = kHz).
    const auto rate = cu::inverse(20.0_ms);
    static_assert(is_same<decltype(rate), const cu::kilohertz<double>>::value, "1 / ms");
    EXPECT_DOUBLE_EQ(rate.value(), 0.05);
    const cu::hertz<double> rate_hz = rate;
    EXPECT_DOUBLE_EQ(rate_hz.value(), 50.0);

    // 1 / frequency is a duration in seconds.
    const auto period = 1.0 / 2.0_kHz;
    static_assert(is_same<decltype(period), const cu::seconds<double>>::value, "period is a duration");
    EXPECT_DOUBLE_EQ(period.count(), 0.0005);
    const auto period_hz = 1.0 / 50.0_Hz;
    EXPECT_DOUBLE_EQ(period_hz.count(), 0.02);

    // Frequency times a duration is a plain, dimensionless cycle count.
    const auto cycles = 50.0_Hz * 2.0_s;
    static_assert(is_same<decltype(cycles), const double>::value, "cycles are dimensionless");
    EXPECT_DOUBLE_EQ(cycles, 100.0);
    const auto khz_cycles = 2.0_kHz * 500.0_ms;
    EXPECT_DOUBLE_EQ(khz_cycles, 1000.0);
}

} // namespace
