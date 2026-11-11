#include <gtest/gtest.h>
#include "castle/cache/cache.hpp"

namespace
{
template <typename Cache>
int evict_after_touching_first(Cache& cache)
{
    cache.put(1, 10);
    cache.put(2, 20);
    static_cast<void>(cache.get(1));
    int evicted = -1;
    cache.put(3, 30, [&evicted](const int& key, int&) { evicted = key; });
    return evicted;
}

TEST(CacheUmbrellaTest, ExposesEveryPolicy)
{
    castle::cache::fifo<int, int, 2U> fifo;
    castle::cache::lifo<int, int, 2U> lifo;
    castle::cache::lru<int, int, 2U> lru;
    castle::cache::mru<int, int, 2U> mru;
    castle::cache::lfu<int, int, 2U> lfu;
    EXPECT_EQ(evict_after_touching_first(fifo), 1);
    EXPECT_EQ(evict_after_touching_first(lifo), 2);
    EXPECT_EQ(evict_after_touching_first(lru), 2);
    EXPECT_EQ(evict_after_touching_first(mru), 1);
    EXPECT_EQ(evict_after_touching_first(lfu), 2);
}
}
