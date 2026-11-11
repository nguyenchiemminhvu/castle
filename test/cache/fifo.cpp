#include <gtest/gtest.h>
#include "castle/cache/fifo.hpp"

#include <vector>

namespace
{
using Cache = castle::cache::fifo<int, int, 3U>;

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

TEST(CacheFifoTest, PolicyTagIsExposed)
{
    static_assert(castle::meta::is_same<Cache::policy_type, castle::cache::fifo_policy>::value, "policy");
    Cache cache;
    EXPECT_EQ(cache.capacity(), 3U);
}

TEST(CacheFifoTest, EvictsEarliestInsertedEntry)
{
    Cache cache;
    EXPECT_EQ(put_and_report(cache, 1, 10), -1);
    EXPECT_EQ(put_and_report(cache, 2, 20), -1);
    EXPECT_EQ(put_and_report(cache, 3, 30), -1);
    EXPECT_EQ(put_and_report(cache, 4, 40), 1);
    EXPECT_EQ(put_and_report(cache, 5, 50), 2);
    EXPECT_EQ(drain(cache), V({3, 4, 5}));
}

TEST(CacheFifoTest, AccessesDoNotChangeOrder)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    ASSERT_NE(cache.get(1), nullptr);
    ASSERT_NE(cache.get(1), nullptr);
    cache.put(1, 11); // update keeps the original insertion slot
    EXPECT_EQ(put_and_report(cache, 4, 40), 1);
    EXPECT_FALSE(cache.contains(1));
    EXPECT_EQ(drain(cache), V({2, 3, 4}));
}

TEST(CacheFifoTest, EraseKeepsRemainingOrder)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    EXPECT_EQ(cache.erase(2), castle::status::ok);
    cache.put(4, 40);
    EXPECT_EQ(put_and_report(cache, 5, 50), 1);
    EXPECT_EQ(drain(cache), V({3, 4, 5}));
}

TEST(CacheFifoTest, ClearRestartsOrdering)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.clear();
    cache.put(3, 30);
    cache.put(4, 40);
    EXPECT_EQ(drain(cache), V({3, 4}));
}
}
