#include <gtest/gtest.h>

#include <limits>
#include <stdint.h>

#include "castle_ext/statistic/cep.hpp"
#include "castle_ext/statistic/order_statistics.hpp"

namespace
{
using castle::status;
using exact100 = castle::statistic::exact_cep<double, 100U>;

constexpr double nan_value = std::numeric_limits<double>::quiet_NaN();
constexpr double inf_value = std::numeric_limits<double>::infinity();

// Fills the tracker with the distances 1, 2, ... count.
void fill_ramp(exact100& tracker, int count)
{
    for (int i = 1; i <= count; ++i)
    {
        EXPECT_EQ(tracker.push_distance(static_cast<double>(i)), status::ok);
    }
}

TEST(CepTest, EmptyReportsEmpty)
{
    exact100 tracker;
    double out = -1.0;
    EXPECT_TRUE(tracker.empty());
    EXPECT_EQ(tracker.count(), 0U);
    EXPECT_EQ(tracker.cep95(out), status::empty);
    EXPECT_EQ(tracker.cep50(out), status::empty);
    EXPECT_EQ(tracker.quantile(0.5, out), status::empty);
    EXPECT_EQ(tracker.percentile(50.0, out), status::empty);
    EXPECT_EQ(tracker.fraction_within(1.0, out), status::empty);
    EXPECT_EQ(out, -1.0);
}

TEST(CepTest, PushDistanceValidatesFloatingPoint)
{
    exact100 tracker;
    EXPECT_EQ(tracker.push_distance(-1.0), status::invalid_argument);
    EXPECT_EQ(tracker.push_distance(nan_value), status::invalid_argument);
    EXPECT_EQ(tracker.push_distance(inf_value), status::invalid_argument);
    EXPECT_EQ(tracker.push_distance(-inf_value), status::invalid_argument);
    EXPECT_TRUE(tracker.empty());

    EXPECT_EQ(tracker.push_distance(0.0), status::ok);
    EXPECT_EQ(tracker.push_distance(2.5), status::ok);
    EXPECT_EQ(tracker.count(), 2U);
}

TEST(CepTest, PushDistanceValidatesSignedIntegral)
{
    castle::statistic::cep<castle::statistic::sorted_window<int, 4U>> tracker;
    double out = 0.0;
    EXPECT_EQ(tracker.push_distance(-1), status::invalid_argument);
    EXPECT_TRUE(tracker.empty());
    EXPECT_EQ(tracker.push_distance(0), status::ok);
    EXPECT_EQ(tracker.push_distance(2), status::ok);
    EXPECT_EQ(tracker.push_distance(4), status::ok);
    EXPECT_EQ(tracker.count(), 3U);
    EXPECT_EQ(tracker.cep50(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.0); // median of 0, 2, 4
}

TEST(CepTest, PushDistanceAcceptsAnyUnsigned)
{
    castle::statistic::exact_cep<uint32_t, 4U> tracker;
    double out = 0.0;
    EXPECT_EQ(tracker.push_distance(10U), status::ok);
    EXPECT_EQ(tracker.push_distance(20U), status::ok);
    EXPECT_EQ(tracker.push_distance(30U), status::ok);
    EXPECT_EQ(tracker.cep50(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 20.0);
}

TEST(CepTest, PushResidualTwoDimensional)
{
    exact100 tracker;
    double out = 0.0;
    EXPECT_EQ(tracker.push_residual(3.0, 4.0), status::ok);
    EXPECT_EQ(tracker.cep50(out), status::ok);
    EXPECT_NEAR(out, 5.0, 1e-12); // hypot(3, 4)

    EXPECT_EQ(tracker.push_residual(nan_value, 1.0), status::invalid_argument);
    EXPECT_EQ(tracker.push_residual(1.0, nan_value), status::invalid_argument);
    EXPECT_EQ(tracker.push_residual(inf_value, 1.0), status::invalid_argument);
    EXPECT_EQ(tracker.push_residual(1.0, -inf_value), status::invalid_argument);
    EXPECT_EQ(tracker.count(), 1U);
}

TEST(CepTest, PushResidualThreeDimensional)
{
    exact100 tracker;
    double out = 0.0;
    EXPECT_EQ(tracker.push_residual(2.0, 3.0, 6.0), status::ok);
    EXPECT_EQ(tracker.cep50(out), status::ok);
    EXPECT_NEAR(out, 7.0, 1e-12); // hypot(2, 3, 6)

    EXPECT_EQ(tracker.push_residual(nan_value, 1.0, 1.0), status::invalid_argument);
    EXPECT_EQ(tracker.push_residual(1.0, nan_value, 1.0), status::invalid_argument);
    EXPECT_EQ(tracker.push_residual(1.0, 1.0, nan_value), status::invalid_argument);
    EXPECT_EQ(tracker.push_residual(inf_value, 1.0, 1.0), status::invalid_argument);
    EXPECT_EQ(tracker.push_residual(1.0, -inf_value, 1.0), status::invalid_argument);
    EXPECT_EQ(tracker.push_residual(1.0, 1.0, inf_value), status::invalid_argument);
    EXPECT_EQ(tracker.count(), 1U);
}

TEST(CepTest, ExactCepValuesForRamp)
{
    exact100 tracker;
    fill_ramp(tracker, 100);
    EXPECT_EQ(tracker.count(), 100U);
    EXPECT_FALSE(tracker.empty());

    double out = 0.0;
    // Linear quantile at p * 99 of the values 1..100.
    EXPECT_EQ(tracker.cep50(out), status::ok);
    EXPECT_NEAR(out, 50.5, 1e-9);
    EXPECT_EQ(tracker.cep68(out), status::ok);
    EXPECT_NEAR(out, 68.32, 1e-9);
    EXPECT_EQ(tracker.cep90(out), status::ok);
    EXPECT_NEAR(out, 90.1, 1e-9);
    EXPECT_EQ(tracker.cep95(out), status::ok);
    EXPECT_NEAR(out, 95.05, 1e-9);
    EXPECT_EQ(tracker.cep99(out), status::ok);
    EXPECT_NEAR(out, 99.01, 1e-9);
}

TEST(CepTest, QuantileAndPercentile)
{
    exact100 tracker;
    fill_ramp(tracker, 100);
    double out = 0.0;
    double reference = 0.0;

    EXPECT_EQ(tracker.quantile(0.5, out), status::ok);
    EXPECT_NEAR(out, 50.5, 1e-9);
    EXPECT_EQ(tracker.quantile(1.5, out), status::invalid_argument);

    EXPECT_EQ(tracker.percentile(-1.0, out), status::invalid_argument);
    EXPECT_EQ(tracker.percentile(100.5, out), status::invalid_argument);
    EXPECT_EQ(tracker.percentile(nan_value, out), status::invalid_argument);

    EXPECT_EQ(tracker.percentile(95.0, out), status::ok);
    EXPECT_EQ(tracker.cep95(reference), status::ok);
    EXPECT_NEAR(out, reference, 1e-9);

    EXPECT_EQ(tracker.percentile(0.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);
    EXPECT_EQ(tracker.percentile(100.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 100.0);
}

TEST(CepTest, FractionWithin)
{
    exact100 tracker;
    fill_ramp(tracker, 100);
    double out = -1.0;
    EXPECT_EQ(tracker.fraction_within(50.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.5); // 50 of 100 errors are <= 50
    EXPECT_EQ(tracker.fraction_within(0.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);
    EXPECT_EQ(tracker.fraction_within(1000.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);
    EXPECT_EQ(tracker.fraction_within(nan_value, out), status::invalid_argument);
}

TEST(CepTest, EvictsOldestErrors)
{
    exact100 tracker;
    fill_ramp(tracker, 150); // window keeps 51..150
    EXPECT_EQ(tracker.count(), 100U);

    double out = 0.0;
    EXPECT_EQ(tracker.cep50(out), status::ok);
    EXPECT_NEAR(out, 100.5, 1e-9); // 51 + 49.5
    EXPECT_EQ(tracker.cep95(out), status::ok);
    EXPECT_NEAR(out, 145.05, 1e-9); // 51 + 94.05
    EXPECT_EQ(tracker.fraction_within(100.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.5); // 51..100
}

TEST(CepTest, ClearDiscardsErrors)
{
    exact100 tracker;
    fill_ramp(tracker, 10);
    tracker.clear();
    EXPECT_TRUE(tracker.empty());
    EXPECT_EQ(tracker.count(), 0U);
    double out = 0.0;
    EXPECT_EQ(tracker.cep50(out), status::empty);
}

TEST(CepTest, EstimatorAccess)
{
    exact100 tracker;
    fill_ramp(tracker, 100);
    double out = 0.0;

    // Non-const access: nearest-rank convention, ceil(0.9 * 100) - 1 = index 89 -> 90.
    EXPECT_EQ(tracker.estimator().quantile(0.9, out, castle::statistic::quantile_method::nearest_rank),
              status::ok);
    EXPECT_DOUBLE_EQ(out, 90.0);

    // Const access.
    const exact100& view = tracker;
    EXPECT_EQ(view.estimator().count(), 100U);
    EXPECT_EQ(view.estimator().median(out), status::ok);
    EXPECT_NEAR(out, 50.5, 1e-9);
}

TEST(CepTest, SlidingHistogramCep)
{
    // Range [0, 100) with 100 bins; distances 0.5, 1.5, ... 99.5 give one sample per bin.
    castle::statistic::sliding_histogram_cep<double, 100U, 100U> tracker(0.0, 100.0);
    EXPECT_TRUE(tracker.estimator().configured());
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(tracker.push_distance(static_cast<double>(i) + 0.5), status::ok);
    }
    EXPECT_EQ(tracker.count(), 100U);

    double out = 0.0;
    EXPECT_EQ(tracker.cep50(out), status::ok);
    EXPECT_NEAR(out, 50.0, 1e-6);
    EXPECT_EQ(tracker.cep68(out), status::ok);
    EXPECT_NEAR(out, 68.0, 1e-6);
    EXPECT_EQ(tracker.cep90(out), status::ok);
    EXPECT_NEAR(out, 90.0, 1e-6);
    EXPECT_EQ(tracker.cep95(out), status::ok);
    EXPECT_NEAR(out, 95.0, 1e-6);
    EXPECT_EQ(tracker.cep99(out), status::ok);
    EXPECT_NEAR(out, 99.0, 1e-6);
    EXPECT_EQ(tracker.fraction_within(50.0, out), status::ok);
    EXPECT_NEAR(out, 0.5, 1e-9);

    // 100 more errors of 90.5 evict the old ones: the window holds only 90.5.
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(tracker.push_distance(90.5), status::ok);
    }
    EXPECT_EQ(tracker.count(), 100U);
    EXPECT_EQ(tracker.cep50(out), status::ok);
    EXPECT_NEAR(out, 90.5, 1e-9); // half way through bin [90, 91)

    EXPECT_EQ(tracker.push_residual(3.0, 4.0), status::ok);
    EXPECT_EQ(tracker.push_residual(2.0, 3.0, 6.0), status::ok);
    EXPECT_EQ(tracker.push_distance(-1.0), status::invalid_argument);

    tracker.clear();
    EXPECT_TRUE(tracker.empty());
}

TEST(CepTest, SlidingHistogramCepUnconfiguredPropagatesStatus)
{
    castle::statistic::sliding_histogram_cep<double, 8U, 4U> tracker;
    EXPECT_EQ(tracker.push_distance(1.0), status::not_configured);
    EXPECT_EQ(tracker.estimator().configure(0.0, 8.0), status::ok);
    EXPECT_EQ(tracker.push_distance(1.0), status::ok);
}

TEST(CepTest, CumulativeCepExpandsRange)
{
    castle::statistic::cumulative_cep<double, 8U> tracker(0.0, 8.0);
    EXPECT_EQ(tracker.push_distance(1.0), status::ok);
    EXPECT_EQ(tracker.push_distance(2.0), status::ok);
    EXPECT_EQ(tracker.push_distance(20.0), status::ok); // range grows to [0, 32)
    EXPECT_EQ(tracker.count(), 3U);

    double out = 0.0;
    // Bin 0 [0, 4) holds 1 and 2, bin 5 [20, 24) holds 20.
    EXPECT_EQ(tracker.cep50(out), status::ok);
    EXPECT_NEAR(out, 3.0, 1e-9); // target 1.5 of weight 2: 0.75 * 4
    EXPECT_EQ(tracker.cep95(out), status::ok);
    EXPECT_NEAR(out, 23.4, 1e-9); // target 2.85: (5 + 0.85) * 4
    EXPECT_EQ(tracker.fraction_within(1000.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);
    EXPECT_EQ(tracker.fraction_within(-1.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);
    EXPECT_EQ(tracker.push_distance(-1.0), status::invalid_argument);
    EXPECT_EQ(tracker.push_distance(inf_value), status::invalid_argument);
}

TEST(CepTest, FloatValueType)
{
    castle::statistic::exact_cep<float, 8U> tracker;
    float out = 0.0f;
    EXPECT_EQ(tracker.push_residual(3.0f, 4.0f), status::ok);
    EXPECT_EQ(tracker.push_residual(0.0f, 0.0f), status::ok);
    EXPECT_EQ(tracker.cep50(out), status::ok);
    EXPECT_NEAR(out, 2.5f, 1e-5f); // median of 0 and 5
}
} // namespace
