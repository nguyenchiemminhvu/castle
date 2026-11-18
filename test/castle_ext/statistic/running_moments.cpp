#include <gtest/gtest.h>

#include <limits>
#include <math.h>
#include <stdint.h>
#include <type_traits>

#include "castle_ext/statistic/running_moments.hpp"

namespace
{
using castle::status;

const double kSkewness = 1.13841995766; // sqrt(5) * 180 / 50^1.5

// Fills an accumulator with {1, 2, 3, 4, 10}: mean 4, M2 50, M3 180, M4 1394.
template <typename Accumulator>
void fill_reference(Accumulator& moments)
{
    const double samples[] = {1.0, 2.0, 3.0, 4.0, 10.0};
    for (const double sample : samples)
    {
        EXPECT_EQ(moments.push(static_cast<typename Accumulator::result_type>(sample)), status::ok);
    }
}

// Reaches count == SIZE_MAX through self-merge doubling (no 2^64 pushes needed).
// Returns with `sink` holding SIZE_MAX samples and `doubled` holding 2^(bits-1).
template <typename Accumulator>
void reach_max_count(Accumulator& sink, Accumulator& doubled)
{
    const unsigned bits = static_cast<unsigned>(sizeof(castle::size_type) * 8U);
    EXPECT_EQ(doubled.push(1.0), status::ok);
    for (unsigned i = 0U; i + 1U < bits; ++i)
    {
        // sink: 2^i - 1 -> 2^(i+1) - 1 ; doubled: 2^i -> 2^(i+1)
        EXPECT_EQ(sink.merge(doubled), status::ok);
        EXPECT_EQ(doubled.merge(doubled), status::ok);
    }
    // sink = 2^(bits-1) - 1, doubled = 2^(bits-1): the sum is exactly SIZE_MAX.
    EXPECT_EQ(sink.merge(doubled), status::ok);
    EXPECT_EQ(sink.count(), std::numeric_limits<castle::size_type>::max());
}

TEST(CentralMomentsTest, EmptyAccumulator)
{
    castle::statistic::central_moments<double, 4U> moments;
    EXPECT_EQ((castle::statistic::central_moments<double, 4U>::order), 4U);
    EXPECT_TRUE(moments.empty());
    EXPECT_EQ(moments.count(), 0U);
    double out = 123.0;
    EXPECT_EQ(moments.mean(out), status::empty);
    EXPECT_EQ(moments.rms(out), status::empty);
    EXPECT_EQ(moments.population_variance(out), status::empty);
    EXPECT_EQ(moments.sample_variance(out), status::empty);
    EXPECT_EQ(moments.population_stddev(out), status::empty);
    EXPECT_EQ(moments.sample_stddev(out), status::empty);
    EXPECT_EQ(moments.skewness(out), status::empty);
    EXPECT_EQ(moments.excess_kurtosis(out), status::empty);
    EXPECT_EQ(out, 123.0);
    EXPECT_EQ(moments.sum(), 0.0);
    EXPECT_EQ(moments.sum_squares(), 0.0);
}

TEST(CentralMomentsTest, ReferenceStatisticsOrder4)
{
    castle::statistic::central_moments<double, 4U> moments;
    fill_reference(moments);
    EXPECT_EQ(moments.count(), 5U);
    EXPECT_FALSE(moments.empty());

    double out = 0.0;
    EXPECT_EQ(moments.mean(out), status::ok);
    EXPECT_NEAR(out, 4.0, 1e-12);
    EXPECT_NEAR(moments.sum(), 20.0, 1e-12);
    EXPECT_NEAR(moments.sum_squares(), 130.0, 1e-9); // 1+4+9+16+100
    EXPECT_EQ(moments.rms(out), status::ok);
    EXPECT_NEAR(out, sqrt(26.0), 1e-9);
    EXPECT_EQ(moments.population_variance(out), status::ok);
    EXPECT_NEAR(out, 10.0, 1e-9); // M2 / n = 50 / 5
    EXPECT_EQ(moments.sample_variance(out), status::ok);
    EXPECT_NEAR(out, 12.5, 1e-9); // 50 / 4
    EXPECT_EQ(moments.population_stddev(out), status::ok);
    EXPECT_NEAR(out, sqrt(10.0), 1e-9);
    EXPECT_EQ(moments.sample_stddev(out), status::ok);
    EXPECT_NEAR(out, sqrt(12.5), 1e-9);
    EXPECT_EQ(moments.skewness(out), status::ok);
    EXPECT_NEAR(out, kSkewness, 1e-9);
    EXPECT_EQ(moments.excess_kurtosis(out), status::ok);
    EXPECT_NEAR(out, -0.212, 1e-9); // 5 * 1394 / 2500 - 3
}

TEST(CentralMomentsTest, OrderTwoAndThreeInstantiations)
{
    castle::statistic::central_moments<double, 2U> second;
    castle::statistic::central_moments<double, 3U> third;
    fill_reference(second);
    fill_reference(third);

    double out = 0.0;
    EXPECT_EQ(second.population_variance(out), status::ok);
    EXPECT_NEAR(out, 10.0, 1e-9);
    EXPECT_EQ(second.sample_variance(out), status::ok);
    EXPECT_NEAR(out, 12.5, 1e-9);
    EXPECT_EQ(third.population_variance(out), status::ok);
    EXPECT_NEAR(out, 10.0, 1e-9);
    EXPECT_EQ(third.skewness(out), status::ok);
    EXPECT_NEAR(out, kSkewness, 1e-9);
}

TEST(CentralMomentsTest, SampleVarianceNeedsTwoSamples)
{
    castle::statistic::central_moments<double, 2U> moments;
    EXPECT_EQ(moments.push(5.0), status::ok);
    double out = 77.0;
    EXPECT_EQ(moments.sample_variance(out), status::empty);
    EXPECT_EQ(moments.sample_stddev(out), status::empty);
    EXPECT_EQ(out, 77.0);
    EXPECT_EQ(moments.population_variance(out), status::ok);
    EXPECT_EQ(out, 0.0);
    EXPECT_EQ(moments.population_stddev(out), status::ok);
    EXPECT_EQ(out, 0.0);
}

TEST(CentralMomentsTest, ZeroVarianceRejectsShapeStatistics)
{
    castle::statistic::central_moments<double, 4U> moments;
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(moments.push(5.0), status::ok);
    }
    double out = 55.0;
    EXPECT_EQ(moments.skewness(out), status::invalid_argument);
    EXPECT_EQ(moments.excess_kurtosis(out), status::invalid_argument);
    EXPECT_EQ(out, 55.0);
    EXPECT_EQ(moments.mean(out), status::ok);
    EXPECT_EQ(out, 5.0);
}

