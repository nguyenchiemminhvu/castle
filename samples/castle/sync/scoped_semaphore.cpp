#include "sample_support.hpp"

#include "castle/sync/scoped_semaphore.hpp"

struct counting_wait_policy
{
    static void wait() CASTLE_NOEXCEPT
    {
    }
};

int main()
{
    castle::semaphore<2> pool(2U);

    CASTLE_SAMPLE_CHECK(pool.count() == 2U);

    {
        castle::scoped_semaphore<2> first(pool);
        (void)first;
        CASTLE_SAMPLE_CHECK(pool.count() == 1U);

        {
            castle::scoped_semaphore<2> second(pool);
            (void)second;
            CASTLE_SAMPLE_CHECK(pool.count() == 0U);
            CASTLE_SAMPLE_CHECK(!pool.try_acquire());
        }

        CASTLE_SAMPLE_CHECK(pool.count() == 1U);
    }

    CASTLE_SAMPLE_CHECK(pool.count() == 2U);

    castle::semaphore<1, counting_wait_policy> custom_pool(1U);

    {
        castle::scoped_semaphore<1, counting_wait_policy> guard(custom_pool);
        (void)guard;
        CASTLE_SAMPLE_CHECK(custom_pool.count() == 0U);
    }

    CASTLE_SAMPLE_CHECK(custom_pool.count() == 1U);

    return 0;
}
