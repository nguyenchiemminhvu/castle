#include "sample_support.hpp"

#include "castle/events/thread_pool.hpp"

#include <assert.h>

static volatile int completed = 0;

int main()
{
    castle::events::thread_pool<4, 8> pool; // 4 threads, 8 tasks

    assert(pool.running() == true);
    assert(pool.thread_count() == 4);
    assert(pool.task_capacity() == 8);

    for (int i = 0; i < 8; ++i)
    {
        const bool accepted = pool.submit(
            []()
            {
                __sync_fetch_and_add(&completed, 1);
            }
        );

        assert(accepted == true);
    }

    while (completed < 8)
    {
        // Wait for all tasks to complete
    }

    assert(pool.stop() == true);
    assert(pool.running() == false);
    assert(pool.submit([]() {}) == false);

    return 0;
}