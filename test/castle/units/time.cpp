#include <gtest/gtest.h>

#include "castle/units/time.hpp"
#include "castle/units/distance.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
namespace cc = castle::chrono;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsTime, AliasesAreChronoDurations)
{
    static_assert(is_same<cu::nanoseconds<int64_t>, cc::nanoseconds>::value, "ns");
    static_assert(is_same<cu::microseconds<int64_t>, cc::microseconds>::value, "us");
    static_assert(is_same<cu::milliseconds<int64_t>, cc::milliseconds>::value, "ms");
    static_assert(is_same<cu::seconds<int64_t>, cc::seconds>::value, "s");
    static_assert(is_same<cu::minutes<int64_t>, cc::minutes>::value, "min");
    static_assert(is_same<cu::hours<int64_t>, cc::hours>::value, "h");
    static_assert(is_same<cu::days<int64_t>, cc::days>::value, "d");
    static_assert(is_same<cu::weeks<int64_t>, cc::weeks>::value, "w");
    static_assert(is_same<cu::seconds<double>::period, cu::second>::value, "period");
    EXPECT_EQ(cu::milliseconds<uint32_t>(cu::seconds<uint32_t>(2)).count(), 2000U);
}

TEST(UnitsTime, IntegerLiterals)
{
    constexpr auto ns = 1_ns;
    constexpr auto us = 2_us;
    constexpr auto ms = 3_ms;
    constexpr auto sec = 4_s;
    constexpr auto mins = 5_min;
    constexpr auto h = 6_h;
    constexpr auto d = 7_d;
    constexpr auto w = 8_w;
    static_assert(is_same<decltype(ms), const cc::milliseconds>::value, "int literal type");
    static_assert(ns.count() == 1, "ns");
    EXPECT_EQ(ns.count(), 1);
    EXPECT_EQ(us.count(), 2);
    EXPECT_EQ(ms.count(), 3);
    EXPECT_EQ(sec.count(), 4);
    EXPECT_EQ(mins.count(), 5);
    EXPECT_EQ(h.count(), 6);
    EXPECT_EQ(d.count(), 7);
    EXPECT_EQ(w.count(), 8);
    EXPECT_EQ((ms + 2_ms).count(), 5);
    EXPECT_TRUE(1_w == 7_d);
}

TEST(UnitsTime, FloatingLiterals)
{
    static_assert(is_same<decltype(0.5_s), cu::seconds<double>>::value, "floating literal type");
    EXPECT_DOUBLE_EQ((1.5_ns).count(), 1.5);
    EXPECT_DOUBLE_EQ((1.5_us).count(), 1.5);
    EXPECT_DOUBLE_EQ((1.5_ms).count(), 1.5);
    EXPECT_DOUBLE_EQ((0.5_s).count(), 0.5);
    EXPECT_DOUBLE_EQ((1.5_min).count(), 1.5);
    EXPECT_DOUBLE_EQ((1.5_h).count(), 1.5);
    EXPECT_DOUBLE_EQ((0.5_d).count(), 0.5);
    EXPECT_DOUBLE_EQ((0.5_w).count(), 0.5);
    EXPECT_TRUE(1.5_min == 90.0_s);
}

TEST(UnitsTime, DurationsJoinUnitsArithmetic)
{
    const auto speed = 100.0_m / 4_s;
    EXPECT_DOUBLE_EQ(speed.value(), 25.0);
    const cu::meters<double> covered = speed * 2.0_min;
    EXPECT_DOUBLE_EQ(covered.value(), 3000.0);
    const auto eta = 1.0_km / speed;
    static_assert(is_same<decltype(eta), const cu::seconds<double>>::value, "time result is a duration");
    EXPECT_DOUBLE_EQ(eta.count(), 40.0);
    const auto frequency = cu::inverse(20.0_ms);
    EXPECT_DOUBLE_EQ(frequency.value(), 0.05);
    EXPECT_DOUBLE_EQ(cu::quantity_cast<cu::seconds<double>>(250_ms).count(), 0.25);
}

} // namespace
