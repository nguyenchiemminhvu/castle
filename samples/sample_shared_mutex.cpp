#include "castle/mutex/shared_mutex.hpp"

#include <stdint.h>
#include <unistd.h>

static castle::shared_mutex<4> mutex;

static uint32_t shared_value = 0U;

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

    return 0;
}
