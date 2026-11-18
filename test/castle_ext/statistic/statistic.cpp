#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <type_traits>
#include <stdint.h>

#include "castle_ext/statistic/statistic.hpp"

namespace
{
using namespace castle::statistic;

// Deterministic xorshift32 stream in [0, 1).
class generator
{
public:
    double next()
    {
        state_ ^= state_ << 13U;
        state_ ^= state_ >> 17U;
        state_ ^= state_ << 5U;
        return static_cast<double>(state_ >> 8U) / 16777216.0;
    }

private:
    uint32_t state_ = 2463534242U;
};

const unsigned kFixes = 150U;
const unsigned kWindow = 100U;

TEST(StatisticUmbrellaTest, GnssPipelineEndToEnd)
{
    generator rng;
    double radius[kFixes];

    exact_cep<double, kWindow> horizontal;
    running_moments<double> all_fixes;
    sliding_moments<double, kWindow> recent;
    histogram<double, 10U> bins(0.0, 4.0);
    p2_quantile<double> streaming_median(0.5);
    ewma<double> smoothed(0.2);
    online_regression<double> calibration;

    double smooth_ref = 0.0;
    for (unsigned i = 0U; i < kFixes; ++i)
    {
        const double east = (rng.next() - 0.5) * 4.0;
        const double north = (rng.next() - 0.5) * 4.0;
        const double r = std::hypot(east, north);
        radius[i] = r;

        EXPECT_EQ(horizontal.push_residual(east, north), castle::status::ok);
        EXPECT_EQ(all_fixes.push(r), castle::status::ok);
        EXPECT_EQ(recent.push(r), castle::status::ok);
        EXPECT_EQ(bins.push(r), castle::status::ok);
        EXPECT_EQ(streaming_median.push(r), castle::status::ok);
        EXPECT_EQ(smoothed.push(r), castle::status::ok);
        // Reported accuracy = true radius + 0.5 m of pessimism.
        EXPECT_EQ(calibration.push(r, r + 0.5), castle::status::ok);
        smooth_ref = (i == 0U) ? r : smooth_ref + 0.2 * (r - smooth_ref);
    }

    // Reference: the newest 100 radii, sorted.
    double window[kWindow];
    for (unsigned i = 0U; i < kWindow; ++i)
    {
        window[i] = radius[kFixes - kWindow + i];
    }
    double ordered[kWindow];
    std::copy(window, window + kWindow, ordered);
    std::sort(ordered, ordered + kWindow);

    // exact_cep: linear quantile at p * 99.
    EXPECT_EQ(horizontal.count(), kWindow);
    double cep50 = 0.0;
    double cep95 = 0.0;
    EXPECT_EQ(horizontal.cep50(cep50), castle::status::ok);
    EXPECT_EQ(horizontal.cep95(cep95), castle::status::ok);
    EXPECT_NEAR(cep50, 0.5 * (ordered[49] + ordered[50]), 1e-9);
    EXPECT_NEAR(cep95, ordered[94] + 0.05 * (ordered[95] - ordered[94]), 1e-9);
    EXPECT_LT(cep50, cep95);
    double inside = 0.0;
    EXPECT_EQ(horizontal.fraction_within(cep95, inside), castle::status::ok);
    EXPECT_GE(inside, 0.9);

    // running_moments over all 150 fixes.
    double sum = 0.0;
    for (unsigned i = 0U; i < kFixes; ++i)
    {
        sum += radius[i];
    }
    double mean = 0.0;
    EXPECT_EQ(all_fixes.mean(mean), castle::status::ok);
    EXPECT_NEAR(mean, sum / static_cast<double>(kFixes), 1e-9);
    EXPECT_EQ(all_fixes.count(), kFixes);

    // sliding_moments over the newest 100.
    double window_sum = 0.0;
    for (unsigned i = 0U; i < kWindow; ++i)
    {
        window_sum += window[i];
    }
    double window_mean = 0.0;
    EXPECT_EQ(recent.mean(window_mean), castle::status::ok);
    EXPECT_NEAR(window_mean, window_sum / static_cast<double>(kWindow), 1e-9);
    EXPECT_EQ(recent.count(), kWindow);

    // histogram: every radius is below 2 * sqrt(2) < 4.
    EXPECT_EQ(bins.count(), kFixes);
    EXPECT_EQ(bins.underflow(), 0U);
    EXPECT_EQ(bins.overflow(), 0U);

    // p2_quantile: streaming estimate close to the exact median of all fixes.
    double all_sorted[kFixes];
    std::copy(radius, radius + kFixes, all_sorted);
    std::sort(all_sorted, all_sorted + kFixes);
    const double exact_median = 0.5 * (all_sorted[74] + all_sorted[75]);
    double estimate = 0.0;
    EXPECT_EQ(streaming_median.quantile(estimate), castle::status::ok);
    EXPECT_NEAR(estimate, exact_median, 0.25);

    // ewma: recursion y += alpha * (x - y).
    double filtered = 0.0;
    EXPECT_EQ(smoothed.mean(filtered), castle::status::ok);
    EXPECT_NEAR(filtered, smooth_ref, 1e-9);

    // online_regression: y = x + 0.5 exactly.
    double slope = 0.0;
    double intercept = 0.0;
    EXPECT_EQ(calibration.fit(slope, intercept), castle::status::ok);
    EXPECT_NEAR(slope, 1.0, 1e-9);
    EXPECT_NEAR(intercept, 0.5, 1e-9);
    double r_xy = 0.0;
    EXPECT_EQ(calibration.correlation(r_xy), castle::status::ok);
    EXPECT_NEAR(r_xy, 1.0, 1e-9);

    // kernel_density over the sorted window view.
    const auto view = horizontal.estimator().sorted();
    double bandwidth = 0.0;
    EXPECT_EQ(normal_reference_bandwidth(view.begin(), view.end(), bandwidth), castle::status::ok);
    EXPECT_GT(bandwidth, 0.0);
    double near_median = 0.0;
    double far_away = 1.0;
    EXPECT_EQ(kernel_density(view.begin(), view.end(), cep50, bandwidth, near_median), castle::status::ok);
    EXPECT_EQ(kernel_density(view.begin(), view.end(), 50.0, bandwidth, far_away), castle::status::ok);
    EXPECT_GT(near_median, 0.0);
    EXPECT_LT(far_away, 1e-12);

    // order statistics on a caller-owned copy.
    double scratch[kWindow];
    std::copy(window, window + kWindow, scratch);
    double median = 0.0;
    EXPECT_EQ(quantile_inplace(scratch, scratch + kWindow, 0.5, median), castle::status::ok);
    EXPECT_NEAR(median, cep50, 1e-12);
    EXPECT_EQ(median_sorted(scratch, scratch + kWindow, median), castle::status::ok);
    EXPECT_NEAR(median, cep50, 1e-12);
    const int discrete[] = {1, 2, 2, 3};
    double mode = 0.0;
    EXPECT_EQ(mode_sorted(discrete, discrete + 4, mode), castle::status::ok);
    EXPECT_DOUBLE_EQ(mode, 2.0);
}

TEST(StatisticUmbrellaTest, RemainingComponentsAreReachable)
{
    // validation
    static_assert(std::is_same<result_t<int>, double>::value, "integral results are double");
    EXPECT_TRUE(is_finite(1.0));
    EXPECT_TRUE(is_probability(0.5));

    // sample_window + sliding_histogram + sorted_window
    sample_window<double, 3U> samples;
    EXPECT_FALSE(samples.push(1.0));
    sorted_window<double, 3U> sorted;
    EXPECT_EQ(sorted.push(2.0), castle::status::ok);
    sliding_histogram<double, 4U, 4U> sliding(0.0, 4.0);
    EXPECT_EQ(sliding.push(1.0), castle::status::ok);
    EXPECT_EQ(sliding.count(), 1U);

    // cumulative_cep grows its range on demand.
    cumulative_cep<double, 8U> cumulative(0.0, 1.0);
    EXPECT_EQ(cumulative.push_distance(5.0), castle::status::ok);
    EXPECT_FALSE(cumulative.empty());
}
} // namespace
