#include "sample_support.hpp"

#include "castle/cache/cache.hpp"

namespace
{
// Replays the same access trace against any cache and returns the hit count.
template <typename Cache>
uint32_t replay(Cache& cache)
{
    static const uint8_t trace[] = {1U, 2U, 3U, 1U, 4U, 1U, 2U, 5U, 1U, 2U};
    uint32_t hits = 0U;
    for (uint8_t key : trace)
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
    return hits;
}
} // namespace

// One include, every policy: compare them on a workload before picking one.
int main()
{
    castle::cache::fifo<uint8_t, uint8_t, 3U> fifo;
    castle::cache::lifo<uint8_t, uint8_t, 3U> lifo;
    castle::cache::lru<uint8_t, uint8_t, 3U> lru;
    castle::cache::mru<uint8_t, uint8_t, 3U> mru;
    castle::cache::lfu<uint8_t, uint8_t, 3U> lfu;

    const uint32_t fifo_hits = replay(fifo);
    const uint32_t lifo_hits = replay(lifo);
    const uint32_t lru_hits = replay(lru);
    const uint32_t mru_hits = replay(mru);
    const uint32_t lfu_hits = replay(lfu);

    // Key 1 is hot: the recency- and frequency-aware policies keep it resident.
    CASTLE_SAMPLE_CHECK(lru_hits >= fifo_hits);
    CASTLE_SAMPLE_CHECK(lfu_hits >= fifo_hits);
    CASTLE_SAMPLE_CHECK(lifo_hits + mru_hits > 0U);
    CASTLE_SAMPLE_CHECK(lru.contains(1U));
    CASTLE_SAMPLE_CHECK(lfu.contains(1U));

    return 0;
}
