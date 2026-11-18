#include <gtest/gtest.h>

#include <stdint.h>

#include "castle_ext/statistic/sample_window.hpp"

namespace
{
TEST(SampleWindowTest, EmptyState)
{
    castle::statistic::sample_window<int, 4U> window;
    EXPECT_EQ((castle::statistic::sample_window<int, 4U>::capacity()), 4U);
    EXPECT_EQ(window.size(), 0U);
    EXPECT_TRUE(window.empty());
    EXPECT_FALSE(window.full());
    EXPECT_EQ(window.available(), 4U);

    int out = 99;
    EXPECT_EQ(window.oldest(out), castle::status::empty);
    EXPECT_EQ(window.newest(out), castle::status::empty);
    EXPECT_EQ(window.at(0U, out), castle::status::out_of_range);
    EXPECT_EQ(out, 99);
    EXPECT_EQ(window[0U], 0);
    EXPECT_TRUE(window.begin() == window.end());
    EXPECT_TRUE(window.latest(3U).empty());
    EXPECT_EQ(window.latest(3U).size(), 0U);
}

TEST(SampleWindowTest, PushWithoutEviction)
{
    castle::statistic::sample_window<int, 3U> window;
    EXPECT_FALSE(window.push(10));
    EXPECT_FALSE(window.push(20));
    EXPECT_EQ(window.size(), 2U);
    EXPECT_EQ(window.available(), 1U);
    EXPECT_FALSE(window.full());

    int evicted = -1;
    EXPECT_FALSE(window.push(30, evicted));
    EXPECT_EQ(evicted, -1); // unchanged: nothing was evicted
    EXPECT_TRUE(window.full());
    EXPECT_EQ(window.available(), 0U);
}

TEST(SampleWindowTest, PushWithEviction)
{
    castle::statistic::sample_window<int, 3U> window;
    window.push(1);
    window.push(2);
    window.push(3);

    // push(value) overload: drops 1, reports eviction.
    EXPECT_TRUE(window.push(4));
    EXPECT_EQ(window[0U], 2);
    EXPECT_EQ(window[2U], 4);

    // push(value, evicted) overload: drops 2.
    int evicted = 0;
    EXPECT_TRUE(window.push(5, evicted));
    EXPECT_EQ(evicted, 2);
    EXPECT_EQ(window.size(), 3U);
    EXPECT_EQ(window[0U], 3);
    EXPECT_EQ(window[1U], 4);
    EXPECT_EQ(window[2U], 5);
}

TEST(SampleWindowTest, AccessorsAndBounds)
{
    castle::statistic::sample_window<int, 3U> window;
    window.push(7);
    window.push(8);
    window.push(9);
    window.push(10); // window: 8 9 10

    int out = 0;
    EXPECT_EQ(window.at(0U, out), castle::status::ok);
    EXPECT_EQ(out, 8);
    EXPECT_EQ(window.at(2U, out), castle::status::ok);
    EXPECT_EQ(out, 10);
    out = 55;
    EXPECT_EQ(window.at(3U, out), castle::status::out_of_range);
    EXPECT_EQ(out, 55);

    EXPECT_EQ(window[1U], 9);
    EXPECT_EQ(window[3U], 0); // out of range: value-initialized
    EXPECT_EQ(window[100U], 0);

    EXPECT_EQ(window.oldest(out), castle::status::ok);
    EXPECT_EQ(out, 8);
    EXPECT_EQ(window.newest(out), castle::status::ok);
    EXPECT_EQ(out, 10);
}

TEST(SampleWindowTest, IterationOldestToNewest)
{
    castle::statistic::sample_window<int, 4U> window;
    for (int i = 1; i <= 6; ++i)
    {
        window.push(i); // window: 3 4 5 6
    }

    int expected = 3;
    size_t visited = 0U;
    for (const int sample : window)
    {
        EXPECT_EQ(sample, expected);
        ++expected;
        ++visited;
    }
    EXPECT_EQ(visited, 4U);

    auto it = window.begin();
    EXPECT_EQ(*it, 3);
    auto previous = it++;
    EXPECT_EQ(*previous, 3);
    EXPECT_EQ(*it, 4);
    EXPECT_EQ(*(++it), 5);
    EXPECT_TRUE(it != window.end());
    EXPECT_FALSE(it == window.end());
    ++it;
    ++it;
    EXPECT_TRUE(it == window.end());
    EXPECT_FALSE(it != window.end());

    castle::statistic::sample_window<int, 4U>::const_iterator singular;
    EXPECT_TRUE(singular == singular);
}

TEST(SampleWindowTest, LatestView)
{
    castle::statistic::sample_window<int, 5U> window;
    for (int i = 1; i <= 7; ++i)
    {
        window.push(i); // window: 3 4 5 6 7
    }

    auto view = window.latest(2U);
    EXPECT_EQ(view.size(), 2U);
    EXPECT_FALSE(view.empty());
    int expected = 6;
    for (const int sample : view)
    {
        EXPECT_EQ(sample, expected);
        ++expected;
    }
    EXPECT_EQ(expected, 8);

    EXPECT_EQ(window.latest(5U).size(), 5U);  // exactly size()
    EXPECT_EQ(window.latest(50U).size(), 5U); // clamped
    EXPECT_EQ(*window.latest(50U).begin(), 3);

    auto none = window.latest(0U);
    EXPECT_TRUE(none.empty());
    EXPECT_EQ(none.size(), 0U);
    EXPECT_TRUE(none.begin() == none.end());
}

TEST(SampleWindowTest, CopyTo)
{
    castle::statistic::sample_window<int, 4U> window;
    int buffer[8] = {};

    EXPECT_EQ(window.copy_to(buffer, 8U), 0U); // empty window: loop not taken
    EXPECT_EQ(window.copy_to(nullptr, 8U), 0U);

    for (int i = 1; i <= 6; ++i)
    {
        window.push(i); // window: 3 4 5 6
    }
    EXPECT_EQ(window.copy_to(nullptr, 8U), 0U);

    EXPECT_EQ(window.copy_to(buffer, 2U), 2U); // short destination
    EXPECT_EQ(buffer[0], 3);
    EXPECT_EQ(buffer[1], 4);
    EXPECT_EQ(buffer[2], 0);

    EXPECT_EQ(window.copy_to(buffer, 4U), 4U); // exactly full
    EXPECT_EQ(buffer[3], 6);

    int large[8] = {};
    EXPECT_EQ(window.copy_to(large, 8U), 4U); // large destination
    EXPECT_EQ(large[0], 3);
    EXPECT_EQ(large[3], 6);
    EXPECT_EQ(large[4], 0);

    EXPECT_EQ(window.copy_to(large, 0U), 0U);
}

TEST(SampleWindowTest, ClearResetsWindow)
{
    castle::statistic::sample_window<double, 3U> window;
    window.push(1.5);
    window.push(2.5);
    window.clear();
    EXPECT_TRUE(window.empty());
    EXPECT_EQ(window.size(), 0U);
    EXPECT_EQ(window.available(), 3U);
    window.push(4.5);
    double out = 0.0;
    EXPECT_EQ(window.oldest(out), castle::status::ok);
    EXPECT_DOUBLE_EQ(out, 4.5);
}

TEST(SampleWindowTest, CapacityOne)
{
    castle::statistic::sample_window<int, 1U> window;
    EXPECT_FALSE(window.push(1));
    EXPECT_TRUE(window.full());
    int evicted = 0;
    EXPECT_TRUE(window.push(2, evicted));
    EXPECT_EQ(evicted, 1);
    EXPECT_TRUE(window.push(3));
    EXPECT_EQ(window.size(), 1U);
    int out = 0;
    EXPECT_EQ(window.oldest(out), castle::status::ok);
    EXPECT_EQ(out, 3);
    EXPECT_EQ(window.newest(out), castle::status::ok);
    EXPECT_EQ(out, 3);
    EXPECT_EQ(window.latest(4U).size(), 1U);
}
} // namespace
