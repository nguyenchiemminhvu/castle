#include "sample_support.h"

#include "castle/atomic/atomic.h"

// Scenario: an ISR/task handshake and a DMA cursor use atomic load/store, RMW and CAS operations.
#include <stdint.h>

namespace
{
castle::atomic<uint32_t> g_event_flags{0U};
castle::atomic<uint32_t*> g_buffer_cursor{nullptr};
}

int main()
{
    // ISR: publish an event bit without locking.
    g_event_flags.fetch_or(0x04U, castle::memory_order_release);
    CASTLE_SAMPLE_CHECK(g_event_flags.load(castle::memory_order_acquire) == 0x04U);

    uint32_t expected = 0x04U;
    CASTLE_SAMPLE_CHECK(
        g_event_flags.compare_exchange_strong(
            expected, 0x08U,
            castle::memory_order_acq_rel,
            castle::memory_order_acquire
        )
    );
    CASTLE_SAMPLE_CHECK(g_event_flags.exchange(0U) == 0x08U);

    uint32_t dma_words[4] = {};
    g_buffer_cursor.store(&dma_words[0], castle::memory_order_release);
    CASTLE_SAMPLE_CHECK(g_buffer_cursor.load(castle::memory_order_acquire) == &dma_words[0]);
    g_buffer_cursor.fetch_add(2);
    CASTLE_SAMPLE_CHECK(g_buffer_cursor.load() == &dma_words[2]);
    return 0;
}
