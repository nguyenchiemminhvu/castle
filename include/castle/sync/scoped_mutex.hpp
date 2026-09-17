// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file scoped_mutex.hpp
 * @brief Defines an RAII guard that locks a Castle mutex for the current scope.
 *
 * Use this header when a `basic_mutex` should be acquired on construction and
 * automatically released on every scope exit path. The guard performs no heap
 * allocation, throws no exceptions, uses no RTTI or virtual functions, and adds
 * no ownership transfer semantics. Construction has no error code or timeout; it
 * simply calls `basic_mutex::lock()` and therefore waits until the mutex is
 * acquired.
 *
 * The guard is strictly scoped: it must not outlive the referenced mutex, and it
 * is neither copyable nor movable. Because the underlying mutex is non-recursive,
 * constructing a second guard on the same mutex from the same holder waits
 * indefinitely.
 *
 * @code
 * #include "castle/sync/scoped_mutex.hpp"
 *
 * int main()
 * {
 *     castle::mutex lock;
 *
 *     {
 *         castle::scoped_mutex guard(lock);
 *         (void)guard;
 *     }
 * }
 * @endcode
 */
#ifndef CASTLE_SYNC_SCOPED_MUTEX_HPP
#define CASTLE_SYNC_SCOPED_MUTEX_HPP

#include "castle/sync/mutex.hpp"

namespace castle
{

/**
 * @brief RAII guard that locks a `basic_mutex` in its constructor and unlocks it in its destructor.
 *
 * @tparam WaitPolicy Wait policy used by the guarded `basic_mutex`.
 * @note The guard is deterministic and allocation-free.
 * @warning The guard must not outlive the referenced mutex object.
 */
template <typename WaitPolicy = spin_wait>
class basic_scoped_mutex
{
public:
    /**
     * @brief Locks the referenced mutex for the lifetime of this guard.
     * @param mutex Mutex to acquire immediately.
     * @note This constructor directly calls `mutex.lock()` and therefore has no
     * separate failure signal.
     * @warning If the current holder already owns a non-recursive mutex,
     * constructing this guard on the same mutex waits indefinitely.
     */
    explicit basic_scoped_mutex(basic_mutex<WaitPolicy>& mutex) CASTLE_NOEXCEPT
        : mutex_(mutex)
    {
        mutex_.lock();
    }

    /**
     * @brief Releases the guarded mutex.
     * @note Destruction always performs exactly one `unlock()` call.
     * @warning Destroy the guard only while the referenced mutex object is still valid.
     */
    ~basic_scoped_mutex() CASTLE_NOEXCEPT
    {
        mutex_.unlock();
    }

    /**
     * @brief Copy construction is disabled.
     * @note Scoped guards are intentionally non-copyable.
     * @warning Attempting to copy a scoped guard is a compile-time error.
     */
    basic_scoped_mutex(CASTLE_CONST basic_scoped_mutex&) CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @note Scoped guards are intentionally non-copyable.
     * @warning Attempting to copy-assign a scoped guard is a compile-time error.
     */
    basic_scoped_mutex& operator=(CASTLE_CONST basic_scoped_mutex&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @note Scoped guards never transfer lock responsibility implicitly.
     * @warning Attempting to move a scoped guard is a compile-time error.
     */
    basic_scoped_mutex(basic_scoped_mutex&&) CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @note Scoped guards never transfer lock responsibility implicitly.
     * @warning Attempting to move-assign a scoped guard is a compile-time error.
     */
    basic_scoped_mutex& operator=(basic_scoped_mutex&&) CASTLE_DELETE;

private:
    basic_mutex<WaitPolicy>& mutex_;
};

/**
 * @brief Convenience alias for `basic_scoped_mutex<spin_wait>`.
 * @note This guard is intended for use with `castle::mutex`.
 * @warning The referenced mutex must remain alive until the guard is destroyed.
 */
using scoped_mutex = basic_scoped_mutex<>;

}

#endif
