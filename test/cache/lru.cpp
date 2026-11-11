#include <gtest/gtest.h>
#include "castle/cache/lru.hpp"

#include <vector>

namespace
{
using Cache = castle::cache::lru<int, int, 3U>;

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

TEST(CacheLruTest, PolicyTagIsExposed)
{
    static_assert(castle::meta::is_same<Cache::policy_type, castle::cache::lru_policy>::value, "policy");
    Cache cache;
    EXPECT_EQ(cache.capacity(), 3U);
}

TEST(CacheLruTest, EvictsLeastRecentlyUsedEntry)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    ASSERT_NE(cache.get(1), nullptr);
    EXPECT_EQ(put_and_report(cache, 4, 40), 2);
    EXPECT_EQ(put_and_report(cache, 5, 50), 3);
    EXPECT_EQ(drain(cache), V({1, 4, 5}));
}

TEST(CacheLruTest, UpdateCountsAsAccess)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    cache.put(1, 11);
    EXPECT_EQ(put_and_report(cache, 4, 40), 2);
    EXPECT_EQ(*cache.peek(1), 11);
}

TEST(CacheLruTest, PeekAndContainsDoNotRefresh)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    ASSERT_NE(cache.peek(1), nullptr);
    EXPECT_TRUE(cache.contains(1));
    EXPECT_EQ(put_and_report(cache, 4, 40), 1);
}

TEST(CacheLruTest, AccessingNewestEntryKeepsOrder)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    ASSERT_NE(cache.get(3), nullptr);
    ASSERT_NE(cache.get(3), nullptr);
    ASSERT_NE(cache.get(2), nullptr); // middle entry moves to the newest end
    EXPECT_EQ(drain(cache), V({1, 3, 2}));
}

TEST(CacheLruTest, EraseFromEitherEnd)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    EXPECT_EQ(cache.erase(1), castle::status::ok);
    EXPECT_EQ(cache.erase(3), castle::status::ok);
    cache.put(4, 40);
    EXPECT_EQ(drain(cache), V({2, 4}));
}
}
