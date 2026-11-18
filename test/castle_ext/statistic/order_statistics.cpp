#include <gtest/gtest.h>

#include <limits>
#include <stdint.h>

#include "castle_ext/statistic/order_statistics.hpp"

namespace
{
using castle::status;
using castle::statistic::quantile_method;

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

TEST(OrderStatisticsTest, NearestRankIndexBoundaries)
{
    using castle::statistic::detail::nearest_rank_index;

    // p = 0 -> position 0 -> rank 0 -> index 0 (the "rank == 0" arm).
    EXPECT_EQ(nearest_rank_index<double>(0.0, 5U), 0U);
    // p = 1 -> position n -> index n - 1.
    EXPECT_EQ(nearest_rank_index<double>(1.0, 5U), 4U);
    EXPECT_EQ(nearest_rank_index<double>(1.0, 1U), 0U);
    // 0.95 * 100 is 95 (up to rounding): ceil(95) - 1 = 94, not 95.
    EXPECT_EQ(nearest_rank_index<double>(0.95, 100U), 94U);
    // 0.07 * 100 is 7.000000000000001 in double; the tolerance keeps index 6.
    EXPECT_EQ(nearest_rank_index<double>(0.07, 100U), 6U);
    // Fractional ranks round up: 0.5 * 5 = 2.5 -> rank 3 -> index 2.
    EXPECT_EQ(nearest_rank_index<double>(0.5, 5U), 2U);
    // 0.1 * 5 = 0.5 -> rank 1 -> index 0.
    EXPECT_EQ(nearest_rank_index<double>(0.1, 5U), 0U);
    // Exact integer position: 0.5 * 4 = 2 -> index 1.
    EXPECT_EQ(nearest_rank_index<double>(0.5, 4U), 1U);
    // float result type.
    EXPECT_EQ(nearest_rank_index<float>(0.75f, 8U), 5U);
    EXPECT_EQ(nearest_rank_index<float>(0.8f, 3U), 2U);
}

TEST(OrderStatisticsTest, QuantileSortedInvalidProbability)
{
    const double data[] = {0.5, 1.0, 1.5, 2.0};
    double out = -7.0;
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, kNaN, out), status::invalid_argument);
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, -0.1, out), status::invalid_argument);
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 1.1, out), status::invalid_argument);
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 1.1, out, quantile_method::nearest_rank),
              status::invalid_argument);
    EXPECT_EQ(out, -7.0);
}

TEST(OrderStatisticsTest, QuantileSortedEmptyRange)
{
    const double data[] = {1.0};
    double out = -7.0;
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data, 0.5, out), status::empty);
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data, 0.5, out, quantile_method::nearest_rank),
              status::empty);
    EXPECT_EQ(out, -7.0);
    // An invalid probability is reported before emptiness.
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data, 2.0, out), status::invalid_argument);
}

TEST(OrderStatisticsTest, QuantileSortedNearestRank)
{
    const double data[] = {0.5, 1.0, 1.5, 2.0};
    const quantile_method method = quantile_method::nearest_rank;
    double out = 0.0;
    // p * n = 3.8 -> rank 4 -> data[3].
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 0.95, out, method), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.0);
    // p * n = 2 -> rank 2 -> data[1].
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 0.5, out, method), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);
    // p = 0 -> first sample.
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 0.0, out, method), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.5);
    // p = 1 -> last sample.
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 1.0, out, method), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.0);
    // p * n = 1 -> rank 1 -> data[0]; just above -> data[1].
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 0.25, out, method), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.5);
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 0.26, out, method), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);

    // 100 samples 1..100: the 95th percentile is the sample 95 (index 94).
    double hundred[100];
    for (int i = 0; i < 100; ++i)
    {
        hundred[i] = static_cast<double>(i + 1);
    }
    EXPECT_EQ(castle::statistic::quantile_sorted(hundred, hundred + 100, 0.95, out, method), status::ok);
    EXPECT_DOUBLE_EQ(out, 95.0);
    EXPECT_EQ(castle::statistic::quantile_sorted(hundred, hundred + 100, 0.5, out, method), status::ok);
    EXPECT_DOUBLE_EQ(out, 50.0);
}