TEST(CentralMomentsTest, RemoveRetractsSamples)
{
    castle::statistic::central_moments<double, 4U> moments;
    double out = 0.0;
    EXPECT_EQ(moments.remove(1.0), status::empty);

    EXPECT_EQ(moments.push(8.0), status::ok);
    EXPECT_EQ(moments.remove(8.0), status::ok); // count == 1 clears
    EXPECT_TRUE(moments.empty());
    EXPECT_EQ(moments.mean(out), status::empty);

    fill_reference(moments);
    EXPECT_EQ(moments.remove(10.0), status::ok); // {1, 2, 3, 4}
    EXPECT_EQ(moments.count(), 4U);
    EXPECT_EQ(moments.mean(out), status::ok);
    EXPECT_NEAR(out, 2.5, 1e-12);
    EXPECT_EQ(moments.population_variance(out), status::ok);
    EXPECT_NEAR(out, 1.25, 1e-12); // M2 = 5
    EXPECT_EQ(moments.skewness(out), status::ok);
    EXPECT_NEAR(out, 0.0, 1e-12);
    EXPECT_EQ(moments.excess_kurtosis(out), status::ok);
    EXPECT_NEAR(out, 4.0 * 10.25 / 25.0 - 3.0, 1e-12); // M4 = 10.25 -> -1.36
}

TEST(CentralMomentsTest, MergeCombinesPartitions)
{
    castle::statistic::central_moments<double, 4U> left;
    castle::statistic::central_moments<double, 4U> right;
    castle::statistic::central_moments<double, 4U> empty_other;
    EXPECT_EQ(left.push(1.0), status::ok);
    EXPECT_EQ(left.push(2.0), status::ok);
    EXPECT_EQ(left.push(3.0), status::ok);
    EXPECT_EQ(right.push(4.0), status::ok);
    EXPECT_EQ(right.push(10.0), status::ok);

    EXPECT_EQ(left.merge(empty_other), status::ok); // other empty: no change
    EXPECT_EQ(left.count(), 3U);

    EXPECT_EQ(left.merge(right), status::ok);
    EXPECT_EQ(left.count(), 5U);
    double out = 0.0;
    EXPECT_EQ(left.mean(out), status::ok);
    EXPECT_NEAR(out, 4.0, 1e-12);
    EXPECT_EQ(left.population_variance(out), status::ok);
    EXPECT_NEAR(out, 10.0, 1e-9);
    EXPECT_EQ(left.skewness(out), status::ok);
    EXPECT_NEAR(out, kSkewness, 1e-9);
    EXPECT_EQ(left.excess_kurtosis(out), status::ok);
    EXPECT_NEAR(out, -0.212, 1e-9);

    castle::statistic::central_moments<double, 4U> blank;
    EXPECT_EQ(blank.merge(right), status::ok); // this empty: copy
    EXPECT_EQ(blank.count(), 2U);
    EXPECT_EQ(blank.mean(out), status::ok);
    EXPECT_NEAR(out, 7.0, 1e-12);
}

