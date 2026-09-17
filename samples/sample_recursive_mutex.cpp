#include "sample_support.hpp"

#include "castle/sync/recursive_mutex.hpp"

// Scenario: a function and the helper it calls both need to hold the same
// lock, e.g. a public API entry point that re-enters through a private
// helper while still protecting the same critical section.
static castle::recursive_mutex lock;

static void inner(int depth)
{
    lock.lock();

    if (depth > 0)
    {
        inner(depth - 1);
    }

    lock.unlock();
}

int main()
{
    CASTLE_SAMPLE_CHECK(lock.try_lock());
    lock.unlock();

    inner(3);

    // Scenario: a custom WaitPolicy plugs into castle::basic_recursive_mutex
    // the same way as castle::basic_mutex. See castle/sync/wait_policy.hpp
    // for the RTOS-integration rationale.
    struct freertos_like_wait_policy
    {
        static void wait() CASTLE_NOEXCEPT
        {
            // A real integration would call e.g. taskYIELD() here.
        }
    };

    castle::basic_recursive_mutex<freertos_like_wait_policy> rtos_lock;
    rtos_lock.lock();
    rtos_lock.lock();
    rtos_lock.unlock();
    rtos_lock.unlock();

    return 0;
}
