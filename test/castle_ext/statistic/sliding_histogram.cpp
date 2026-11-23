#include <gtest/gtest.h>

#include <cstddef>
#include <initializer_list>
#include <limits>
#include <stdint.h>

#include "castle_ext/statistic/sliding_histogram.hpp"

namespace
{
using castle::status;

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

// Checks the weight of every bin of `histogram` against `expected`.
template <typename H>
void ExpectBins(const H& histogram, std::initializer_list<typename H::count_type> expected)
{
    ASSERT_EQ(static_cast<std::size_t>(H::bin_count()), expected.size());
    std::size_t index = 0U;
    for (const typename H::count_type value : expected)
    {
        typename H::count_type out{};
        EXPECT_EQ(histogram.bin(index, out), status::ok);
        EXPECT_EQ(static_cast<unsigned>(out), static_cast<unsigned>(value)) << "bin " << index;
        ++index;
    }
}

TEST(SlidingHistogramTest, CounterTypeIsSmallestUnsigned)
{
    static_assert(sizeof(castle::statistic::sliding_histogram<double, 3U, 4U>::histogram_type::count_type) == 1U,
                  "capacity 3 fits uint8_t");
    static_assert(sizeof(castle::statistic::sliding_histogram<double, 255U, 4U>::histogram_type::count_type) == 1U,
                  "capacity 255 fits uint8_t");
    static_assert(sizeof(castle::statistic::sliding_histogram<uint8_t, 256U, 4U>::histogram_type::count_type) == 2U,
                  "capacity 256 needs uint16_t");
    SUCCEED();
}

TEST(SlidingHistogramTest, UnconfiguredReportsNotConfigured)
{
    using S = castle::statistic::sliding_histogram<double, 3U, 4U>;
    S s;
    EXPECT_FALSE(s.configured());
    EXPECT_EQ(S::capacity(), 3U);
    EXPECT_TRUE(s.empty());
    EXPECT_FALSE(s.full());
    EXPECT_EQ(s.count(), 0U);

    EXPECT_EQ(s.push(1.0), status::not_configured);
    EXPECT_EQ(s.push(kNaN), status::not_configured); // the range check comes first
    EXPECT_EQ(s.count(), 0U);

    double out = -7.0;
    EXPECT_EQ(s.quantile(0.5, out), status::not_configured);
    EXPECT_EQ(s.median(out), status::not_configured);
    EXPECT_EQ(s.cdf(1.0, out), status::not_configured);
    EXPECT_EQ(s.mode(out), status::not_configured);
    EXPECT_EQ(out, -7.0);
    EXPECT_FALSE(s.distribution().configured());
}

TEST(SlidingHistogramTest, ConstructorWithBoundsAndConfigure)
{
    using S = castle::statistic::sliding_histogram<double, 3U, 4U>;
    S good(0.0, 4.0);
    EXPECT_TRUE(good.configured());
    EXPECT_DOUBLE_EQ(good.distribution().lower(), 0.0);
    EXPECT_DOUBLE_EQ(good.distribution().upper(), 4.0);

    S bad(4.0, 4.0);
    EXPECT_FALSE(bad.configured());
    EXPECT_EQ(bad.push(1.0), status::not_configured);

    S s;
    EXPECT_EQ(s.configure(kNaN, 1.0), status::invalid_config);
    EXPECT_FALSE(s.configured());
    EXPECT_EQ(s.configure(0.0, 4.0), status::ok);
    EXPECT_TRUE(s.configured());

    // Reconfiguring discards the window and the histogram.
    EXPECT_EQ(s.push(1.0), status::ok);
    EXPECT_EQ(s.push(2.0), status::ok);
    EXPECT_EQ(s.configure(0.0, 8.0), status::ok);
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.count(), 0U);
    EXPECT_TRUE(s.distribution().empty());
    EXPECT_EQ(s.samples().size(), 0U);
    EXPECT_DOUBLE_EQ(s.distribution().upper(), 8.0);

    // An invalid range also clears the window and unconfigures.
    EXPECT_EQ(s.push(1.0), status::ok);
    EXPECT_EQ(s.configure(1.0, 0.0), status::invalid_config);
    EXPECT_TRUE(s.empty());
    EXPECT_FALSE(s.configured());
}

TEST(SlidingHistogramTest, RejectsNonFiniteSamples)
{
    castle::statistic::sliding_histogram<double, 3U, 4U> s(0.0, 4.0);
    EXPECT_EQ(s.push(kNaN), status::invalid_argument);
    EXPECT_EQ(s.push(kInf), status::invalid_argument);
    EXPECT_EQ(s.push(-kInf), status::invalid_argument);
    EXPECT_TRUE(s.empty());
    EXPECT_TRUE(s.distribution().empty());

    EXPECT_EQ(s.push(1.5), status::ok);
    EXPECT_EQ(s.push(kNaN), status::invalid_argument);
    EXPECT_EQ(s.count(), 1U);
    EXPECT_EQ(s.distribution().count(), 1U);
}