TEST(CentralMomentsTest, SelfMergeDoublesCount)
{
    castle::statistic::central_moments<double, 4U> moments;
    EXPECT_EQ(moments.push(1.0), status::ok);
    EXPECT_EQ(moments.push(2.0), status::ok);
    EXPECT_EQ(moments.push(3.0), status::ok);
    EXPECT_EQ(moments.merge(moments), status::ok); // {1,1,2,2,3,3}
    EXPECT_EQ(moments.count(), 6U);
    double out = 0.0;
    EXPECT_EQ(moments.mean(out), status::ok);
    EXPECT_NEAR(out, 2.0, 1e-12);
    EXPECT_EQ(moments.population_variance(out), status::ok);
    EXPECT_NEAR(out, 4.0 / 6.0, 1e-12); // M2 = 4
    EXPECT_EQ(moments.skewness(out), status::ok);
    EXPECT_NEAR(out, 0.0, 1e-12);
    EXPECT_EQ(moments.excess_kurtosis(out), status::ok);
    EXPECT_NEAR(out, 6.0 * 4.0 / 16.0 - 3.0, 1e-12); // M4 = 4 -> -1.5
}

TEST(CentralMomentsTest, CounterExhaustionReportsFull)
{
    castle::statistic::central_moments<double, 2U> sink;
    castle::statistic::central_moments<double, 2U> doubled;
    reach_max_count(sink, doubled);

    EXPECT_EQ(sink.push(1.0), status::full);
    EXPECT_EQ(sink.merge(doubled), status::full);
    EXPECT_EQ(sink.count(), std::numeric_limits<castle::size_type>::max());
    double out = 0.0;
    EXPECT_EQ(sink.mean(out), status::ok);
    EXPECT_NEAR(out, 1.0, 1e-12);
}

TEST(CentralMomentsTest, ClearResetsEverything)
{
    castle::statistic::central_moments<double, 4U> moments;
    fill_reference(moments);
    moments.clear();
    EXPECT_TRUE(moments.empty());
    EXPECT_EQ(moments.sum(), 0.0);
    EXPECT_EQ(moments.push(2.0), status::ok);
    EXPECT_EQ(moments.push(2.0), status::ok);
    double out = 1.0;
    EXPECT_EQ(moments.population_variance(out), status::ok);
    EXPECT_EQ(out, 0.0);
}

TEST(CentralMomentsTest, FloatInstantiation)
{
    castle::statistic::central_moments<float, 4U> moments;
    fill_reference(moments);
    float out = 0.0F;
    EXPECT_EQ(moments.population_variance(out), status::ok);
    EXPECT_NEAR(out, 10.0F, 1e-4F);
    EXPECT_EQ(moments.skewness(out), status::ok);
    EXPECT_NEAR(out, static_cast<float>(kSkewness), 1e-4F);
}

TEST(RunningMomentsTest, EmptyState)
{
    castle::statistic::running_moments<double, 4U> stats;
    EXPECT_TRUE(stats.empty());
    EXPECT_EQ(stats.count(), 0U);
    double out = 9.0;
    EXPECT_EQ(stats.minimum(out), status::empty);
    EXPECT_EQ(stats.maximum(out), status::empty);
    EXPECT_EQ(stats.range(out), status::empty);
    EXPECT_EQ(stats.mean(out), status::empty);
    EXPECT_EQ(out, 9.0);
}

