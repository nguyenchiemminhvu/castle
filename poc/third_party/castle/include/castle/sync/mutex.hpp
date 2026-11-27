// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mutex.hpp
 * @brief Defines a non-recursive spinlock-style mutex with a pluggable wait policy.
 *
 * Use this header when one execution context must exclude every other context from
 * a short critical section and a busy-wait or scheduler-yield retry loop is
 * acceptable. The mutex owns all state internally, performs no heap allocation,
 * throws no exceptions, uses no RTTI or virtual functions, and depends on no STL
 * synchronization facilities. Lock acquisition failure is surfaced only through
 * `try_lock()` returning `false`; `lock()` has no timeout or error code and waits
 * until the lock is obtained.
 *
 * The mutex is not recursive. Calling `lock()` again from the same execution
 * context while it already holds the mutex waits indefinitely, and `unlock()` does
 * not diagnose protocol misuse.
 *
 * @code
 * #include "castle/sync/mutex.hpp"
 *
 * int main()
 * {
 *     castle::mutex lock;
 *
 *     if (lock.try_lock())
 *     {
 *         lock.unlock();
 *     }
 *
 *     lock.lock();
 *     lock.unlock();
 * }
 * @endcode
 */
#ifndef CASTLE_SYNC_MUTEX_HPP
#define CASTLE_SYNC_MUTEX_HPP

#include "castle/core/compiler.hpp"
#include "castle/sync/wait_policy.hpp"

#include <stdint.h>

namespace castle
{

/**
 * @brief Mutual-exclusion primitive implemented as a spinlock with a compile-time wait policy.
 *
 * @tparam WaitPolicy Type providing `static void wait() noexcept`, invoked once
 * per failed `lock()` retry.
 * @note This type is safe to access concurrently when callers obey the locking
 * protocol.
 * @warning The mutex is non-recursive and does not track ownership. Re-locking
 * from the current holder or unlocking without a matching successful lock is not
 * diagnosed.
 */
template <typename WaitPolicy = spin_wait>
class basic_mutex
{
    static_assert(detail::has_static_wait<WaitPolicy>::value,
                  "WaitPolicy must provide a public, callable `static void wait() noexcept`.");

public:
    /** @brief Wait policy used by this mutex specialization. */
    using wait_policy_type = WaitPolicy;

    /**
     * @brief Constructs an unlocked mutex.
     * @note Construction performs no allocation and cannot fail.
     * @warning The object must have static, automatic, or otherwise stable
     * storage for the full time it may be shared between execution contexts.
     */
    basic_mutex() : flag_(0U)
    {
        __sync_lock_release(&flag_);
    }

    /**
     * @brief Destroys the mutex object.
     * @note Destruction is trivial and reports no errors.
     * @warning Callers must ensure no execution context still relies on this
     * mutex when it is destroyed.
     */
    ~basic_mutex() CASTLE_DEFAULT;

    /**
     * @brief Copy construction is disabled.
     * @note Mutex state is intentionally non-copyable.
     * @warning Attempting to copy a mutex is a compile-time error.
     */
    basic_mutex(CASTLE_CONST basic_mutex&) CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @note Mutex state is intentionally non-copyable.
     * @warning Attempting to copy-assign a mutex is a compile-time error.
     */
    basic_mutex& operator=(CASTLE_CONST basic_mutex&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @note Ownership is never transferred implicitly.
     * @warning Attempting to move a mutex is a compile-time error.
     */
    basic_mutex(basic_mutex&&) CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @note Ownership is never transferred implicitly.
     * @warning Attempting to move-assign a mutex is a compile-time error.
     */
    basic_mutex& operator=(basic_mutex&&) CASTLE_DELETE;

    /**
     * @brief Acquires the mutex, waiting until it becomes available.
     * @note `WaitPolicy::wait()` runs once per failed retry. This function has no
     * timeout, error code, or failure callback.
     * @warning Because the mutex is non-recursive, calling `lock()` again before
     * `unlock()` from the current holder waits indefinitely.
     */
    void lock() CASTLE_NOEXCEPT
    {
        while (__sync_lock_test_and_set(&flag_, 1U))
        {
            WaitPolicy::wait();
        }
    }

    /**
     * @brief Attempts to acquire the mutex without waiting.
     * @return `true` if the mutex was acquired; otherwise `false`.
     * @note `try_lock()` is the only lock operation that surfaces contention to
     * the caller directly.
     * @warning A `true` result must be balanced with exactly one `unlock()`.
     */
    CASTLE_NODISCARD bool try_lock() CASTLE_NOEXCEPT
    {
        return (__sync_lock_test_and_set(&flag_, 1U) == 0U);
    }

    /**
     * @brief Releases the mutex.
     * @note This function reports no status and performs no ownership checks.
     * @warning Only call `unlock()` after a successful `lock()` or `try_lock()`
     * on the same mutex. Misuse is not diagnosed.
     */
    void unlock() CASTLE_NOEXCEPT
    {
        __sync_lock_release(&flag_);
    }

private:
    unsigned char flag_;
};

/**
 * @brief Convenience alias for `basic_mutex<spin_wait>`.
 * @note This is the library's default busy-wait mutex type.
 * @warning The default wait policy spins; integrate a different `WaitPolicy`
 * when blocked callers should yield to a scheduler instead.
 */
using mutex = basic_mutex<>;

}

#endif