TEST(SlidingHistogramTest, EvictionRetractsFromHistogram)
{
    using S = castle::statistic::sliding_histogram<double, 3U, 4U>;
    S s(0.0, 4.0); // bin width 1
    EXPECT_EQ(s.push(0.5), status::ok);
    EXPECT_EQ(s.push(1.5), status::ok);
    EXPECT_FALSE(s.full());
    EXPECT_EQ(s.push(2.5), status::ok);
    EXPECT_TRUE(s.full());
    ExpectBins(s.distribution(), {1, 1, 1, 0});

    EXPECT_EQ(s.push(3.5), status::ok); // evicts 0.5
    ExpectBins(s.distribution(), {0, 1, 1, 1});
    EXPECT_EQ(s.distribution().count(), 3U);

    EXPECT_EQ(s.push(10.0), status::ok); // evicts 1.5; overflow
    ExpectBins(s.distribution(), {0, 0, 1, 1});
    EXPECT_EQ(s.distribution().overflow(), 1U);
    EXPECT_EQ(s.distribution().count(), 3U);

    EXPECT_EQ(s.push(-1.0), status::ok); // evicts 2.5; underflow
    ExpectBins(s.distribution(), {0, 0, 0, 1});
    EXPECT_EQ(s.distribution().overflow(), 1U);
    EXPECT_EQ(s.distribution().underflow(), 1U);

    EXPECT_EQ(s.push(0.5), status::ok); // evicts 3.5
    ExpectBins(s.distribution(), {1, 0, 0, 0});
    EXPECT_EQ(s.distribution().overflow(), 1U);
    EXPECT_EQ(s.distribution().underflow(), 1U);

    EXPECT_EQ(s.push(0.5), status::ok); // evicts 10.0: overflow leaves
    ExpectBins(s.distribution(), {2, 0, 0, 0});
    EXPECT_EQ(s.distribution().overflow(), 0U);
    EXPECT_EQ(s.distribution().underflow(), 1U);

    EXPECT_EQ(s.push(0.5), status::ok); // evicts -1.0: underflow leaves
    ExpectBins(s.distribution(), {3, 0, 0, 0});
    EXPECT_EQ(s.distribution().underflow(), 0U);
    EXPECT_EQ(s.distribution().count(), 3U);
    EXPECT_EQ(s.count(), 3U);
}

TEST(SlidingHistogramTest, IntegralSamplesAndForwarders)
{
    using S = castle::statistic::sliding_histogram<unsigned, 3U, 10U>;
    S s(0.0, 10.0);
    const unsigned inputs[] = {1U, 2U, 3U, 4U, 11U};
    for (const unsigned value : inputs)
    {
        EXPECT_EQ(s.push(value), status::ok);
    }
    // Window {3, 4, 11}: bins 3 and 4 hold one each, overflow holds one.
    EXPECT_EQ(s.count(), 3U);
    EXPECT_EQ(s.distribution().overflow(), 1U);
    ExpectBins(s.distribution(), {0, 0, 0, 1, 1, 0, 0, 0, 0, 0});

    double out = 0.0;
    EXPECT_EQ(s.median(out), status::ok);
    EXPECT_NEAR(out, 4.5, 1e-12);
    EXPECT_EQ(s.quantile(0.5, out), status::ok);
    EXPECT_NEAR(out, 4.5, 1e-12);
    EXPECT_EQ(s.quantile(0.0, out), status::ok);
    EXPECT_NEAR(out, 3.0, 1e-12);
    EXPECT_EQ(s.quantile(1.0, out), status::ok);
    EXPECT_NEAR(out, 10.0, 1e-12);
    EXPECT_EQ(s.quantile(1.5, out), status::invalid_argument);

    // One of three samples lies below 4; two lie below the upper bound.
    EXPECT_EQ(s.cdf(4.0, out), status::ok);
    EXPECT_NEAR(out, 1.0 / 3.0, 1e-12);
    EXPECT_EQ(s.cdf(10.0, out), status::ok);
    EXPECT_NEAR(out, 2.0 / 3.0, 1e-12);
    EXPECT_EQ(s.cdf(kNaN, out), status::invalid_argument);

    // Tie between bins 3 and 4: the lowest wins; bin 3 is [3, 4).
    EXPECT_EQ(s.mode(out), status::ok);
    EXPECT_NEAR(out, 3.5, 1e-12);

    // Arrival order, oldest first.
    const auto& samples = s.samples();
    ASSERT_EQ(samples.size(), 3U);
    EXPECT_EQ(samples[0U], 3U);
    EXPECT_EQ(samples[1U], 4U);
    EXPECT_EQ(samples[2U], 11U);
}

TEST(SlidingHistogramTest, ClearKeepsRange)
{
    castle::statistic::sliding_histogram<double, 3U, 4U> s(0.0, 4.0);
    s.push(1.5);
    s.push(9.0);
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.count(), 0U);
    EXPECT_TRUE(s.distribution().empty());
    EXPECT_EQ(s.distribution().overflow(), 0U);
    EXPECT_EQ(s.samples().size(), 0U);
    EXPECT_TRUE(s.configured());

    double out = -7.0;
    EXPECT_EQ(s.median(out), status::empty);
    EXPECT_EQ(s.mode(out), status::empty);
    EXPECT_EQ(s.cdf(1.0, out), status::empty);
    EXPECT_EQ(out, -7.0);

    EXPECT_EQ(s.push(2.5), status::ok);
    EXPECT_EQ(s.median(out), status::ok);
    EXPECT_NEAR(out, 2.5, 1e-12);
}

TEST(SlidingHistogramTest, FloatSamples)
{
    castle::statistic::sliding_histogram<float, 2U, 4U> s(0.0f, 4.0f);
    EXPECT_EQ(s.push(0.5f), status::ok);
    EXPECT_EQ(s.push(1.5f), status::ok);
    EXPECT_EQ(s.push(3.5f), status::ok); // evicts 0.5
    float out = 0.0f;
    // Bins 1 and 3 hold one each; target 1 ends bin 1.
    EXPECT_EQ(s.median(out), status::ok);
    EXPECT_NEAR(out, 2.0f, 1e-6f);
    EXPECT_EQ(s.mode(out), status::ok);
    EXPECT_NEAR(out, 1.5f, 1e-6f);
    EXPECT_EQ(s.cdf(2.0f, out), status::ok);
    EXPECT_NEAR(out, 0.5f, 1e-6f);
}
} // namespace