TEST(RunningMomentsTest, PushTracksStatisticsAndExtremes)
{
    castle::statistic::running_moments<double, 4U> stats;
    EXPECT_EQ(stats.push(5.0), status::ok); // first sample sets both extremes
    double out = 0.0;
    EXPECT_EQ(stats.minimum(out), status::ok);
    EXPECT_EQ(out, 5.0);
    EXPECT_EQ(stats.maximum(out), status::ok);
    EXPECT_EQ(out, 5.0);
    EXPECT_EQ(stats.range(out), status::ok);
    EXPECT_EQ(out, 0.0);

    EXPECT_EQ(stats.push(-2.0), status::ok); // new minimum
    EXPECT_EQ(stats.push(9.0), status::ok);  // new maximum
    EXPECT_EQ(stats.push(3.0), status::ok);  // inside the range
    EXPECT_EQ(stats.minimum(out), status::ok);
    EXPECT_EQ(out, -2.0);
    EXPECT_EQ(stats.maximum(out), status::ok);
    EXPECT_EQ(out, 9.0);
    EXPECT_EQ(stats.range(out), status::ok);
    EXPECT_EQ(out, 11.0);
    EXPECT_EQ(stats.count(), 4U);
    EXPECT_NEAR(stats.sum(), 15.0, 1e-12);
}

TEST(RunningMomentsTest, PushRejectsNonFiniteSamples)
{
    castle::statistic::running_moments<double, 2U> stats;
    EXPECT_EQ(stats.push(1.0), status::ok);
    EXPECT_EQ(stats.push(std::numeric_limits<double>::quiet_NaN()), status::invalid_argument);
    EXPECT_EQ(stats.push(std::numeric_limits<double>::infinity()), status::invalid_argument);
    EXPECT_EQ(stats.push(-std::numeric_limits<double>::infinity()), status::invalid_argument);
    EXPECT_EQ(stats.count(), 1U);
    double out = 0.0;
    EXPECT_EQ(stats.maximum(out), status::ok);
    EXPECT_EQ(out, 1.0);
}

TEST(RunningMomentsTest, ReferenceMomentsThroughWrapper)
{
    castle::statistic::running_moments<double, 4U> stats;
    const double samples[] = {1.0, 2.0, 3.0, 4.0, 10.0};
    EXPECT_EQ(stats.push_range(samples, samples + 5), status::ok);
    EXPECT_EQ(stats.count(), 5U);
    double out = 0.0;
    EXPECT_EQ(stats.mean(out), status::ok);
    EXPECT_NEAR(out, 4.0, 1e-12);
    EXPECT_NEAR(stats.sum_squares(), 130.0, 1e-9);
    EXPECT_EQ(stats.rms(out), status::ok);
    EXPECT_NEAR(out, sqrt(26.0), 1e-9);
    EXPECT_EQ(stats.population_variance(out), status::ok);
    EXPECT_NEAR(out, 10.0, 1e-9);
    EXPECT_EQ(stats.sample_variance(out), status::ok);
    EXPECT_NEAR(out, 12.5, 1e-9);
    EXPECT_EQ(stats.population_stddev(out), status::ok);
    EXPECT_NEAR(out, sqrt(10.0), 1e-9);
    EXPECT_EQ(stats.sample_stddev(out), status::ok);
    EXPECT_NEAR(out, sqrt(12.5), 1e-9);
    EXPECT_EQ(stats.skewness(out), status::ok);
    EXPECT_NEAR(out, kSkewness, 1e-9);
    EXPECT_EQ(stats.excess_kurtosis(out), status::ok);
    EXPECT_NEAR(out, -0.212, 1e-9);
}

TEST(RunningMomentsTest, PushRangeStopsAtFirstFailure)
{
    castle::statistic::running_moments<double, 2U> stats;
    EXPECT_EQ(stats.push_range(static_cast<const double*>(nullptr), static_cast<const double*>(nullptr)),
              status::ok); // empty range
    const double samples[] = {1.0, 2.0, std::numeric_limits<double>::quiet_NaN(), 4.0};
    EXPECT_EQ(stats.push_range(samples, samples + 4), status::invalid_argument);
    EXPECT_EQ(stats.count(), 2U); // samples before the failure remain
    double out = 0.0;
    EXPECT_EQ(stats.mean(out), status::ok);
    EXPECT_NEAR(out, 1.5, 1e-12);
}

