#include "sample_support.hpp"

#include "castle/cache/mru.hpp"

// Repeated sequential scans over more blocks than the cache holds. LRU would miss on every access;
// MRU sacrifices the block just used, so most of the working set stays resident between scans.
int main()
{
    castle::cache::mru<uint8_t, uint32_t, 3U> blocks; // block number -> checksum

    uint32_t hits = 0U;
    uint32_t misses = 0U;
    for (uint8_t pass = 0U; pass < 3U; ++pass)
    {
        for (uint8_t block = 0U; block < 4U; ++block)
        {
            if (blocks.get(block) != nullptr)
            {
                ++hits;
            }
            else
            {
                ++misses;
                blocks.put(block, static_cast<uint32_t>(block) * 0x1111U);
            }
        }
    }

    CASTLE_SAMPLE_CHECK(hits + misses == 12U);
    CASTLE_SAMPLE_CHECK(hits >= 5U);
    CASTLE_SAMPLE_CHECK(blocks.full());

    return 0;
}
