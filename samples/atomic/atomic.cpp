#include "sample_support.hpp"

#include "castle/atomic/atomic.hpp"

#include <stdint.h>

namespace
{
struct register_snapshot
{
    uint32_t counter;
    uint16_t flags;
};
}

int main()
{
    castle::atomic<uint32_t> flags{1U};
    CASTLE_SAMPLE_CHECK(flags.load() == 1U);
    CASTLE_SAMPLE_CHECK(flags++ == 1U);
    CASTLE_SAMPLE_CHECK(++flags == 3U);
    CASTLE_SAMPLE_CHECK((flags += 5U) == 8U);
    CASTLE_SAMPLE_CHECK((flags -= 3U) == 5U);
    CASTLE_SAMPLE_CHECK(flags.fetch_or(0x10U, castle::memory_order_release) == 5U);
    CASTLE_SAMPLE_CHECK(flags.load(castle::memory_order_acquire) == 0x15U);
    CASTLE_SAMPLE_CHECK(flags.fetch_and(0x1FU) == 0x15U);
    CASTLE_SAMPLE_CHECK(flags.fetch_xor(0x05U) == 0x15U);
    CASTLE_SAMPLE_CHECK(flags.load() == 0x10U);

    uint32_t expected = 0x10U;
    CASTLE_SAMPLE_CHECK(flags.compare_exchange_strong(
        expected,
        0x20U,
        castle::memory_order_acq_rel,
        castle::memory_order_acquire));
    CASTLE_SAMPLE_CHECK(flags.exchange(0U) == 0x20U);

    uint32_t words[4] = {0U, 1U, 2U, 3U};
    castle::atomic<uint32_t*> cursor{&words[0]};
    CASTLE_SAMPLE_CHECK(cursor.fetch_add(2) == &words[0]);
    CASTLE_SAMPLE_CHECK(cursor.load() == &words[2]);
    CASTLE_SAMPLE_CHECK(cursor.fetch_sub(1) == &words[2]);
    CASTLE_SAMPLE_CHECK(cursor.load() == &words[1]);

    register_snapshot snapshot_value = {7U, 3U};
    castle::atomic<register_snapshot> snapshot{snapshot_value};
    register_snapshot loaded = snapshot.load();
    CASTLE_SAMPLE_CHECK(loaded.counter == 7U && loaded.flags == 3U);

    register_snapshot expected_snapshot = {7U, 3U};
    register_snapshot desired_snapshot = {9U, 5U};
    CASTLE_SAMPLE_CHECK(snapshot.compare_exchange_strong(expected_snapshot, desired_snapshot));
    loaded = snapshot.exchange(register_snapshot{1U, 2U});
    CASTLE_SAMPLE_CHECK(loaded.counter == 9U && loaded.flags == 5U);
    loaded = snapshot.load();
    CASTLE_SAMPLE_CHECK(loaded.counter == 1U && loaded.flags == 2U);

    castle::atomic_thread_fence(castle::memory_order_seq_cst);
    castle::atomic_signal_fence(castle::memory_order_acquire);
    return 0;
}
