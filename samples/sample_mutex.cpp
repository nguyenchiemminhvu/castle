#include "sample_support.hpp"

#include "castle/sync/mutex.hpp"

// Scenario: protecting a short critical section in a no-OS cooperative application.
int main()
{
    castle::mutex lock;
    CASTLE_SAMPLE_CHECK(lock.try_lock());
    lock.unlock();
    lock.lock();
    lock.unlock();

    // Scenario: swap the default CPU-relax spin for a custom WaitPolicy. See
    // castle/sync/wait_policy.hpp for why castle::mutex cannot itself
    // implement real thread scheduling, and how to plug in a real RTOS
    // scheduler's yield/block primitive (e.g. k_yield(), taskYIELD())
    // instead of spinning.
    struct freertos_like_wait_policy
    {
        static void wait() CASTLE_NOEXCEPT
        {
            // A real integration would call e.g. taskYIELD() here.
        }
    };

    castle::basic_mutex<freertos_like_wait_policy> rtos_lock;
    CASTLE_SAMPLE_CHECK(rtos_lock.try_lock());
    rtos_lock.unlock();

    return 0;
}
