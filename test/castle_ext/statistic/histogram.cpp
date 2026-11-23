#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <stdint.h>

#include "castle_ext/statistic/histogram.hpp"

namespace
{
using castle::status;
using castle::statistic::histogram_range;

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

using fixed4 = castle::statistic::histogram<double, 4U>;
using fixed10 = castle::statistic::histogram<double, 10U>;
using expanding4 = castle::statistic::histogram<double, 4U, uint32_t, histogram_range::expanding>;

// Checks every bin of `histogram` against `expected`.
template <typename H>
void ExpectBins(const H& histogram, std::initializer_list<typename H::count_type> expected)
{
    ASSERT_EQ(static_cast<std::size_t>(H::bin_count()), expected.size());
    std::size_t index = 0U;
    for (const typename H::count_type value : expected)
    {
        typename H::count_type out{};
        EXPECT_EQ(histogram.bin(index, out), status::ok);
        EXPECT_EQ(out, value) << "bin " << index;
        ++index;
    }
}

TEST(HistogramTest, UnconfiguredEveryOperationReportsNotConfigured)
{
    fixed4 h;
    EXPECT_FALSE(h.configured());
    EXPECT_DOUBLE_EQ(h.lower(), 0.0);
    EXPECT_DOUBLE_EQ(h.upper(), 0.0);
    EXPECT_DOUBLE_EQ(h.bin_width(), 0.0);
    EXPECT_EQ(h.count(), 0U);
    EXPECT_TRUE(h.empty());
    EXPECT_EQ(h.underflow(), 0U);
    EXPECT_EQ(h.overflow(), 0U);
    EXPECT_EQ(fixed4::bin_count(), 4U);
    EXPECT_EQ(fixed4::range_policy, histogram_range::fixed);

    EXPECT_EQ(h.push(1.0), status::not_configured);
    EXPECT_EQ(h.push(kNaN), status::not_configured);
    EXPECT_EQ(h.remove(1.0), status::not_configured);

    const double values[] = {1.0};
    EXPECT_EQ(h.push_range(values, values + 1), status::not_configured);
    EXPECT_EQ(h.push_range(values, values), status::ok);

    double out = -7.0;
    double lo = -7.0;
    double hi = -7.0;
    EXPECT_EQ(h.bin_edges(0U, lo, hi), status::not_configured);
    EXPECT_EQ(h.bin_edges(99U, lo, hi), status::not_configured);
    EXPECT_EQ(h.mode(out), status::not_configured);
    EXPECT_EQ(h.quantile(0.5, out), status::not_configured);
    EXPECT_EQ(h.quantile(kNaN, out), status::not_configured);
    EXPECT_EQ(h.cdf(1.0, out), status::not_configured);
    EXPECT_EQ(h.cdf(kNaN, out), status::not_configured);
    EXPECT_EQ(out, -7.0);
    EXPECT_EQ(lo, -7.0);
    EXPECT_EQ(hi, -7.0);

    // bin() does not need a range.
    uint32_t weight = 5U;
    EXPECT_EQ(h.bin(0U, weight), status::ok);
    EXPECT_EQ(weight, 0U);
    EXPECT_EQ(h.bin(4U, weight), status::out_of_range);
}

TEST(HistogramTest, ConfigureRejectsInvalidRanges)
{
    constexpr double kMax = std::numeric_limits<double>::max();
    fixed4 h;
    EXPECT_EQ(h.configure(kNaN, 1.0), status::invalid_config);
    EXPECT_FALSE(h.configured());
    EXPECT_EQ(h.configure(0.0, kNaN), status::invalid_config);
    EXPECT_EQ(h.configure(0.0, kInf), status::invalid_config);
    EXPECT_EQ(h.configure(-kInf, 0.0), status::invalid_config);
    EXPECT_EQ(h.configure(1.0, 0.0), status::invalid_config);
    EXPECT_EQ(h.configure(1.0, 1.0), status::invalid_config);
    // Width overflows although both bounds are finite.
    EXPECT_EQ(h.configure(-kMax, kMax), status::invalid_config);
    EXPECT_FALSE(h.configured());
    EXPECT_DOUBLE_EQ(h.lower(), 0.0);
    EXPECT_DOUBLE_EQ(h.upper(), 0.0);
}

TEST(HistogramTest, ConfigureSuccessAndReconfigure)
{
    fixed4 h;
    EXPECT_EQ(h.configure(0.0, 10.0), status::ok);
    EXPECT_TRUE(h.configured());
    EXPECT_DOUBLE_EQ(h.lower(), 0.0);
    EXPECT_DOUBLE_EQ(h.upper(), 10.0);
    EXPECT_DOUBLE_EQ(h.bin_width(), 2.5);

    EXPECT_EQ(h.push(1.0), status::ok);
    EXPECT_EQ(h.count(), 1U);
    // Reconfiguring discards all weight.
    EXPECT_EQ(h.configure(-4.0, 4.0), status::ok);
    EXPECT_TRUE(h.empty());
    EXPECT_DOUBLE_EQ(h.lower(), -4.0);
    EXPECT_DOUBLE_EQ(h.bin_width(), 2.0);

    // An invalid range after a valid one unconfigures the histogram.
    EXPECT_EQ(h.push(1.0), status::ok);
    EXPECT_EQ(h.configure(1.0, 0.0), status::invalid_config);
    EXPECT_FALSE(h.configured());
    EXPECT_TRUE(h.empty());
    EXPECT_EQ(h.push(1.0), status::not_configured);
}

TEST(HistogramTest, ConstructorWithBounds)
{
    fixed4 good(0.0, 4.0);
    EXPECT_TRUE(good.configured());
    EXPECT_DOUBLE_EQ(good.bin_width(), 1.0);
    EXPECT_TRUE(good.empty());
    EXPECT_EQ(good.underflow(), 0U);
    EXPECT_EQ(good.overflow(), 0U);

    fixed4 equal(4.0, 4.0);
    EXPECT_FALSE(equal.configured());
    fixed4 nan_bound(kNaN, 1.0);
    EXPECT_FALSE(nan_bound.configured());
    EXPECT_EQ(nan_bound.push(1.0), status::not_configured);
}

TEST(HistogramTest, PushBinsUnderflowOverflowAndEdges)
{
    fixed4 h(0.0, 4.0);
    EXPECT_EQ(h.push(0.0), status::ok);   // exactly at lower -> bin 0
    EXPECT_EQ(h.push(0.99), status::ok);  // bin 0
    EXPECT_EQ(h.push(1.0), status::ok);   // bin 1
    EXPECT_EQ(h.push(3.5), status::ok);   // bin 3
    EXPECT_EQ(h.push(std::nextafter(4.0, 0.0)), status::ok); // just below upper -> last bin
    EXPECT_EQ(h.push(4.0), status::ok);   // at upper -> overflow
    EXPECT_EQ(h.push(-0.001), status::ok); // underflow
    EXPECT_EQ(h.push(1.0e9, 3U), status::ok); // weighted overflow

    ExpectBins(h, {2U, 1U, 0U, 2U});
    EXPECT_EQ(h.underflow(), 1U);
    EXPECT_EQ(h.overflow(), 4U);
    EXPECT_EQ(h.count(), 10U);
    EXPECT_FALSE(h.empty());

    EXPECT_EQ(h.push(2.5, 7U), status::ok);
    ExpectBins(h, {2U, 1U, 7U, 2U});
    EXPECT_EQ(h.count(), 17U);
}

TEST(HistogramTest, PushRejectsInvalidInput)
{
    fixed4 h(0.0, 4.0);
    EXPECT_EQ(h.push(kNaN), status::invalid_argument);
    EXPECT_EQ(h.push(kInf), status::invalid_argument);
    EXPECT_EQ(h.push(-kInf), status::invalid_argument);
    EXPECT_EQ(h.push(1.0, 0U), status::invalid_argument);
    EXPECT_TRUE(h.empty());
    ExpectBins(h, {0U, 0U, 0U, 0U});
}

TEST(HistogramTest, FloatingPointWeights)
{
    using H = castle::statistic::histogram<double, 4U, double>;
    H h(0.0, 4.0);
    EXPECT_EQ(h.push(1.0, kNaN), status::invalid_argument);
    EXPECT_EQ(h.push(1.0, kInf), status::invalid_argument);
    EXPECT_EQ(h.push(1.0, -1.0), status::invalid_argument);
    EXPECT_EQ(h.push(1.0, 0.0), status::invalid_argument);
    EXPECT_EQ(h.push(kNaN, 1.0), status::invalid_argument);
    EXPECT_TRUE(h.empty());

    EXPECT_EQ(h.push(1.0, 0.5), status::ok);
    EXPECT_EQ(h.push(1.2, 1.5), status::ok);
    ExpectBins(h, {0.0, 2.0, 0.0, 0.0});
    EXPECT_DOUBLE_EQ(h.count(), 2.0);

    // Target 1.0 sits in the middle of bin 1.
    double out = 0.0;
    EXPECT_EQ(h.quantile(0.5, out), status::ok);
    EXPECT_NEAR(out, 1.5, 1e-12);

    EXPECT_EQ(h.remove(1.0, kNaN), status::invalid_argument);
    EXPECT_EQ(h.remove(1.0, -1.0), status::invalid_argument);
    EXPECT_EQ(h.remove(1.0, 0.0), status::invalid_argument);
    EXPECT_EQ(h.remove(1.0, 5.0), status::not_found);
    EXPECT_EQ(h.remove(1.0, 0.25), status::ok);
    EXPECT_DOUBLE_EQ(h.count(), 1.75);

    // Single-precision samples with single-precision weights.
    castle::statistic::histogram<float, 4U, float> hf(0.0f, 4.0f);
    EXPECT_EQ(hf.push(1.0f, 2.5f), status::ok);
    EXPECT_FLOAT_EQ(hf.count(), 2.5f);
    float fout = 0.0f;
    EXPECT_EQ(hf.quantile(0.5f, fout), status::ok);
    EXPECT_NEAR(fout, 1.5f, 1e-6f);
    EXPECT_EQ(hf.cdf(2.0f, fout), status::ok);
    EXPECT_NEAR(fout, 1.0f, 1e-6f);
}

TEST(HistogramTest, IntegralCounterOverflowReportsFull)
{
    castle::statistic::histogram<double, 4U, uint8_t> h(0.0, 4.0);
    EXPECT_EQ(h.push(1.0, 200U), status::ok);
    EXPECT_EQ(h.push(1.0, 56U), status::full); // 200 + 56 > 255
    EXPECT_EQ(static_cast<unsigned>(h.count()), 200U);
    EXPECT_EQ(h.push(1.0, 55U), status::ok);   // exactly 255
    EXPECT_EQ(static_cast<unsigned>(h.count()), 255U);
    EXPECT_EQ(h.push(1.0), status::full);
    EXPECT_EQ(h.push(5.0), status::full);      // the overflow slot is guarded too
    EXPECT_EQ(static_cast<unsigned>(h.overflow()), 0U);
    EXPECT_EQ(h.remove(1.0, 255U), status::ok);
    EXPECT_TRUE(h.empty());
}

TEST(HistogramTest, PushRange)
{
    fixed4 h(0.0, 4.0);
    const int ints[] = {0, 1, 1, 3};
    EXPECT_EQ(h.push_range(ints, ints + 4), status::ok);
    ExpectBins(h, {1U, 2U, 0U, 1U});

    const double empty_values[] = {1.0};
    EXPECT_EQ(h.push_range(empty_values, empty_values), status::ok);
    EXPECT_EQ(h.count(), 4U);

    // The first failure stops the loop; earlier samples stay.
    fixed4 g(0.0, 4.0);
    const double with_nan[] = {0.5, kNaN, 2.5};
    EXPECT_EQ(g.push_range(with_nan, with_nan + 3), status::invalid_argument);
    ExpectBins(g, {1U, 0U, 0U, 0U});
}

TEST(HistogramTest, RemoveRetractsWeight)
{
    fixed4 h(0.0, 4.0);
    EXPECT_EQ(h.remove(1.5), status::not_found); // empty bin
    EXPECT_EQ(h.remove(kNaN), status::invalid_argument);
    EXPECT_EQ(h.remove(1.5, 0U), status::invalid_argument);

    EXPECT_EQ(h.push(1.5, 3U), status::ok);
    EXPECT_EQ(h.remove(1.5, 4U), status::not_found); // not enough weight
    EXPECT_EQ(h.count(), 3U);
    EXPECT_EQ(h.remove(1.5, 3U), status::ok);
    EXPECT_TRUE(h.empty());
    ExpectBins(h, {0U, 0U, 0U, 0U});

    // Underflow and overflow slots.
    EXPECT_EQ(h.remove(-1.0), status::not_found);
    EXPECT_EQ(h.remove(9.0), status::not_found);
    EXPECT_EQ(h.push(-1.0, 2U), status::ok);
    EXPECT_EQ(h.push(9.0, 2U), status::ok);
    EXPECT_EQ(h.remove(-1.0), status::ok);
    EXPECT_EQ(h.underflow(), 1U);
    EXPECT_EQ(h.remove(9.0, 2U), status::ok);
    EXPECT_EQ(h.overflow(), 0U);
    EXPECT_EQ(h.count(), 1U);
}

TEST(HistogramTest, BinAndBinEdges)
{
    fixed4 h(-2.0, 2.0);
    EXPECT_EQ(h.push(-1.5), status::ok);
    uint32_t weight = 99U;
    EXPECT_EQ(h.bin(0U, weight), status::ok);
    EXPECT_EQ(weight, 1U);
    EXPECT_EQ(h.bin(3U, weight), status::ok);
    EXPECT_EQ(weight, 0U);
    weight = 99U;
    EXPECT_EQ(h.bin(4U, weight), status::out_of_range);
    EXPECT_EQ(weight, 99U);

    double lo = 0.0;
    double hi = 0.0;
    EXPECT_EQ(h.bin_edges(0U, lo, hi), status::ok);
    EXPECT_DOUBLE_EQ(lo, -2.0);
    EXPECT_DOUBLE_EQ(hi, -1.0);
    EXPECT_EQ(h.bin_edges(2U, lo, hi), status::ok);
    EXPECT_DOUBLE_EQ(lo, 0.0);
    EXPECT_DOUBLE_EQ(hi, 1.0);
    EXPECT_EQ(h.bin_edges(3U, lo, hi), status::ok);
    EXPECT_DOUBLE_EQ(lo, 1.0);
    EXPECT_DOUBLE_EQ(hi, 2.0);
    lo = -9.0;
    hi = -9.0;
    EXPECT_EQ(h.bin_edges(4U, lo, hi), status::out_of_range);
    EXPECT_EQ(lo, -9.0);
    EXPECT_EQ(hi, -9.0);
}

TEST(HistogramTest, Mode)
{
    fixed10 h(0.0, 10.0);
    double out = -7.0;
    EXPECT_EQ(h.mode(out), status::empty);

    // Only out-of-range weight: still no in-range mode.
    EXPECT_EQ(h.push(-3.0, 4U), status::ok);
    EXPECT_EQ(h.push(30.0, 4U), status::ok);
    EXPECT_EQ(h.mode(out), status::empty);
    EXPECT_EQ(out, -7.0);

    // Tie: the lowest bin wins; bin 2 is [2, 3).
    EXPECT_EQ(h.push(6.5), status::ok);
    EXPECT_EQ(h.push(2.5), status::ok);
    EXPECT_EQ(h.mode(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.5);

    // A heavier later bin wins.
    EXPECT_EQ(h.push(6.5), status::ok);
    EXPECT_EQ(h.mode(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 6.5);

    // Heaviest bin is the first one.
    fixed10 first(0.0, 10.0);
    EXPECT_EQ(first.push(0.5, 5U), status::ok);
    EXPECT_EQ(first.push(9.5, 2U), status::ok);
    EXPECT_EQ(first.mode(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.5);
}

TEST(HistogramTest, QuantileInterpolatesInsideBins)
{
    fixed10 h(0.0, 10.0);
    // Bins: 0 -> 1, 1 -> 2, 3 -> 1; total 4.
    h.push(0.5);
    h.push(1.5);
    h.push(1.5);
    h.push(3.5);

    double out = -7.0;
    EXPECT_EQ(h.quantile(0.0, out), status::ok);
    EXPECT_NEAR(out, 0.0, 1e-12);
    EXPECT_EQ(h.quantile(0.25, out), status::ok);
    EXPECT_NEAR(out, 1.0, 1e-12);
    EXPECT_EQ(h.quantile(0.5, out), status::ok);
    EXPECT_NEAR(out, 1.5, 1e-12);
    EXPECT_EQ(h.quantile(0.75, out), status::ok);
    EXPECT_NEAR(out, 2.0, 1e-12);
    // Empty bin 2 is skipped: target 3.2 lands in bin 3.
    EXPECT_EQ(h.quantile(0.8, out), status::ok);
    EXPECT_NEAR(out, 3.2, 1e-12);
    EXPECT_EQ(h.quantile(1.0, out), status::ok);
    EXPECT_NEAR(out, 4.0, 1e-12);

    out = -7.0;
    EXPECT_EQ(h.quantile(kNaN, out), status::invalid_argument);
    EXPECT_EQ(h.quantile(-0.1, out), status::invalid_argument);
    EXPECT_EQ(h.quantile(1.1, out), status::invalid_argument);
    EXPECT_EQ(out, -7.0);
}

TEST(HistogramTest, QuantileEmptyAndLeadingEmptyBins)
{
    fixed10 h(0.0, 10.0);
    double out = -7.0;
    EXPECT_EQ(h.quantile(0.5, out), status::empty);
    EXPECT_EQ(out, -7.0);

    // Only bin 2 holds weight: p = 0 maps to its lower edge, p = 1 to its upper edge.
    h.push(2.5);
    EXPECT_EQ(h.quantile(0.0, out), status::ok);
    EXPECT_NEAR(out, 2.0, 1e-12);
    EXPECT_EQ(h.quantile(1.0, out), status::ok);
    EXPECT_NEAR(out, 3.0, 1e-12);
}

TEST(HistogramTest, QuantileWithUnderflow)
{
    fixed10 h(0.0, 10.0);
    h.push(-1.0, 2U);
    h.push(5.5, 2U);
    double out = -7.0;
    // Targets 1 and 2 are covered by the underflow weight -> lower().
    EXPECT_EQ(h.quantile(0.25, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);
    EXPECT_EQ(h.quantile(0.5, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);
    // Target 3 passes the underflow weight and lands mid-bin 5.
    EXPECT_EQ(h.quantile(0.75, out), status::ok);
    EXPECT_NEAR(out, 5.5, 1e-12);
    EXPECT_EQ(h.quantile(1.0, out), status::ok);
    EXPECT_NEAR(out, 6.0, 1e-12);
}

TEST(HistogramTest, QuantileWithOverflow)
{
    fixed10 h(0.0, 10.0);
    h.push(5.5, 1U);
    h.push(20.0, 3U);
    double out = -7.0;
    EXPECT_EQ(h.quantile(0.25, out), status::ok);
    EXPECT_NEAR(out, 6.0, 1e-12);
    // Past the in-range weight: reported as upper().
    EXPECT_EQ(h.quantile(0.26, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 10.0);
    EXPECT_EQ(h.quantile(0.9, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 10.0);
    EXPECT_EQ(h.quantile(1.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 10.0);

    // Only overflow weight.
    fixed10 only(0.0, 10.0);
    only.push(50.0);
    EXPECT_EQ(only.quantile(0.5, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 10.0);
}

TEST(HistogramTest, QuantileWithWideBins)
{
    fixed4 h(0.0, 100.0); // bin width 25
    h.push(10.0, 2U);     // bin 0
    h.push(60.0, 2U);     // bin 2 = [50, 75)
    double out = 0.0;
    EXPECT_EQ(h.quantile(0.5, out), status::ok);
    EXPECT_NEAR(out, 25.0, 1e-12);
    EXPECT_EQ(h.quantile(0.75, out), status::ok);
    EXPECT_NEAR(out, 62.5, 1e-12);
}

TEST(HistogramTest, CdfInterpolationAndBounds)
{
    fixed10 h(0.0, 10.0);
    double out = -7.0;
    EXPECT_EQ(h.cdf(kNaN, out), status::invalid_argument); // finite check precedes the empty check
    EXPECT_EQ(h.cdf(1.0, out), status::empty);
    EXPECT_EQ(out, -7.0);

    h.push(0.5);
    h.push(1.5);
    h.push(1.5);
    h.push(3.5);
    EXPECT_EQ(h.cdf(kNaN, out), status::invalid_argument);
    EXPECT_EQ(h.cdf(kInf, out), status::invalid_argument);
    EXPECT_EQ(h.cdf(-kInf, out), status::invalid_argument);
    EXPECT_EQ(out, -7.0);

    EXPECT_EQ(h.cdf(-1.0, out), status::ok); // below lower
    EXPECT_DOUBLE_EQ(out, 0.0);
    EXPECT_EQ(h.cdf(0.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);
    EXPECT_EQ(h.cdf(1.5, out), status::ok); // (1 + 0.5 * 2) / 4
    EXPECT_NEAR(out, 0.5, 1e-12);
    EXPECT_EQ(h.cdf(3.0, out), status::ok); // 3 / 4
    EXPECT_NEAR(out, 0.75, 1e-12);
    EXPECT_EQ(h.cdf(3.5, out), status::ok); // (3 + 0.5 * 1) / 4
    EXPECT_NEAR(out, 0.875, 1e-12);
    EXPECT_EQ(h.cdf(std::nextafter(10.0, 0.0), out), status::ok);
    EXPECT_NEAR(out, 1.0, 1e-9);
    EXPECT_EQ(h.cdf(10.0, out), status::ok); // at upper, no overflow
    EXPECT_DOUBLE_EQ(out, 1.0);
    EXPECT_EQ(h.cdf(100.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);
}

TEST(HistogramTest, CdfWithUnderflowAndOverflow)
{
    fixed10 h(0.0, 10.0);
    h.push(5.5);
    h.push(-5.0);
    h.push(20.0);
    h.push(20.0);
    // total 4, underflow 1, overflow 2.
    double out = 0.0;
    EXPECT_EQ(h.cdf(-1.0, out), status::ok); // conservative lower bound
    EXPECT_DOUBLE_EQ(out, 0.0);
    EXPECT_EQ(h.cdf(0.0, out), status::ok); // underflow counted: 1 / 4
    EXPECT_NEAR(out, 0.25, 1e-12);
    EXPECT_EQ(h.cdf(6.0, out), status::ok); // (1 + 1) / 4
    EXPECT_NEAR(out, 0.5, 1e-12);
    EXPECT_EQ(h.cdf(10.0, out), status::ok); // (4 - 2) / 4
    EXPECT_NEAR(out, 0.5, 1e-12);
    EXPECT_EQ(h.cdf(1.0e9, out), status::ok);
    EXPECT_NEAR(out, 0.5, 1e-12);
}

TEST(HistogramTest, ClearKeepsRange)
{
    fixed4 h(0.0, 4.0);
    h.push(1.0);
    h.push(-1.0);
    h.push(9.0);
    h.clear();
    EXPECT_TRUE(h.configured());
    EXPECT_DOUBLE_EQ(h.lower(), 0.0);
    EXPECT_DOUBLE_EQ(h.upper(), 4.0);
    EXPECT_TRUE(h.empty());
    EXPECT_EQ(h.count(), 0U);
    EXPECT_EQ(h.underflow(), 0U);
    EXPECT_EQ(h.overflow(), 0U);
    ExpectBins(h, {0U, 0U, 0U, 0U});
    EXPECT_EQ(h.push(1.0), status::ok);
}

TEST(HistogramTest, IntegralSamples)
{
    castle::statistic::histogram<int, 4U> h(0.0, 8.0); // bin width 2
    EXPECT_DOUBLE_EQ(h.bin_width(), 2.0);
    EXPECT_EQ(h.push(3), status::ok);  // bin 1
    EXPECT_EQ(h.push(0), status::ok);  // bin 0
    EXPECT_EQ(h.push(8), status::ok);  // overflow
    EXPECT_EQ(h.push(-1), status::ok); // underflow
    const int more[] = {7, 7};
    EXPECT_EQ(h.push_range(more, more + 2), status::ok); // bin 3
    ExpectBins(h, {1U, 1U, 0U, 2U});
    EXPECT_EQ(h.underflow(), 1U);
    EXPECT_EQ(h.overflow(), 1U);
    EXPECT_EQ(h.remove(7), status::ok);
    ExpectBins(h, {1U, 1U, 0U, 1U});

    double out = 0.0;
    EXPECT_EQ(h.mode(out), status::ok); // tie -> lowest bin, centre of [0, 2)
    EXPECT_DOUBLE_EQ(out, 1.0);
}

TEST(HistogramTest, ExpandingHandTrace)
{
    expanding4 h(0.0, 1.0);
    EXPECT_EQ(expanding4::range_policy, histogram_range::expanding);

    EXPECT_EQ(h.push(0.1), status::ok);
    ExpectBins(h, {1U, 0U, 0U, 0U});
    EXPECT_EQ(h.push(0.6), status::ok);
    ExpectBins(h, {1U, 0U, 1U, 0U});
    EXPECT_DOUBLE_EQ(h.upper(), 1.0);

    // 3.5 doubles the range twice: [0, 2) then [0, 4).
    EXPECT_EQ(h.push(3.5), status::ok);
    EXPECT_DOUBLE_EQ(h.lower(), 0.0);
    EXPECT_DOUBLE_EQ(h.upper(), 4.0);
    ExpectBins(h, {2U, 0U, 0U, 1U});

    // -2 doubles downwards once: [-4, 4).
    EXPECT_EQ(h.push(-2.0), status::ok);
    EXPECT_DOUBLE_EQ(h.lower(), -4.0);
    EXPECT_DOUBLE_EQ(h.upper(), 4.0);
    EXPECT_DOUBLE_EQ(h.bin_width(), 2.0);
    ExpectBins(h, {0U, 1U, 2U, 1U});
    EXPECT_EQ(h.count(), 4U);
    EXPECT_EQ(h.underflow(), 0U);
    EXPECT_EQ(h.overflow(), 0U);

    double lo = 0.0;
    double hi = 0.0;
    EXPECT_EQ(h.bin_edges(0U, lo, hi), status::ok);
    EXPECT_DOUBLE_EQ(lo, -4.0);
    EXPECT_DOUBLE_EQ(hi, -2.0);
    EXPECT_EQ(h.bin_edges(3U, lo, hi), status::ok);
    EXPECT_DOUBLE_EQ(lo, 2.0);
    EXPECT_DOUBLE_EQ(hi, 4.0);

    double out = 0.0;
    EXPECT_EQ(h.mode(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);
    EXPECT_EQ(h.quantile(0.5, out), status::ok); // target 2 inside bin 2: -4 + 2.5 * 2
    EXPECT_NEAR(out, 1.0, 1e-12);
    EXPECT_EQ(h.cdf(0.0, out), status::ok); // 1 / 4
    EXPECT_NEAR(out, 0.25, 1e-12);
}

TEST(HistogramTest, ExpandingManyDoublingsUpward)
{
    expanding4 h(0.0, 1.0);
    EXPECT_EQ(h.push(0.5), status::ok);
    ExpectBins(h, {0U, 0U, 1U, 0U});
    // [0, 1) -> [0, 128): seven doublings.
    EXPECT_EQ(h.push(100.0), status::ok);
    EXPECT_DOUBLE_EQ(h.lower(), 0.0);
    EXPECT_DOUBLE_EQ(h.upper(), 128.0);
    ExpectBins(h, {1U, 0U, 0U, 1U});
    EXPECT_EQ(h.overflow(), 0U);
    EXPECT_EQ(h.count(), 2U);
}

TEST(HistogramTest, ExpandingManyDoublingsDownward)
{
    expanding4 h(0.0, 1.0);
    EXPECT_EQ(h.push(0.5), status::ok);
    // [0, 1) -> [-127, 1): seven doublings.
    EXPECT_EQ(h.push(-100.0), status::ok);
    EXPECT_DOUBLE_EQ(h.lower(), -127.0);
    EXPECT_DOUBLE_EQ(h.upper(), 1.0);
    ExpectBins(h, {1U, 0U, 0U, 1U});
    EXPECT_EQ(h.underflow(), 0U);
    EXPECT_EQ(h.count(), 2U);
}

TEST(HistogramTest, ExpandingValueAtUpperBoundExpands)
{
    expanding4 h(0.0, 1.0);
    EXPECT_EQ(h.push(1.0), status::ok);
    EXPECT_DOUBLE_EQ(h.upper(), 2.0);
    ExpectBins(h, {0U, 0U, 1U, 0U});
    EXPECT_EQ(h.overflow(), 0U);
}

TEST(HistogramTest, ExpandingStopsWhenRangeWouldOverflow)
{
    // Upward: the doubled width is not finite.
    {
        expanding4 h(-1.5e308, -0.5e308);
        EXPECT_TRUE(h.configured());
        EXPECT_EQ(h.push(1.0e308), status::ok);
        EXPECT_EQ(h.overflow(), 1U);
        EXPECT_EQ(h.count(), 1U);
        EXPECT_DOUBLE_EQ(h.lower(), -1.5e308);
        EXPECT_DOUBLE_EQ(h.upper(), -0.5e308);
        ExpectBins(h, {0U, 0U, 0U, 0U});
    }
    // Upward: the width is finite but the next upper bound is not.
    {
        expanding4 h(1.0e308, 1.5e308);
        EXPECT_EQ(h.push(1.7e308), status::ok);
        EXPECT_EQ(h.overflow(), 1U);
        EXPECT_DOUBLE_EQ(h.lower(), 1.0e308);
        EXPECT_DOUBLE_EQ(h.upper(), 1.5e308);
    }
    // Downward: the doubled width is not finite.
    {
        expanding4 h(0.5e308, 1.5e308);
        EXPECT_EQ(h.push(-1.0e308), status::ok);
        EXPECT_EQ(h.underflow(), 1U);
        EXPECT_EQ(h.count(), 1U);
        EXPECT_DOUBLE_EQ(h.lower(), 0.5e308);
        EXPECT_DOUBLE_EQ(h.upper(), 1.5e308);
        ExpectBins(h, {0U, 0U, 0U, 0U});
    }
    // Downward: the width is finite but the next lower bound is not.
    {
        expanding4 h(-1.5e308, -1.0e308);
        EXPECT_EQ(h.push(-1.7e308), status::ok);
        EXPECT_EQ(h.underflow(), 1U);
        EXPECT_DOUBLE_EQ(h.lower(), -1.5e308);
        EXPECT_DOUBLE_EQ(h.upper(), -1.0e308);
    }
}

TEST(HistogramTest, ExpandingWithFloatingPointWeights)
{
    castle::statistic::histogram<double, 4U, double, histogram_range::expanding> h(0.0, 1.0);
    EXPECT_EQ(h.push(0.1, 0.5), status::ok);
    EXPECT_EQ(h.push(0.6, 1.5), status::ok);
    EXPECT_EQ(h.push(1.5, 2.0), status::ok); // [0, 2): merges bins pairwise
    EXPECT_DOUBLE_EQ(h.upper(), 2.0);
    // The first two bins merged into bins 0 and 1; 1.5 lands in bin 3 of [0, 2).
    ExpectBins(h, {0.5, 1.5, 0.0, 2.0});
}
} // namespace
