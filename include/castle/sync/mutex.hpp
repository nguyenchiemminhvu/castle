#ifndef CASTLE_SYNC_MUTEX_HPP
#define CASTLE_SYNC_MUTEX_HPP

#include "castle/core/compiler.hpp"
#include "castle/sync/wait_policy.hpp"

#include <stdint.h>

namespace castle
{

// ---------------------------------------------------------------------------
// A basic spinlock-based mutex implementation using GCC's legacy __sync built-ins.
//
// Suitable for low-level embedded resource synchronization where standard OS primitives 
// are unavailable.
//
// WaitPolicy is invoked once per failed lock() retry; it defaults to
// castle::spin_wait (a portable CPU-relax hint). See
// castle/sync/wait_policy.hpp to plug in a real RTOS scheduler's
// yield/block primitive instead of spinning.
// ---------------------------------------------------------------------------
template <typename WaitPolicy = spin_wait>
class basic_mutex
{
    static_assert(detail::has_static_wait<WaitPolicy>::value,
                  "WaitPolicy must provide a public, callable `static void wait() noexcept`.");

public:
    using wait_policy_type = WaitPolicy;

    // ---------------------------------------------------------------------------
    // Construct a new mutex object and guarantee it starts in an unlocked state.
    // ---------------------------------------------------------------------------
    basic_mutex() : flag_(0U)
    {
        __sync_lock_release(&flag_);
    }

    ~basic_mutex() CASTLE_DEFAULT;

    // Non-copyable semantics matching the library safety goals
    basic_mutex(CASTLE_CONST basic_mutex&) CASTLE_DELETE;
    basic_mutex& operator=(CASTLE_CONST basic_mutex&) CASTLE_DELETE;

    // Non-movable semantics to prevent accidental transfer of lock ownership
    basic_mutex(basic_mutex&&) CASTLE_DELETE;
    basic_mutex& operator=(basic_mutex&&) CASTLE_DELETE;

    // ---------------------------------------------------------------------------
    // Acquires the mutex lock. Blocks (busy-waits) until the lock becomes available.
    // ---------------------------------------------------------------------------
    void lock() CASTLE_NOEXCEPT
    {
        while (__sync_lock_test_and_set(&flag_, 1U)) // LCOV_EXCL_BR_LINE
        {
            WaitPolicy::wait();
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

// ---------------------------------------------------------------------------
// castle::mutex is basic_mutex<> using castle::spin_wait, castle's
// zero-dependency, bare-metal-safe default WaitPolicy. See
// castle/sync/wait_policy.hpp to plug in a real RTOS scheduler's
// yield/block primitive instead of spinning.
// ---------------------------------------------------------------------------
using mutex = basic_mutex<>;

} // namespace castle

#endif // CASTLE_SYNC_MUTEX_HPP
