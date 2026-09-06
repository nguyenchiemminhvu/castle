#ifndef CASTLE_MUTEX_MUTEX_H
#define CASTLE_MUTEX_MUTEX_H

#include "castle/core/compiler.h"

#include <stdint.h>

namespace castle
{

// ---------------------------------------------------------------------------
// A basic spinlock-based mutex implementation using GCC's legacy __sync built-ins.
//
// Suitable for low-level embedded resource synchronization where standard OS primitives 
// are unavailable.
// ---------------------------------------------------------------------------
class mutex
{
public:
    // ---------------------------------------------------------------------------
    // Construct a new mutex object and guarantee it starts in an unlocked state.
    // ---------------------------------------------------------------------------
    mutex() : flag_(0U)
    {
        __sync_lock_release(&flag_);
    }

    ~mutex() CASTLE_DEFAULT;

    // Non-copyable semantics matching the library safety goals
    mutex(CASTLE_CONST mutex&) CASTLE_DELETE;
    mutex& operator=(CASTLE_CONST mutex&) CASTLE_DELETE;

    // Non-movable semantics to prevent accidental transfer of lock ownership
    mutex(mutex&&) CASTLE_DELETE;
    mutex& operator=(mutex&&) CASTLE_DELETE;

    // ---------------------------------------------------------------------------
    // Acquires the mutex lock. Blocks (busy-waits) until the lock becomes available.
    // ---------------------------------------------------------------------------
    void lock() CASTLE_NOEXCEPT
    {
        while (__sync_lock_test_and_set(&flag_, 1U))
        {
            // Spin/Busy-wait loop
        }
    }

    // ---------------------------------------------------------------------------
    // Tries to acquire the mutex lock without blocking.
    // return true if the lock was successfully acquired, false otherwise.
    // ---------------------------------------------------------------------------
    CASTLE_NODISCARD bool try_lock() CASTLE_NOEXCEPT
    {
        return (__sync_lock_test_and_set(&flag_, 1U) == 0U);
    }

    // ---------------------------------------------------------------------------
    // Releases the mutex lock.
    // ---------------------------------------------------------------------------
    void unlock() CASTLE_NOEXCEPT
    {
        __sync_lock_release(&flag_);
    }

private:
    unsigned char flag_; ///< Lock state indicator (0 = unlocked, 1 = locked)
};

} // namespace castle

#endif // CASTLE_MUTEX_MUTEX_H
