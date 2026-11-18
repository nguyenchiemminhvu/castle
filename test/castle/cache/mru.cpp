#include <gtest/gtest.h>
#include "castle/cache/mru.hpp"
#include "castle/cache/lru.hpp"

#include <vector>

namespace
{
using Cache = castle::cache::mru<int, int, 3U>;

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

TEST(CacheMruTest, PolicyTagIsExposed)
{
    static_assert(castle::meta::is_same<Cache::policy_type, castle::cache::mru_policy>::value, "policy");
    Cache cache;
    EXPECT_EQ(cache.capacity(), 3U);
}

TEST(CacheMruTest, EvictsMostRecentlyUsedEntry)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    ASSERT_NE(cache.get(1), nullptr);
    EXPECT_EQ(put_and_report(cache, 4, 40), 1);
    EXPECT_EQ(put_and_report(cache, 5, 50), 4);
    EXPECT_EQ(drain(cache), V({5, 3, 2}));
}

TEST(CacheMruTest, UpdateCountsAsAccess)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    cache.put(2, 21);
    EXPECT_EQ(put_and_report(cache, 4, 40), 2);
}

TEST(CacheMruTest, PeekDoesNotRefresh)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    ASSERT_NE(cache.peek(1), nullptr);
    EXPECT_EQ(put_and_report(cache, 4, 40), 3);
}

TEST(CacheMruTest, AccessingNewestEntryKeepsOrder)
{
    Cache cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    ASSERT_NE(cache.get(3), nullptr);
    ASSERT_NE(cache.get(1), nullptr);
    EXPECT_EQ(drain(cache), V({1, 3, 2}));
}

template <typename Scanned>
int cyclic_scan_hits(Scanned& cache)
{
    int hits = 0;
    for (int round = 0; round < 4; ++round)
    {
        for (int key = 0; key < 4; ++key)
        {
            if (cache.get(key) != nullptr)
            {
                ++hits;
            }
            else
            {
                cache.put(key, key);
            }
        }
    }
    return hits;
}

TEST(CacheMruTest, CyclicScanOutperformsLru)
{
    Cache mru;
    castle::cache::lru<int, int, 3U> lru;
    EXPECT_EQ(cyclic_scan_hits(lru), 0);
    EXPECT_GT(cyclic_scan_hits(mru), 0);
}
}
