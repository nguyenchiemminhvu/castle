#include "sample_support.hpp"

#include "castle/cache/fifo.hpp"

// Duplicate-frame filter: remember the last few message IDs in arrival order. Old IDs age out in
// the order they were seen, no matter how often a duplicate hits them.
int main()
{
    castle::cache::fifo<uint32_t, uint16_t, 3U> seen; // frame ID -> sequence number

    CASTLE_SAMPLE_CHECK(seen.capacity() == 3U);
    seen.put(0x100U, 1U);
    seen.put(0x200U, 2U);
    seen.put(0x300U, 3U);
    CASTLE_SAMPLE_CHECK(seen.full());

    // A duplicate of 0x100 is detected, but FIFO does not let it jump the queue.
    CASTLE_SAMPLE_CHECK(seen.get(0x100U) != nullptr);

    uint32_t dropped = 0U;
    seen.put(0x400U, 4U, [&dropped](const uint32_t& id, uint16_t&) { dropped = id; });
    CASTLE_SAMPLE_CHECK(dropped == 0x100U);
    CASTLE_SAMPLE_CHECK(!seen.contains(0x100U));
    CASTLE_SAMPLE_CHECK(seen.contains(0x200U));
    CASTLE_SAMPLE_CHECK(*seen.peek(0x400U) == 4U);

    return 0;
}