TEST(OrderStatisticsTest, QuantileSortedLinear)
{
    const double data[] = {0.5, 1.0, 1.5, 2.0};
    double out = 0.0;
    // position 0.95 * 3 = 2.85: 1.5 + 0.5 * 0.85 = 1.925.
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 0.95, out), status::ok);
    EXPECT_NEAR(out, 1.925, 1e-12);
    // position 1.5: 1.0 + 0.5 * 0.5 = 1.25.
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 0.5, out), status::ok);
    EXPECT_NEAR(out, 1.25, 1e-12);
    // p = 0 -> position 0 -> first sample.
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 0.0, out), status::ok);
    EXPECT_NEAR(out, 0.5, 1e-12);
    // p = 1 -> lower + 1 >= count -> last sample.
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 1.0, out), status::ok);
    EXPECT_NEAR(out, 2.0, 1e-12);
    // Explicit linear method behaves like the default.
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 4, 0.95, out, quantile_method::linear), status::ok);
    EXPECT_NEAR(out, 1.925, 1e-12);
}

TEST(OrderStatisticsTest, QuantileSortedSingleElement)
{
    const double data[] = {7.0};
    double out = 0.0;
    // Linear: position 0, lower + 1 >= 1 -> the only sample.
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 1, 0.3, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 7.0);
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 1, 0.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 7.0);
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 1, 1.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 7.0);
    // Nearest rank.
    out = 0.0;
    EXPECT_EQ(castle::statistic::quantile_sorted(data, data + 1, 0.3, out, quantile_method::nearest_rank),
              status::ok);
    EXPECT_DOUBLE_EQ(out, 7.0);
}

