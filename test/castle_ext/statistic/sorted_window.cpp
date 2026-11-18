#include <gtest/gtest.h>

#include <cstddef>
#include <initializer_list>
#include <limits>
#include <stdint.h>

#include "castle_ext/statistic/sorted_window.hpp"

namespace
{
using castle::status;
using castle::statistic::quantile_method;

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

// Checks that `window.sorted()` equals `expected` element by element.
template <typename Window>
void ExpectSorted(const Window& window, std::initializer_list<int> expected)
{
    const auto view = window.sorted();
    ASSERT_EQ(static_cast<std::size_t>(view.size()), expected.size());
    std::size_t index = 0U;
    for (const int value : expected)
    {
        EXPECT_EQ(view[index], value);
        ++index;
    }
}

// Pushes one sample and checks the sorted contents afterwards.
template <typename Window>
void PushExpect(Window& window, int sample, std::initializer_list<int> expected)
{
    EXPECT_EQ(window.push(sample), status::ok);
    ExpectSorted(window, expected);
}

TEST(SortedWindowTest, HandTraceCapacityFive)
{
    using W = castle::statistic::sorted_window<int, 5U>;
    W window;
    EXPECT_EQ(W::capacity(), 5U);
    EXPECT_TRUE(window.empty());
    EXPECT_FALSE(window.full());
    EXPECT_EQ(window.count(), 0U);
    ExpectSorted(window, {});

    // Non-evicting inserts: end, front, middle, front duplicate, middle.
    PushExpect(window, 5, {5});
    PushExpect(window, 1, {1, 5});
    PushExpect(window, 4, {1, 4, 5});
    PushExpect(window, 1, {1, 1, 4, 5});
    PushExpect(window, 3, {1, 1, 3, 4, 5});
    EXPECT_TRUE(window.full());
    EXPECT_FALSE(window.empty());
    EXPECT_EQ(window.count(), 5U);

    // Evicting pushes. Arrival order before each push is shown in the comment.
    PushExpect(window, 9, {1, 1, 3, 4, 9}); // evict 5: insert_at == remove_at + 1
    PushExpect(window, 2, {1, 2, 3, 4, 9}); // evict 1: one element shifts
    PushExpect(window, 6, {1, 2, 3, 6, 9}); // evict 4: insert_at == remove_at + 1
    PushExpect(window, 5, {2, 3, 5, 6, 9}); // evict 1: two elements shift
    PushExpect(window, 3, {2, 3, 5, 6, 9}); // evict 3: sample equals the evicted one
    PushExpect(window, 5, {2, 3, 5, 5, 6}); // evict 9: insert_at <= remove_at

    EXPECT_EQ(window.count(), 5U);
    EXPECT_TRUE(window.full());

    // Arrival order of the retained samples: 2, 6, 5, 3, 5.
    const auto& samples = window.samples();
    ASSERT_EQ(samples.size(), 5U);
    EXPECT_EQ(samples[0U], 2);
    EXPECT_EQ(samples[1U], 6);
    EXPECT_EQ(samples[2U], 5);
    EXPECT_EQ(samples[3U], 3);
    EXPECT_EQ(samples[4U], 5);
}

TEST(SortedWindowTest, EvictionWhereInsertEqualsRemovePosition)
{
    castle::statistic::sorted_window<int, 3U> window;
    PushExpect(window, 5, {5});
    PushExpect(window, 1, {1, 5});
    PushExpect(window, 9, {1, 5, 9});
    // Evict 5 (position 1); 3 also belongs at position 1.
    PushExpect(window, 3, {1, 3, 9});
}

TEST(SortedWindowTest, EvictionMovesNewSampleToFront)
{
    castle::statistic::sorted_window<int, 3U> window;
    PushExpect(window, 9, {9});
    PushExpect(window, 8, {8, 9});
    PushExpect(window, 7, {7, 8, 9});
    // Evict 9 (position 2); 1 belongs at position 0: two elements shift up.
    PushExpect(window, 1, {1, 7, 8});
    // Arrival {8, 7, 1}: evict 8 (position 2), insert 20 at position 3.
    PushExpect(window, 20, {1, 7, 20});
    // Arrival {7, 1, 20}: evict 7, insert 7: stays in place.
    PushExpect(window, 7, {1, 7, 20});
    // Arrival {1, 20, 7}: evict 1 (position 0), insert 7 after the existing 7.
    PushExpect(window, 7, {7, 7, 20});
}

TEST(SortedWindowTest, DuplicatesOnly)
{
    castle::statistic::sorted_window<int, 3U> window;
    PushExpect(window, 4, {4});
    PushExpect(window, 4, {4, 4});
    PushExpect(window, 4, {4, 4, 4});
    // Evict 4 (first of three equal values) and insert 4 behind all of them.
    PushExpect(window, 4, {4, 4, 4});
    PushExpect(window, 4, {4, 4, 4});
    PushExpect(window, 5, {4, 4, 5});
    PushExpect(window, 3, {3, 4, 5}); // evicts the oldest 4
}

TEST(SortedWindowTest, CapacityOne)
{
    using W = castle::statistic::sorted_window<int, 1U>;
    W window;
    EXPECT_EQ(W::capacity(), 1U);
    PushExpect(window, 5, {5});
    EXPECT_TRUE(window.full());
    PushExpect(window, 3, {3});
    PushExpect(window, 7, {7});
    PushExpect(window, 7, {7});
    EXPECT_EQ(window.count(), 1U);

    double out = 0.0;
    EXPECT_EQ(window.median(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 7.0);
    EXPECT_EQ(window.interquartile_range(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);
    EXPECT_EQ(window.range(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);
}

TEST(SortedWindowTest, RejectsNonFiniteSamples)
{
    castle::statistic::sorted_window<double, 3U> window;
    EXPECT_EQ(window.push(kNaN), status::invalid_argument);
    EXPECT_EQ(window.push(kInf), status::invalid_argument);
    EXPECT_EQ(window.push(-kInf), status::invalid_argument);
    EXPECT_TRUE(window.empty());

    EXPECT_EQ(window.push(1.5), status::ok);
    EXPECT_EQ(window.push(kNaN), status::invalid_argument);
    EXPECT_EQ(window.push(kInf), status::invalid_argument);
    EXPECT_EQ(window.count(), 1U);
    ASSERT_EQ(window.sorted().size(), 1U);
    EXPECT_DOUBLE_EQ(window.sorted()[0U], 1.5);

    castle::statistic::sorted_window<float, 2U> single;
    EXPECT_EQ(single.push(std::numeric_limits<float>::quiet_NaN()), status::invalid_argument);
    EXPECT_EQ(single.push(std::numeric_limits<float>::infinity()), status::invalid_argument);
    EXPECT_TRUE(single.empty());
}

TEST(SortedWindowTest, EmptyQueries)
{
    castle::statistic::sorted_window<double, 4U> window;
    double out = -7.0;
    EXPECT_EQ(window.quantile(0.5, out), status::empty);
    EXPECT_EQ(window.quantile(0.5, out, quantile_method::nearest_rank), status::empty);
    EXPECT_EQ(window.quantile(2.0, out), status::invalid_argument);
    EXPECT_EQ(window.median(out), status::empty);
    EXPECT_EQ(window.interquartile_range(out), status::empty);
    EXPECT_EQ(window.minimum(out), status::empty);
    EXPECT_EQ(window.maximum(out), status::empty);
    EXPECT_EQ(window.range(out), status::empty);
    EXPECT_EQ(window.mode(out), status::empty);
    EXPECT_EQ(window.cdf(1.0, out), status::empty);
    EXPECT_EQ(window.cdf(kInf, out), status::empty);
    EXPECT_EQ(window.cdf(kNaN, out), status::invalid_argument);
    EXPECT_EQ(out, -7.0);
    EXPECT_EQ(window.sorted().size(), 0U);
    EXPECT_EQ(window.samples().size(), 0U);
}

TEST(SortedWindowTest, QuantileAndSummaryQueries)
{
    castle::statistic::sorted_window<int, 5U> window;
    const int inputs[] = {5, 3, 2, 6, 5};
    for (const int value : inputs)
    {
        EXPECT_EQ(window.push(value), status::ok);
    }
    ExpectSorted(window, {2, 3, 5, 5, 6});

    double out = 0.0;
    // position 0.95 * 4 = 3.8: 5 + (6 - 5) * 0.8 = 5.8.
    EXPECT_EQ(window.quantile(0.95, out), status::ok);
    EXPECT_NEAR(out, 5.8, 1e-12);
    // 0.95 * 5 = 4.75 -> rank 5 -> index 4.
    EXPECT_EQ(window.quantile(0.95, out, quantile_method::nearest_rank), status::ok);
    EXPECT_DOUBLE_EQ(out, 6.0);
    EXPECT_EQ(window.quantile(0.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.0);
    EXPECT_EQ(window.quantile(1.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 6.0);

    out = -7.0;
    EXPECT_EQ(window.quantile(1.5, out), status::invalid_argument);
    EXPECT_EQ(window.quantile(kNaN, out), status::invalid_argument);
    EXPECT_EQ(window.quantile(-0.1, out), status::invalid_argument);
    EXPECT_EQ(out, -7.0);

    EXPECT_EQ(window.median(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 5.0);

    // Linear: Q1 position 1 -> 3, Q3 position 3 -> 5.
    EXPECT_EQ(window.interquartile_range(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.0);
    // Nearest rank: Q1 = index 1 -> 3, Q3 = index 3 -> 5.
    EXPECT_EQ(window.interquartile_range(out, quantile_method::nearest_rank), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.0);

    EXPECT_EQ(window.minimum(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 2.0);
    EXPECT_EQ(window.maximum(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 6.0);
    EXPECT_EQ(window.range(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 4.0);
    EXPECT_EQ(window.mode(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 5.0);
}

TEST(SortedWindowTest, ModeTiesResolveToSmallest)
{
    castle::statistic::sorted_window<int, 4U> ties;
    ties.push(2);
    ties.push(1);
    ties.push(2);
    ties.push(1);
    double out = 0.0;
    EXPECT_EQ(ties.mode(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);

    castle::statistic::sorted_window<int, 4U> unique;
    unique.push(3);
    unique.push(1);
    unique.push(2);
    EXPECT_EQ(unique.mode(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);
}

TEST(SortedWindowTest, EmpiricalCdf)
{
    castle::statistic::sorted_window<int, 5U> window;
    const int inputs[] = {5, 3, 2, 6, 5};
    for (const int value : inputs)
    {
        window.push(value);
    }
    // Sorted: 2 3 5 5 6.
    double out = -7.0;
    EXPECT_EQ(window.cdf(5.0, out), status::ok); // four samples <= 5
    EXPECT_DOUBLE_EQ(out, 0.8);
    EXPECT_EQ(window.cdf(4.0, out), status::ok); // between samples: 2 and 3
    EXPECT_DOUBLE_EQ(out, 0.4);
    EXPECT_EQ(window.cdf(4.999, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.4);
    EXPECT_EQ(window.cdf(1.0, out), status::ok); // below the minimum
    EXPECT_DOUBLE_EQ(out, 0.0);
    EXPECT_EQ(window.cdf(100.0, out), status::ok); // above the maximum
    EXPECT_DOUBLE_EQ(out, 1.0);
    EXPECT_EQ(window.cdf(6.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);
    EXPECT_EQ(window.cdf(kInf, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 1.0);
    EXPECT_EQ(window.cdf(-kInf, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);

    out = -7.0;
    EXPECT_EQ(window.cdf(kNaN, out), status::invalid_argument);
    EXPECT_EQ(out, -7.0);
}

TEST(SortedWindowTest, ClearAllowsReuse)
{
    castle::statistic::sorted_window<int, 3U> window;
    window.push(9);
    window.push(8);
    window.push(7);
    EXPECT_TRUE(window.full());
    window.clear();
    EXPECT_TRUE(window.empty());
    EXPECT_FALSE(window.full());
    EXPECT_EQ(window.count(), 0U);
    EXPECT_EQ(window.sorted().size(), 0U);
    EXPECT_EQ(window.samples().size(), 0U);

    PushExpect(window, 7, {7});
    PushExpect(window, 3, {3, 7});
    double out = 0.0;
    EXPECT_EQ(window.median(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 5.0);
}

TEST(SortedWindowTest, UnsignedIntegralSamples)
{
    using W = castle::statistic::sorted_window<uint32_t, 4U>;
    static_assert(sizeof(W::result_type) == sizeof(double), "integral samples report in double");
    W window;
    EXPECT_EQ(window.push(10U), status::ok);
    EXPECT_EQ(window.push(4000000000U), status::ok);
    EXPECT_EQ(window.push(5U), status::ok);
    EXPECT_EQ(window.push(7U), status::ok);

    ASSERT_EQ(window.sorted().size(), 4U);
    EXPECT_EQ(window.sorted()[0U], 5U);
    EXPECT_EQ(window.sorted()[1U], 7U);
    EXPECT_EQ(window.sorted()[2U], 10U);
    EXPECT_EQ(window.sorted()[3U], 4000000000U);

    double out = 0.0;
    EXPECT_EQ(window.minimum(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 5.0);
    EXPECT_EQ(window.maximum(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 4000000000.0);
    EXPECT_EQ(window.range(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 3999999995.0);
    // position 1.5: 7 + (10 - 7) * 0.5.
    EXPECT_EQ(window.median(out), status::ok);
    EXPECT_DOUBLE_EQ(out, 8.5);

    // Evict 10: arrival {4e9, 5, 7, 1}.
    EXPECT_EQ(window.push(1U), status::ok);
    ASSERT_EQ(window.sorted().size(), 4U);
    EXPECT_EQ(window.sorted()[0U], 1U);
    EXPECT_EQ(window.sorted()[1U], 5U);
    EXPECT_EQ(window.sorted()[2U], 7U);
    EXPECT_EQ(window.sorted()[3U], 4000000000U);
    EXPECT_EQ(window.cdf(5.0, out), status::ok);
    EXPECT_DOUBLE_EQ(out, 0.5);
}

TEST(SortedWindowTest, FloatSamples)
{
    castle::statistic::sorted_window<float, 3U> window;
    EXPECT_EQ(window.push(3.0f), status::ok);
    EXPECT_EQ(window.push(1.0f), status::ok);
    EXPECT_EQ(window.push(2.0f), status::ok);
    float out = 0.0f;
    EXPECT_EQ(window.median(out), status::ok);
    EXPECT_FLOAT_EQ(out, 2.0f);

    // Evict 3: arrival {1, 2, 0.5}.
    EXPECT_EQ(window.push(0.5f), status::ok);
    EXPECT_EQ(window.median(out), status::ok);
    EXPECT_FLOAT_EQ(out, 1.0f);
    EXPECT_EQ(window.minimum(out), status::ok);
    EXPECT_FLOAT_EQ(out, 0.5f);
    EXPECT_EQ(window.cdf(1.0f, out), status::ok);
    EXPECT_FLOAT_EQ(out, 2.0f / 3.0f);
    EXPECT_EQ(window.quantile(0.5f, out, quantile_method::nearest_rank), status::ok);
    EXPECT_FLOAT_EQ(out, 1.0f);
}
} // namespace
