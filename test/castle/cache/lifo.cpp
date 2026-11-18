#include <gtest/gtest.h>
#include "castle/cache/lifo.hpp"

#include <vector>

namespace
{
using Cache = castle::cache::lifo<int, int, 3U>;

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

TEST(CacheLifoTest, PolicyTagIsExposed)
{
    static_assert(castle::meta::is_same<Cache::policy_type, castle::cache::lifo_policy>::value, "policy");
    Cache cache;
    EXPECT_EQ(cache.capacity(), 3U);
}

TEST(CacheLifoTest, EvictsMostRecentlyInsertedEntry)
{
    Cache cache;
    EXPECT_EQ(put_and_report(cache, 1, 10), -1);
    EXPECT_EQ(put_and_report(cache, 2, 20), -1);
    EXPECT_EQ(put_and_report(cache, 3, 30), -1);
    EXPECT_EQ(put_and_report(cache, 4, 40), 3);
    EXPECT_EQ(put_and_report(cache, 5, 50), 4);
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_EQ(drain(cache), V({5, 2, 1}));
}

TEST(CacheLifoTest, AccessesDoNotChangeOrder)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    ASSERT_NE(cache.get(1), nullptr);
    cache.put(2, 21); // update keeps the original insertion slot
    EXPECT_EQ(*cache.peek(2), 21);
    EXPECT_EQ(put_and_report(cache, 4, 40), 3);
    EXPECT_EQ(drain(cache), V({4, 2, 1}));
}

TEST(CacheLifoTest, EraseOfNewestExposesPreviousEntry)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    EXPECT_EQ(cache.erase(3), castle::status::ok);
    EXPECT_EQ(cache.evict(), castle::status::ok);
    EXPECT_FALSE(cache.contains(2));
    EXPECT_EQ(drain(cache), V({1}));
}
}
