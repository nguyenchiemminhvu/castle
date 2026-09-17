#include "sample_support.hpp"

#include "castle/sync/scoped_mutex.hpp"

// Scenario: RAII locking of a very short critical section while preserving exception-free code.
int main()
{
    castle::mutex lock;
    {
        castle::scoped_mutex guard(lock);
        (void)guard;
    }

    // Scenario: a custom WaitPolicy carries through from the guarded
    // castle::basic_mutex to its castle::basic_scoped_mutex guard. See
    // castle/sync/wait_policy.hpp for the RTOS-integration rationale.
    struct freertos_like_wait_policy
    {
        static void wait() CASTLE_NOEXCEPT
        {
            // A real integration would call e.g. taskYIELD() here.
        }
    };

    castle::basic_mutex<freertos_like_wait_policy> rtos_lock;
    {
        castle::basic_scoped_mutex<freertos_like_wait_policy> guard(rtos_lock);
        (void)guard;
    }

    return 0;
}
