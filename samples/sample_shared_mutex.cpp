#include "castle/sync/shared_mutex.hpp"

#include <stdint.h>
#include <unistd.h>

static castle::shared_mutex<4> mutex;

static uint32_t shared_value = 0U;

// ---------------------------------------------------------------------------
// Scenario: swap the default CPU-relax spin for a custom WaitPolicy. See
// castle/sync/wait_policy.hpp for why castle::shared_mutex cannot itself
// implement real thread scheduling, and how to plug in a real RTOS
// scheduler's yield/block primitive (e.g. k_yield(), taskYIELD()) instead
// of spinning.
// ---------------------------------------------------------------------------
struct freertos_like_wait_policy
{
    static void wait() CASTLE_NOEXCEPT
    {
        // A real integration would call e.g. taskYIELD() here.
    }
};

static castle::shared_mutex<4, freertos_like_wait_policy> rtos_mutex;

// ---------------------------------------------------------------------------
// Reader
// ---------------------------------------------------------------------------
static void reader()
{
    mutex.lock_read();

    uint32_t value = shared_value;

    (void)value;
    usleep(100); // Simulate some work while holding the read lock

    mutex.unlock_read();
}

// ---------------------------------------------------------------------------
// Writer
// ---------------------------------------------------------------------------
static void writer()
{
    mutex.lock_write();

    shared_value = 42U;
    usleep(100); // Simulate some work while holding the write lock

    mutex.unlock_write();
}

int main()
{
    // Multiple readers may execute concurrently.
    reader();
    reader();
    reader();
    reader();

    // Writer obtains exclusive access.
    writer();

    // Readers can enter again after the writer releases the lock.
    reader();

    // Custom WaitPolicy usage: identical API, just a different template arg.
    rtos_mutex.lock_read();
    rtos_mutex.unlock_read();

    return 0;
}
