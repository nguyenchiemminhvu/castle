#include <gtest/gtest.h>
#include "castle/cache/lfu.hpp"

#include <vector>

namespace
{
using Cache = castle::cache::lfu<int, int, 3U>;

template <typename Cache>
std::vector<int> drain(Cache& cache)
{
    std::vector<int> keys;
    while (cache.evict([&keys](const int& key, int&) { keys.push_back(key); }) == castle::status::ok)
    {
    }
    return keys;
}

int put_and_report(Cache& cache, int key, int value)
{
    int evicted = -1;
    cache.put(key, value, [&evicted](const int& victim, int&) { evicted = victim; });
    return evicted;
}

using V = std::vector<int>;

TEST(CacheLfuTest, PolicyTagIsExposed)
{
    static_assert(castle::meta::is_same<Cache::policy_type, castle::cache::lfu_policy<uint32_t>>::value, "policy");
    Cache cache;
    EXPECT_EQ(cache.capacity(), 3U);
}

TEST(CacheLfuTest, EvictsLeastFrequentlyUsedEntry)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    ASSERT_NE(cache.get(1), nullptr);
    ASSERT_NE(cache.get(1), nullptr);
    ASSERT_NE(cache.get(3), nullptr);
    EXPECT_EQ(put_and_report(cache, 4, 40), 2);
    EXPECT_EQ(put_and_report(cache, 5, 50), 4); // newcomer count 1 is now the minimum
    EXPECT_EQ(drain(cache), V({5, 3, 1}));
}

TEST(CacheLfuTest, TiesAreBrokenByLeastRecentUse)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    EXPECT_EQ(put_and_report(cache, 4, 40), 1);
    ASSERT_NE(cache.get(3), nullptr);
    ASSERT_NE(cache.get(2), nullptr);
    ASSERT_NE(cache.get(4), nullptr);
    EXPECT_EQ(put_and_report(cache, 5, 50), 3);
}

TEST(CacheLfuTest, UpdateCountsAsAccessButPeekDoesNot)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    cache.put(1, 11);
    ASSERT_NE(cache.peek(2), nullptr);
    EXPECT_TRUE(cache.contains(2));
    EXPECT_EQ(put_and_report(cache, 4, 40), 2);
    EXPECT_EQ(*cache.peek(1), 11);
}

TEST(CacheLfuTest, PromotionPathsKeepFrequencyOrder)
{
    castle::cache::lfu<int, int, 4U> cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);            // count1: 1 2 3
    ASSERT_NE(cache.get(1), nullptr); // new trailing bucket       -> count1: 2 3   count2: 1
    ASSERT_NE(cache.get(2), nullptr); // join next bucket          -> count1: 3     count2: 1 2
    ASSERT_NE(cache.get(3), nullptr); // join next, release lowest -> count2: 1 2 3
    ASSERT_NE(cache.get(1), nullptr); // new trailing bucket       -> count2: 2 3   count3: 1
    ASSERT_NE(cache.get(1), nullptr); // sole member bumps in place-> count2: 2 3   count4: 1
    ASSERT_NE(cache.get(2), nullptr); // new middle bucket         -> count2: 3     count3: 2  count4: 1
    ASSERT_NE(cache.get(3), nullptr); // join next, release lowest -> count3: 2 3   count4: 1
    cache.put(4, 40);                 // new lowest bucket         -> count1: 4 ...
    ASSERT_NE(cache.get(4), nullptr); // sole member bumps below a larger bucket -> count2: 4
    EXPECT_EQ(drain(cache), V({4, 2, 3, 1}));
}

TEST(CacheLfuTest, EraseReleasesBucketsAnywhere)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    ASSERT_NE(cache.get(2), nullptr);
    ASSERT_NE(cache.get(3), nullptr);
    ASSERT_NE(cache.get(3), nullptr); // count1: 1   count2: 2   count3: 3
    EXPECT_EQ(cache.erase(2), castle::status::ok); // middle bucket
    EXPECT_EQ(cache.erase(3), castle::status::ok); // highest bucket
    cache.put(4, 40);
    EXPECT_EQ(cache.erase(1), castle::status::ok); // bucket keeps key 4
    cache.put(5, 50);
    cache.put(6, 60);
    EXPECT_EQ(drain(cache), V({4, 5, 6}));
}

TEST(CacheLfuTest, ClearResetsCounts)
{
    Cache cache;
    cache.put(1, 10);
    ASSERT_NE(cache.get(1), nullptr);
    ASSERT_NE(cache.get(1), nullptr);
    cache.clear();
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    ASSERT_NE(cache.get(2), nullptr);
    EXPECT_EQ(put_and_report(cache, 4, 40), 1);
}

TEST(CacheLfuTest, NarrowCounterSaturatesDeterministically)
{
    castle::cache::basic_cache<int, int, 2U, castle::cache::lfu_policy<uint8_t>> cache;
    cache.put(1, 10);
    for (int i = 0; i < 300; ++i)
    {
        ASSERT_NE(cache.get(1), nullptr);
    }
    cache.put(2, 20);
    for (int i = 0; i < 300; ++i)
    {
        ASSERT_NE(cache.get(2), nullptr);
    }
    // Both counts are pinned at 255; key 2 reached it last, so key 1 is the LRU tie.
    ASSERT_NE(cache.get(1), nullptr); // saturated access still refreshes recency
    int evicted = -1;
    cache.put(3, 30, [&evicted](const int& key, int&) { evicted = key; });
    EXPECT_EQ(evicted, 2);
    EXPECT_EQ(drain(cache), V({3, 1}));
}
}