TEST(RunningMomentsTest, MergeCombinesExtremes)
{
    castle::statistic::running_moments<double, 2U> left;
    castle::statistic::running_moments<double, 2U> right;
    castle::statistic::running_moments<double, 2U> none;
    EXPECT_EQ(left.push(5.0), status::ok);
    EXPECT_EQ(left.push(1.0), status::ok);
    EXPECT_EQ(right.push(9.0), status::ok);
    EXPECT_EQ(right.push(-2.0), status::ok);

    EXPECT_EQ(left.merge(none), status::ok); // other empty
    EXPECT_EQ(left.count(), 2U);

    EXPECT_EQ(left.merge(right), status::ok);
    EXPECT_EQ(left.count(), 4U);
    double out = 0.0;
    EXPECT_EQ(left.minimum(out), status::ok);
    EXPECT_EQ(out, -2.0);
    EXPECT_EQ(left.maximum(out), status::ok);
    EXPECT_EQ(out, 9.0);
    EXPECT_EQ(left.mean(out), status::ok);
    EXPECT_NEAR(out, 3.25, 1e-12); // (5 + 1 + 9 - 2) / 4

    castle::statistic::running_moments<double, 2U> blank;
    EXPECT_EQ(blank.merge(right), status::ok); // this empty
    EXPECT_EQ(blank.minimum(out), status::ok);
    EXPECT_EQ(out, -2.0);
    EXPECT_EQ(blank.maximum(out), status::ok);
    EXPECT_EQ(out, 9.0);

    // Merging a set that lies inside the existing range keeps the extremes.
    castle::statistic::running_moments<double, 2U> inner;
    EXPECT_EQ(inner.push(0.0), status::ok);
    EXPECT_EQ(left.merge(inner), status::ok);
    EXPECT_EQ(left.minimum(out), status::ok);
    EXPECT_EQ(out, -2.0);
    EXPECT_EQ(left.maximum(out), status::ok);
    EXPECT_EQ(out, 9.0);
}

TEST(RunningMomentsTest, CounterExhaustionPropagates)
{
    castle::statistic::running_moments<double, 2U> sink;
    castle::statistic::running_moments<double, 2U> doubled;
    reach_max_count(sink, doubled);

    EXPECT_EQ(sink.push(1.0), status::full);
    EXPECT_EQ(sink.merge(doubled), status::full);
    double out = 0.0;
    EXPECT_EQ(sink.minimum(out), status::ok);
    EXPECT_EQ(out, 1.0);
}

TEST(RunningMomentsTest, ClearResetsExtremes)
{
    castle::statistic::running_moments<double, 3U> stats;
    EXPECT_EQ(stats.push(4.0), status::ok);
    stats.clear();
    EXPECT_TRUE(stats.empty());
    double out = 0.0;
    EXPECT_EQ(stats.minimum(out), status::empty);
    EXPECT_EQ(stats.push(-8.0), status::ok);
    EXPECT_EQ(stats.maximum(out), status::ok);
    EXPECT_EQ(out, -8.0); // not polluted by the earlier 4.0
}

TEST(RunningMomentsTest, IntegralSamples)
{
    castle::statistic::running_moments<int, 4U> stats;
    static_assert(std::is_same<decltype(stats)::result_type, double>::value, "int reports in double");
    const int samples[] = {1, 2, 3, 4, 10};
    EXPECT_EQ(stats.push_range(samples, samples + 5), status::ok);
    double out = 0.0;
    EXPECT_EQ(stats.mean(out), status::ok);
    EXPECT_NEAR(out, 4.0, 1e-12);
    EXPECT_EQ(stats.sample_variance(out), status::ok);
    EXPECT_NEAR(out, 12.5, 1e-9);
    EXPECT_EQ(stats.skewness(out), status::ok);
    EXPECT_NEAR(out, kSkewness, 1e-9);
    EXPECT_EQ(stats.minimum(out), status::ok);
    EXPECT_EQ(out, 1.0);
    EXPECT_EQ(stats.maximum(out), status::ok);
    EXPECT_EQ(out, 10.0);
    EXPECT_EQ(stats.push(-3), status::ok);
    EXPECT_EQ(stats.range(out), status::ok);
    EXPECT_EQ(out, 13.0);
}

TEST(RunningMomentsTest, FloatSamples)
{
    castle::statistic::running_moments<float, 3U> stats;
    static_assert(std::is_same<decltype(stats)::result_type, float>::value, "float stays float");
    EXPECT_EQ(stats.push(2.0F), status::ok);
    EXPECT_EQ(stats.push(4.0F), status::ok);
    float out = 0.0F;
    EXPECT_EQ(stats.mean(out), status::ok);
    EXPECT_NEAR(out, 3.0F, 1e-6F);
    EXPECT_EQ(stats.push(std::numeric_limits<float>::infinity()), status::invalid_argument);
}
} // namespace
