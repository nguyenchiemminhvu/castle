#include <gtest/gtest.h>

#include <limits>
#include <math.h>
#include <stdint.h>
#include <type_traits>

#include "castle_ext/statistic/sliding_moments.hpp"

namespace
{
using castle::status;

const double kSkewness = 1.13841995766; // {1, 2, 3, 4, 10}

// Deterministic pseudo-random stream (xorshift32).
class xorshift
{
public:
    double next()
    {
        state_ ^= state_ << 13U;
        state_ ^= state_ >> 17U;
        state_ ^= state_ << 5U;
        return static_cast<double>(state_ % 10000U) / 100.0; // [0, 100)
    }

private:
    uint32_t state_ = 2463534242U;
};

// Compares a sliding accumulator with a fresh running_moments over its window.
template <typename Sliding>
void expect_matches_fresh_window(const Sliding& sliding, double tolerance)
{
    castle::statistic::running_moments<double, Sliding::moments_type::order> fresh;
    for (const double sample : sliding.samples())
    {
        ASSERT_EQ(fresh.push(sample), status::ok);
    }
    ASSERT_EQ(sliding.count(), fresh.count());
    double expected = 0.0;
    double actual = 0.0;
    ASSERT_EQ(fresh.mean(expected), status::ok);
    ASSERT_EQ(sliding.mean(actual), status::ok);
    EXPECT_NEAR(actual, expected, tolerance);
    ASSERT_EQ(fresh.population_variance(expected), status::ok);
    ASSERT_EQ(sliding.population_variance(actual), status::ok);
    EXPECT_NEAR(actual, expected, tolerance);
    if (fresh.skewness(expected) == status::ok)
    {
        ASSERT_EQ(sliding.skewness(actual), status::ok);
        EXPECT_NEAR(actual, expected, tolerance);
    }
    if (fresh.excess_kurtosis(expected) == status::ok)
    {
        ASSERT_EQ(sliding.excess_kurtosis(actual), status::ok);
        EXPECT_NEAR(actual, expected, tolerance);
    }
}

TEST(SlidingMomentsTest, EmptyState)
{
    castle::statistic::sliding_moments<double, 4U, 4U> sliding;
    EXPECT_EQ((castle::statistic::sliding_moments<double, 4U, 4U>::capacity()), 4U);
    EXPECT_TRUE(sliding.empty());
    EXPECT_FALSE(sliding.full());
    EXPECT_EQ(sliding.count(), 0U);
    EXPECT_TRUE(sliding.samples().empty());
    EXPECT_TRUE(sliding.moments().empty());

    double out = 42.0;
    EXPECT_EQ(sliding.mean(out), status::empty);
    EXPECT_EQ(sliding.rms(out), status::empty);
    EXPECT_EQ(sliding.population_variance(out), status::empty);
    EXPECT_EQ(sliding.sample_variance(out), status::empty);
    EXPECT_EQ(sliding.population_stddev(out), status::empty);
    EXPECT_EQ(sliding.sample_stddev(out), status::empty);
    EXPECT_EQ(sliding.skewness(out), status::empty);
    EXPECT_EQ(sliding.excess_kurtosis(out), status::empty);
    EXPECT_EQ(sliding.minimum(out), status::empty);
    EXPECT_EQ(sliding.maximum(out), status::empty);
    EXPECT_EQ(sliding.range(out), status::empty);
    EXPECT_EQ(out, 42.0);
    EXPECT_EQ(sliding.sum(), 0.0);
    EXPECT_EQ(sliding.sum_squares(), 0.0);
}

TEST(SlidingMomentsTest, NonEvictingPushesMatchReference)
{
    castle::statistic::sliding_moments<double, 5U, 4U> sliding;
    const double samples[] = {1.0, 2.0, 3.0, 4.0, 10.0};
    for (const double sample : samples)
    {
        EXPECT_EQ(sliding.push(sample), status::ok);
    }
    EXPECT_TRUE(sliding.full());
    EXPECT_EQ(sliding.count(), 5U);

    double out = 0.0;
    EXPECT_EQ(sliding.mean(out), status::ok);
    EXPECT_NEAR(out, 4.0, 1e-12);
    EXPECT_NEAR(sliding.sum(), 20.0, 1e-12);
    EXPECT_NEAR(sliding.sum_squares(), 130.0, 1e-9);
    EXPECT_EQ(sliding.rms(out), status::ok);
    EXPECT_NEAR(out, sqrt(26.0), 1e-9);
    EXPECT_EQ(sliding.population_variance(out), status::ok);
    EXPECT_NEAR(out, 10.0, 1e-9);
    EXPECT_EQ(sliding.sample_variance(out), status::ok);
    EXPECT_NEAR(out, 12.5, 1e-9);
    EXPECT_EQ(sliding.population_stddev(out), status::ok);
    EXPECT_NEAR(out, sqrt(10.0), 1e-9);
    EXPECT_EQ(sliding.sample_stddev(out), status::ok);
    EXPECT_NEAR(out, sqrt(12.5), 1e-9);
    EXPECT_EQ(sliding.skewness(out), status::ok);
    EXPECT_NEAR(out, kSkewness, 1e-9);
    EXPECT_EQ(sliding.excess_kurtosis(out), status::ok);
    EXPECT_NEAR(out, -0.212, 1e-9);
    EXPECT_EQ(sliding.moments().count(), 5U);
    EXPECT_EQ(sliding.samples().size(), 5U);
}

TEST(SlidingMomentsTest, EvictingPushesWithoutRebuild)
{
    castle::statistic::sliding_moments<double, 3U, 4U> sliding;
    for (int i = 1; i <= 3; ++i)
    {
        EXPECT_EQ(sliding.push(static_cast<double>(i)), status::ok);
    }
    EXPECT_EQ(sliding.push(4.0), status::ok); // window {2, 3, 4}
    double out = 0.0;
    EXPECT_EQ(sliding.mean(out), status::ok);
    EXPECT_NEAR(out, 3.0, 1e-12);
    EXPECT_EQ(sliding.population_variance(out), status::ok);
    EXPECT_NEAR(out, 2.0 / 3.0, 1e-12);
    EXPECT_EQ(sliding.push(10.0), status::ok); // window {3, 4, 10}
    EXPECT_EQ(sliding.mean(out), status::ok);
    EXPECT_NEAR(out, 17.0 / 3.0, 1e-12);
    EXPECT_EQ(sliding.minimum(out), status::ok);
    EXPECT_EQ(out, 3.0);
    EXPECT_EQ(sliding.maximum(out), status::ok);
    EXPECT_EQ(out, 10.0);
    EXPECT_EQ(sliding.range(out), status::ok);
    EXPECT_EQ(out, 7.0);
    expect_matches_fresh_window(sliding, 1e-9);
}

TEST(SlidingMomentsTest, OutlierLeavingTriggersRebuild)
{
    castle::statistic::sliding_moments<double, 4U, 4U> sliding;
    EXPECT_EQ(sliding.push(1.0e9), status::ok);
    EXPECT_EQ(sliding.push(1.0), status::ok);
    EXPECT_EQ(sliding.push(2.0), status::ok);
    EXPECT_EQ(sliding.push(3.0), status::ok);
    // Evicting 1e9 shrinks the variance by far more than 1/sqrt(eps): rebuild.
    EXPECT_EQ(sliding.push(4.0), status::ok); // window {1, 2, 3, 4}

    double out = 0.0;
    EXPECT_EQ(sliding.mean(out), status::ok);
    EXPECT_NEAR(out, 2.5, 1e-9);
    EXPECT_EQ(sliding.population_variance(out), status::ok);
    EXPECT_NEAR(out, 1.25, 1e-9);
    EXPECT_EQ(sliding.skewness(out), status::ok);
    EXPECT_NEAR(out, 0.0, 1e-9);
    EXPECT_EQ(sliding.excess_kurtosis(out), status::ok);
    EXPECT_NEAR(out, -1.36, 1e-9);
    expect_matches_fresh_window(sliding, 1e-9);
}

TEST(SlidingMomentsTest, FreshAccumulatorReplacesCurrentAfterCapacityEvictions)
{
    castle::statistic::sliding_moments<double, 4U, 4U> sliding;
    const double stream[] = {1.0e9, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    // The last four pushes are the 1st..4th evicting ones; the 4th swaps `fresh_` in.
    for (const double sample : stream)
    {
        EXPECT_EQ(sliding.push(sample), status::ok);
    }
    EXPECT_EQ(sliding.count(), 4U); // window {4, 5, 6, 7}
    double out = 0.0;
    EXPECT_EQ(sliding.mean(out), status::ok);
    EXPECT_NEAR(out, 5.5, 1e-12);
    EXPECT_EQ(sliding.population_variance(out), status::ok);
    EXPECT_NEAR(out, 1.25, 1e-12);
    expect_matches_fresh_window(sliding, 1e-12);

    // Keeps working after the swap (accumulator restarted from scratch).
    EXPECT_EQ(sliding.push(8.0), status::ok); // window {5, 6, 7, 8}
    EXPECT_EQ(sliding.mean(out), status::ok);
    EXPECT_NEAR(out, 6.5, 1e-12);
    expect_matches_fresh_window(sliding, 1e-12);
}

TEST(SlidingMomentsTest, LongStreamTracksFreshWindow)
{
    castle::statistic::sliding_moments<double, 16U, 4U> sliding;
    xorshift generator;
    for (int i = 0; i < 300; ++i)
    {
        ASSERT_EQ(sliding.push(generator.next()), status::ok);
        if (sliding.count() >= 4U)
        {
            expect_matches_fresh_window(sliding, 1e-6);
        }
    }
    EXPECT_TRUE(sliding.full());
}

TEST(SlidingMomentsTest, RejectsNonFiniteSamples)
{
    castle::statistic::sliding_moments<double, 4U, 2U> sliding;
    EXPECT_EQ(sliding.push(1.0), status::ok);
    EXPECT_EQ(sliding.push(std::numeric_limits<double>::quiet_NaN()), status::invalid_argument);
    EXPECT_EQ(sliding.push(std::numeric_limits<double>::infinity()), status::invalid_argument);
    EXPECT_EQ(sliding.push(-std::numeric_limits<double>::infinity()), status::invalid_argument);
    EXPECT_EQ(sliding.count(), 1U);
    double out = 0.0;
    EXPECT_EQ(sliding.mean(out), status::ok);
    EXPECT_EQ(out, 1.0);
}

TEST(SlidingMomentsTest, ClearEmptiesEverything)
{
    castle::statistic::sliding_moments<double, 3U, 2U> sliding;
    for (int i = 1; i <= 5; ++i)
    {
        EXPECT_EQ(sliding.push(static_cast<double>(i)), status::ok);
    }
    sliding.clear();
    EXPECT_TRUE(sliding.empty());
    EXPECT_TRUE(sliding.moments().empty());
    EXPECT_TRUE(sliding.samples().empty());
    EXPECT_EQ(sliding.push(10.0), status::ok);
    double out = 0.0;
    EXPECT_EQ(sliding.mean(out), status::ok);
    EXPECT_EQ(out, 10.0);
    EXPECT_EQ(sliding.sample_variance(out), status::empty);
}

TEST(SlidingMomentsTest, IntegralSamples)
{
    castle::statistic::sliding_moments<int, 3U, 3U> sliding;
    static_assert(std::is_same<decltype(sliding)::result_type, double>::value, "int reports in double");
    for (int i = 1; i <= 4; ++i)
    {
        EXPECT_EQ(sliding.push(i), status::ok); // window {2, 3, 4}
    }
    double out = 0.0;
    EXPECT_EQ(sliding.mean(out), status::ok);
    EXPECT_NEAR(out, 3.0, 1e-12);
    EXPECT_EQ(sliding.population_variance(out), status::ok);
    EXPECT_NEAR(out, 2.0 / 3.0, 1e-12);
    EXPECT_EQ(sliding.sample_variance(out), status::ok);
    EXPECT_NEAR(out, 1.0, 1e-12);
    EXPECT_EQ(sliding.skewness(out), status::ok);
    EXPECT_NEAR(out, 0.0, 1e-12);
    EXPECT_EQ(sliding.minimum(out), status::ok);
    EXPECT_EQ(out, 2.0);
    EXPECT_EQ(sliding.maximum(out), status::ok);
    EXPECT_EQ(out, 4.0);
    EXPECT_EQ(sliding.range(out), status::ok);
    EXPECT_EQ(out, 2.0);
}

TEST(SlidingMomentsTest, FloatSamples)
{
    castle::statistic::sliding_moments<float, 3U, 2U> sliding;
    for (int i = 1; i <= 4; ++i)
    {
        EXPECT_EQ(sliding.push(static_cast<float>(i)), status::ok);
    }
    float out = 0.0F;
    EXPECT_EQ(sliding.mean(out), status::ok);
    EXPECT_NEAR(out, 3.0F, 1e-5F);
    EXPECT_EQ(sliding.push(std::numeric_limits<float>::quiet_NaN()), status::invalid_argument);
}

TEST(SlidingMomentsTest, CapacityOne)
{
    castle::statistic::sliding_moments<double, 1U, 2U> sliding;
    EXPECT_EQ(sliding.push(5.0), status::ok);
    EXPECT_TRUE(sliding.full());
    EXPECT_EQ(sliding.push(7.0), status::ok); // evicting push, immediate swap
    EXPECT_EQ(sliding.push(9.0), status::ok);
    EXPECT_EQ(sliding.count(), 1U);
    double out = 0.0;
    EXPECT_EQ(sliding.mean(out), status::ok);
    EXPECT_EQ(out, 9.0);
    EXPECT_EQ(sliding.population_variance(out), status::ok);
    EXPECT_EQ(out, 0.0);
    EXPECT_EQ(sliding.sample_variance(out), status::empty);
    EXPECT_EQ(sliding.minimum(out), status::ok);
    EXPECT_EQ(out, 9.0);
    EXPECT_EQ(sliding.range(out), status::ok);
    EXPECT_EQ(out, 0.0);
}
} // namespace
