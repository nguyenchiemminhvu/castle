#include "sample_support.hpp"

#include "castle/events/thread_pool.hpp"

int main()
{
    using pool_type = castle::events::thread_pool<1U, 3U, 64U>;

    pool_type pool;
    volatile int completed = 0;
    volatile int sum = 0;

    CASTLE_SAMPLE_CHECK(pool.running());
    CASTLE_SAMPLE_CHECK(pool.queued() == 0U);
    CASTLE_SAMPLE_CHECK(pool.available() == pool.task_capacity());
    CASTLE_SAMPLE_CHECK(pool.thread_count() == 1U);
    CASTLE_SAMPLE_CHECK(pool.task_capacity() == 3U);

    pool_type::task_type explicit_task(
        [&completed, &sum]()
        {
            __sync_fetch_and_add(&sum, 1);
            __sync_fetch_and_add(&completed, 1);
        }
    );

    CASTLE_SAMPLE_CHECK(pool.submit(CASTLE_MOVE(explicit_task)));
    CASTLE_SAMPLE_CHECK(pool.submit(
        [&completed, &sum]()
        {
            __sync_fetch_and_add(&sum, 2);
            __sync_fetch_and_add(&completed, 1);
        }
    ));

    while (__sync_fetch_and_add(&completed, 0) != 2)
    {
    }

    CASTLE_SAMPLE_CHECK(__sync_fetch_and_add(&sum, 0) == 3);
    CASTLE_SAMPLE_CHECK(pool.queued() == 0U);
    CASTLE_SAMPLE_CHECK(pool.available() == pool.task_capacity());

    CASTLE_SAMPLE_CHECK(pool.stop());
    CASTLE_SAMPLE_CHECK(!pool.running());
    CASTLE_SAMPLE_CHECK(!pool.submit([]() {}));
    CASTLE_SAMPLE_CHECK(pool.stop());

    return 0;
}
