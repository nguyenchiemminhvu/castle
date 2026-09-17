// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file recursive_mutex.hpp
 * @brief Defines a recursive mutex built from Castle's basic mutex and wait-policy model.
 *
 * Use this header when the same thread may need to re-enter a protected region,
 * such as a function calling another helper that locks the same object. Recursive
 * ownership is tracked with a per-thread token and a recursion counter guarded by
 * an internal `basic_mutex`. The type performs no heap allocation, throws no
 * exceptions, uses no RTTI or virtual functions, and depends on no STL
 * synchronization facilities. Lock contention is surfaced only through
 * `try_lock()` returning `false`; `lock()` waits until ownership is acquired.
 *
 * Successful `lock()` and `try_lock()` calls must be balanced with the same
 * number of `unlock()` calls. Unlike `basic_mutex`, `unlock()` from a non-owner
 * or with a zero recursion count is ignored rather than reported.
 *
 * @code
 * #include "castle/sync/recursive_mutex.hpp"
 *
 * void nested(castle::recursive_mutex& lock, int depth)
 * {
 *     lock.lock();
 *
 *     if (depth > 0)
 *     {
 *         nested(lock, depth - 1);
 *     }
 *
 *     lock.unlock();
 * }
 * @endcode
 */
#ifndef CASTLE_SYNC_RECURSIVE_MUTEX_HPP
#define CASTLE_SYNC_RECURSIVE_MUTEX_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/types.hpp"
#include "castle/sync/mutex.hpp"
#include "castle/sync/wait_policy.hpp"

namespace castle
{

namespace detail
{

using thread_id_type = CASTLE_CONST void*;

/**
 * @brief Retrieves the unique identifier for the current thread.
 * @return The unique identifier for the current thread.
 * @note The returned identifier is unique for each thread and remains constant for the thread's lifetime.
 */
inline thread_id_type current_thread_id() CASTLE_NOEXCEPT
{
    static thread_local unsigned char marker = 0U; // The address is unique for each thread-local instance.
    return &marker;
}

/**
 * @brief Compares two thread identifiers for equality.
 * @param lhs The first thread identifier to compare.
 * @param rhs The second thread identifier to compare.
 * @return `true` if the thread identifiers are equal, `false` otherwise.
 */
inline bool thread_id_equal(CASTLE_CONST thread_id_type& lhs, CASTLE_CONST thread_id_type& rhs) CASTLE_NOEXCEPT
{
    return lhs == rhs;
}

}

/**
 * @brief Recursive mutual-exclusion primitive for re-entrant lock ownership by one thread.
 *
 * @tparam WaitPolicy Type providing `static void wait() noexcept`, invoked once
 * per failed retry while waiting for ownership.
 * @note Concurrent access to the mutex object is safe when callers respect the
 * lock/unlock protocol.
 * @warning Each successful acquisition increments the recursion depth and must
 * be matched by one `unlock()` from the owning thread.
 */
template <typename WaitPolicy = spin_wait>
class basic_recursive_mutex
{
    static_assert(detail::has_static_wait<WaitPolicy>::value,
                  "WaitPolicy must provide a public, callable `static void wait() noexcept`.");

public:
    /** @brief Wait policy used by this recursive mutex specialization. */
    using wait_policy_type = WaitPolicy;

    /**
     * @brief Constructs an unlocked recursive mutex.
     * @note Construction initializes the recursion count to zero and reports no
     * errors.
     * @warning The mutex object must outlive every context that may access it.
     */
    basic_recursive_mutex() CASTLE_NOEXCEPT
        : guard_()
        , owner_()
        , count_(0U)
    {
    }

    /**
     * @brief Destroys the recursive mutex object.
     * @note Destruction is trivial and reports no errors.
     * @warning Callers must ensure no thread still relies on this mutex when it
     * is destroyed.
     */
    ~basic_recursive_mutex() CASTLE_DEFAULT;

    /**
     * @brief Copy construction is disabled.
     * @note Recursive mutex state is intentionally non-copyable.
     * @warning Attempting to copy a recursive mutex is a compile-time error.
     */
    basic_recursive_mutex(CASTLE_CONST basic_recursive_mutex&) CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @note Recursive mutex state is intentionally non-copyable.
     * @warning Attempting to copy-assign a recursive mutex is a compile-time error.
     */
    basic_recursive_mutex& operator=(CASTLE_CONST basic_recursive_mutex&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @note Recursive mutex ownership is never transferred implicitly.
     * @warning Attempting to move a recursive mutex is a compile-time error.
     */
    basic_recursive_mutex(basic_recursive_mutex&&) CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @note Recursive mutex ownership is never transferred implicitly.
     * @warning Attempting to move-assign a recursive mutex is a compile-time error.
     */
    basic_recursive_mutex& operator=(basic_recursive_mutex&&) CASTLE_DELETE;

    /**
     * @brief Acquires the mutex, or increments recursion depth when already owned by the caller.
     * @note `WaitPolicy::wait()` runs once per failed retry while another thread
     * owns the mutex. This function has no timeout or error code.
     * @warning Every successful acquisition must be matched by one `unlock()`
     * from the same thread before another thread can acquire the mutex.
     */
    void lock() CASTLE_NOEXCEPT
    {
        CASTLE_CONST detail::thread_id_type self = detail::current_thread_id();

        while (true)
        {
            guard_.lock();

            if ((count_ == 0U) || detail::thread_id_equal(owner_, self)) // LCOV_EXCL_BR_LINE
            {
                owner_ = self;
                ++count_;
                guard_.unlock();
                return;
            }

            // LCOV_EXCL_START
            guard_.unlock();
            WaitPolicy::wait();
            // LCOV_EXCL_STOP
        }
    }

    /**
     * @brief Attempts to acquire the mutex without waiting.
     * @return `true` if the mutex becomes owned by the caller, including a
     * recursive re-acquisition by the current owner; otherwise `false`.
     * @note This is the only acquisition path that surfaces contention directly.
     * @warning A `true` result increases recursion depth and must be balanced
     * with one `unlock()`.
     */
    CASTLE_NODISCARD bool try_lock() CASTLE_NOEXCEPT
    {
        CASTLE_CONST detail::thread_id_type self = detail::current_thread_id();

        guard_.lock();

        if ((count_ == 0U) || detail::thread_id_equal(owner_, self))
        {
            owner_ = self;
            ++count_;
            guard_.unlock();
            return true;
        }

        guard_.unlock();
        return false;
    }

    /**
     * @brief Releases one level of recursive ownership.
     * @note If the caller does not own the mutex, or the recursion count is
     * already zero, this function leaves the state unchanged.
     * @warning The mutex becomes available to other threads only after the final
     * matching `unlock()` reduces the recursion count to zero.
     */
    void unlock() CASTLE_NOEXCEPT
    {
        CASTLE_CONST detail::thread_id_type self = detail::current_thread_id();

        guard_.lock();

        // LCOV_EXCL_START
        if (!detail::thread_id_equal(owner_, self) || (count_ == 0U))
        {
            guard_.unlock();
            return;
        }
        // LCOV_EXCL_STOP

        --count_;

        guard_.unlock();
    }

private:
    castle::basic_mutex<WaitPolicy> guard_;
    detail::thread_id_type owner_;
    size_type count_;
};

/**
 * @brief Convenience alias for `basic_recursive_mutex<spin_wait>`.
 * @note This is the default recursive mutex type for Castle.
 * @warning The default wait policy spins; use a custom `WaitPolicy` when
 * blocked callers should yield to a scheduler.
 */
using recursive_mutex = basic_recursive_mutex<>;

}

#endif