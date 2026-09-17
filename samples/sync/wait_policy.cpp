#include "sample_support.hpp"

#include "castle/sync/mutex.hpp"
#include "castle/sync/wait_policy.hpp"

#include <stdint.h>

static uint32_t wait_calls = 0U;

struct counting_wait_policy
{
    static void wait() CASTLE_NOEXCEPT
    {
        ++wait_calls;
    }
};

int main()
{
    castle::spin_wait::wait();

    counting_wait_policy::wait();
    CASTLE_SAMPLE_CHECK(wait_calls == 1U);

    castle::basic_mutex<counting_wait_policy> lock;
    CASTLE_SAMPLE_CHECK(lock.try_lock());
    lock.unlock();

    return 0;
}