TEST(OrderStatisticsTest, QuantileSortedIntegralAndFloatTypes)
{
    const int ints[] = {1, 2, 3, 4, 5};
    double out = 0.0;
    // position 0.25 * 4 = 1 -> 2.
    EXPECT_EQ(castle::statistic::quantile_sorted(ints, ints + 5, 0.25, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.0);
    EXPECT_EQ(castle::statistic::quantile_sorted(ints, ints + 5, 0.9, out, quantile_method::nearest_rank),
              status::ok);
    EXPECT_DOUBLE_EQ(out, 5.0);

    const float floats[] = {1.0f, 2.0f, 3.0f};
    float fout = 0.0f;
    EXPECT_EQ(castle::statistic::quantile_sorted(floats, floats + 3, 0.5f, fout), status::ok);
    EXPECT_FLOAT_EQ(fout, 2.0f);
}

TEST(OrderStatisticsTest, MedianSorted)
{
    const double odd[] = {1.0, 2.0, 3.0};
    const double even[] = {1.0, 2.0, 3.0, 4.0};
    double out = 0.0;
    EXPECT_EQ(castle::statistic::median_sorted(odd, odd + 3, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.0);
    EXPECT_EQ(castle::statistic::median_sorted(even, even + 4, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.5);

    out = -1.0;
    EXPECT_EQ(castle::statistic::median_sorted(even, even, out), status::empty);
    EXPECT_EQ(out, -1.0);
}

TEST(OrderStatisticsTest, InterquartileRangeSorted)
{
    const double data[] = {0.5, 1.0, 1.5, 2.0};
    double out = 0.0;
    // Linear: Q1 = 0.5 + 0.5 * 0.75 = 0.875, Q3 = 1.5 + 0.5 * 0.25 = 1.625.
    EXPECT_EQ(castle::statistic::interquartile_range_sorted(data, data + 4, out), status::ok);
    EXPECT_NEAR(out, 0.75, 1e-12);
    // Nearest rank: Q1 = data[0] = 0.5, Q3 = data[2] = 1.5.
    EXPECT_EQ(castle::statistic::interquartile_range_sorted(data, data + 4, out, quantile_method::nearest_rank),
              status::ok);
    EXPECT_NEAR(out, 1.0, 1e-12);

    const int ints[] = {1, 2, 3, 4, 5};
    EXPECT_EQ(castle::statistic::interquartile_range_sorted(ints, ints + 5, out), status::ok);
    EXPECT_NEAR(out, 2.0, 1e-12);

    out = -1.0;
    EXPECT_EQ(castle::statistic::interquartile_range_sorted(data, data, out), status::empty);
    EXPECT_EQ(out, -1.0);
}

TEST(OrderStatisticsTest, ModeSorted)
{
    double out = -1.0;
    const int none[] = {1};
    EXPECT_EQ(castle::statistic::mode_sorted(none, none, out), status::empty);
    EXPECT_EQ(out, -1.0);

    // Unique values: the smallest wins.
    const int unique[] = {1, 2, 3};
    EXPECT_EQ(castle::statistic::mode_sorted(unique, unique + 3, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);

    // Ties resolve to the smallest value.
    const int ties[] = {1, 1, 2, 2};
    EXPECT_EQ(castle::statistic::mode_sorted(ties, ties + 4, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);

    // Longest run in the middle.
    const int middle[] = {1, 2, 2, 2, 3};
    EXPECT_EQ(castle::statistic::mode_sorted(middle, middle + 5, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.0);

    // Longest run at the end.
    const int tail[] = {1, 2, 2, 3, 3, 3};
    EXPECT_EQ(castle::statistic::mode_sorted(tail, tail + 6, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 3.0);

    // Longest run at the start.
    const double head[] = {0.5, 0.5, 0.5, 1.5, 2.5};
    EXPECT_EQ(castle::statistic::mode_sorted(head, head + 5, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.5);

    // Single element.
    const double one[] = {4.25};
    EXPECT_EQ(castle::statistic::mode_sorted(one, one + 1, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 4.25);
}

TEST(OrderStatisticsTest, SortSamplesFloating)
{
    double ok[] = {3.0, 1.0, 2.0, -4.0, 2.0};
    EXPECT_EQ(castle::statistic::sort_samples(ok, ok + 5), status::ok);
    EXPECT_DOUBLE_EQ(ok[0], -4.0);
    EXPECT_DOUBLE_EQ(ok[1], 1.0);
    EXPECT_DOUBLE_EQ(ok[2], 2.0);
    EXPECT_DOUBLE_EQ(ok[3], 2.0);
    EXPECT_DOUBLE_EQ(ok[4], 3.0);

    // An empty range is valid.
    EXPECT_EQ(castle::statistic::sort_samples(ok, ok), status::ok);

    // NaN: rejected, range untouched.
    double with_nan[] = {3.0, kNaN, 1.0};
    EXPECT_EQ(castle::statistic::sort_samples(with_nan, with_nan + 3), status::invalid_argument);
    EXPECT_DOUBLE_EQ(with_nan[0], 3.0);
    EXPECT_TRUE(with_nan[1] != with_nan[1]);
    EXPECT_DOUBLE_EQ(with_nan[2], 1.0);

    // +inf and -inf: rejected, range untouched.
    double with_inf[] = {3.0, kInf, 1.0};
    EXPECT_EQ(castle::statistic::sort_samples(with_inf, with_inf + 3), status::invalid_argument);
    EXPECT_DOUBLE_EQ(with_inf[0], 3.0);
    EXPECT_DOUBLE_EQ(with_inf[2], 1.0);
    double with_minus_inf[] = {3.0, -kInf, 1.0};
    EXPECT_EQ(castle::statistic::sort_samples(with_minus_inf, with_minus_inf + 3), status::invalid_argument);
    EXPECT_DOUBLE_EQ(with_minus_inf[0], 3.0);

    float floats[] = {2.0f, 1.0f};
    EXPECT_EQ(castle::statistic::sort_samples(floats, floats + 2), status::ok);
    EXPECT_FLOAT_EQ(floats[0], 1.0f);
    EXPECT_FLOAT_EQ(floats[1], 2.0f);
}

TEST(OrderStatisticsTest, SortSamplesIntegral)
{
    int ints[] = {5, -3, 1, 4, 1};
    EXPECT_EQ(castle::statistic::sort_samples(ints, ints + 5), status::ok);
    EXPECT_EQ(ints[0], -3);
    EXPECT_EQ(ints[1], 1);
    EXPECT_EQ(ints[2], 1);
    EXPECT_EQ(ints[3], 4);
    EXPECT_EQ(ints[4], 5);

    uint32_t unsigned_values[] = {9U, 3U, 6U};
    EXPECT_EQ(castle::statistic::sort_samples(unsigned_values, unsigned_values + 3), status::ok);
    EXPECT_EQ(unsigned_values[0], 3U);
    EXPECT_EQ(unsigned_values[1], 6U);
    EXPECT_EQ(unsigned_values[2], 9U);
}

TEST(OrderStatisticsTest, QuantileInplaceInvalidProbabilityKeepsOrder)
{
    double data[] = {3.0, 1.0, 2.0};
    double out = -7.0;
    EXPECT_EQ(castle::statistic::quantile_inplace(data, data + 3, 1.5, out), status::invalid_argument);
    EXPECT_EQ(castle::statistic::quantile_inplace(data, data + 3, -0.5, out), status::invalid_argument);
    EXPECT_EQ(castle::statistic::quantile_inplace(data, data + 3, kNaN, out), status::invalid_argument);
    // The probability is checked before sorting, so the order is untouched.
    EXPECT_DOUBLE_EQ(data[0], 3.0);
    EXPECT_DOUBLE_EQ(data[1], 1.0);
    EXPECT_DOUBLE_EQ(data[2], 2.0);
    EXPECT_EQ(out, -7.0);
}

TEST(OrderStatisticsTest, QuantileInplaceNonFiniteSample)
{
    double data[] = {3.0, kNaN, 2.0};
    double out = -7.0;
    EXPECT_EQ(castle::statistic::quantile_inplace(data, data + 3, 0.5, out), status::invalid_argument);
    EXPECT_DOUBLE_EQ(data[0], 3.0);
    EXPECT_DOUBLE_EQ(data[2], 2.0);
    EXPECT_EQ(out, -7.0);

    double with_inf[] = {3.0, kInf};
    EXPECT_EQ(castle::statistic::quantile_inplace(with_inf, with_inf + 2, 0.5, out), status::invalid_argument);
    EXPECT_EQ(out, -7.0);
}

TEST(OrderStatisticsTest, QuantileInplaceOk)
{
    double errors[] = {2.0, 0.5, 1.5, 1.0};
    double out = 0.0;
    EXPECT_EQ(castle::statistic::quantile_inplace(errors, errors + 4, 0.95, out), status::ok);
    EXPECT_NEAR(out, 1.925, 1e-12);
    // The range is now sorted.
    EXPECT_DOUBLE_EQ(errors[0], 0.5);
    EXPECT_DOUBLE_EQ(errors[1], 1.0);
    EXPECT_DOUBLE_EQ(errors[2], 1.5);
    EXPECT_DOUBLE_EQ(errors[3], 2.0);

    double mixed[] = {4.0, 1.0, 3.0, 2.0};
    EXPECT_EQ(castle::statistic::quantile_inplace(mixed, mixed + 4, 0.5, out, quantile_method::nearest_rank),
              status::ok);
    EXPECT_DOUBLE_EQ(out, 2.0);

    int ints[] = {5, 3, 1};
    EXPECT_EQ(castle::statistic::quantile_inplace(ints, ints + 3, 0.5, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 3.0);

    // Empty range with a valid probability.
    out = -7.0;
    EXPECT_EQ(castle::statistic::quantile_inplace(errors, errors, 0.5, out), status::empty);
    EXPECT_EQ(out, -7.0);
}
} // namespace
