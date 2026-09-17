// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file shared_mutex.hpp
 * @brief Defines a bounded reader/writer mutex with writer preference and a pluggable wait policy.
 *
 * Use this header when many readers may safely observe shared state concurrently
 * but writers need exclusive access. The primitive performs no heap allocation,
 * throws no exceptions, uses no RTTI or virtual functions, and depends on no STL
 * synchronization facilities. `lock_read()` and `lock_write()` have no timeout or
 * error code and wait until they succeed; `try_lock_read()` and `try_lock_write()`
 * surface contention through boolean return values.
 *
 * The lock is not recursive. New readers are blocked while one or more writers are
 * waiting, which gives writers preference and helps prevent writer starvation.
 * Callers must strictly balance successful lock operations with the matching
 * unlock operation because protocol misuse is not diagnosed.
 *
 * @code
 * #include "castle/sync/shared_mutex.hpp"
 *
 * int main()
 * {
 *     castle::shared_mutex<2> lock;
 *
 *     lock.lock_read();
 *     lock.unlock_read();
 *
 *     lock.lock_write();
 *     lock.unlock_write();
 * }
 * @endcode
 */
#ifndef CASTLE_SYNC_SHARED_MUTEX_HPP
#define CASTLE_SYNC_SHARED_MUTEX_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/sync/wait_policy.hpp"

#include <stdint.h>

namespace castle
{

/**
 * @brief Reader/writer mutex with bounded concurrent readers and writer preference.
 *
 * @tparam MaxReaders Maximum number of simultaneous readers representable by the
 * mutex. Values above `31` are rejected at compile time; a value of `0` prevents
 * successful read locking.
 * @tparam WaitPolicy Type providing `static void wait() noexcept`, invoked once
 * per failed blocking retry.
 * @note Concurrent access to the mutex object is safe when callers obey the read
 * and write locking protocol.
 * @warning The lock is non-recursive and does not track which reader or writer
 * owns it. Unbalanced unlocks corrupt the synchronization state.
 */
template <uint32_t MaxReaders, typename WaitPolicy = spin_wait>
class shared_mutex
{
private:
    static_assert(MaxReaders <= 31, "MaxReaders must be at most 31.");
    static_assert(detail::has_static_wait<WaitPolicy>::value,
                  "WaitPolicy must provide a public, callable `static void wait() noexcept`.");

    static const uint32_t writer_bit_ = 0x80000000UL;
    static const uint32_t reader_mask_ = 0x7FFFFFFFUL;

public:
    /** @brief Wait policy used by this shared mutex specialization. */
    using wait_policy_type = WaitPolicy;

    /**
     * @brief Constructs an unlocked shared mutex.
     * @note Both the reader/writer state and waiting-writer count start at zero.
     * @warning The mutex object must outlive every context that may access it.
     */
    shared_mutex()
        : state_(0U)
        , waiting_writers_(0U)
    {
        __sync_lock_release(&state_);
        __sync_lock_release(&waiting_writers_);
    }

    /**
     * @brief Destroys the shared mutex object.
     * @note Destruction is trivial and reports no errors.
     * @warning Callers must ensure no context still relies on this mutex when it
     * is destroyed.
     */
    ~shared_mutex() CASTLE_DEFAULT;

    /**
     * @brief Copy construction is disabled.
     * @note Shared mutex state is intentionally non-copyable.
     * @warning Attempting to copy a shared mutex is a compile-time error.
     */
    shared_mutex(CASTLE_CONST shared_mutex&) CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @note Shared mutex state is intentionally non-copyable.
     * @warning Attempting to copy-assign a shared mutex is a compile-time error.
     */
    shared_mutex& operator=(CASTLE_CONST shared_mutex&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @note Lock state is never transferred implicitly.
     * @warning Attempting to move a shared mutex is a compile-time error.
     */
    shared_mutex(shared_mutex&&) CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @note Lock state is never transferred implicitly.
     * @warning Attempting to move-assign a shared mutex is a compile-time error.
     */
    shared_mutex& operator=(shared_mutex&&) CASTLE_DELETE;

    /**
     * @brief Acquires a shared read lock.
     * @note New readers wait while a writer owns the lock or while one or more
     * writers are queued, so writers are given preference.
     * @warning A successful read lock must be matched by exactly one
     * `unlock_read()`. If `MaxReaders` is `0`, this function never succeeds.
     */
    void lock_read() CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        while (true)
        {
            if (__sync_add_and_fetch(&waiting_writers_, 0U) != 0U)
            {
                WaitPolicy::wait();
                continue;
            }

            uint32_t state = __sync_add_and_fetch(&state_, 0U);

            if ((state & writer_bit_) != 0U)
            {
                WaitPolicy::wait();
                continue;
            }

            uint32_t readers = state & reader_mask_;

            if (readers >= MaxReaders)
            {
                WaitPolicy::wait();
                continue;
            }

            if (__sync_bool_compare_and_swap(
                    &state_,
                    state,
                    state + 1U))
            {
                return;
            }

            WaitPolicy::wait();
        }
        // LCOV_EXCL_STOP
    }

    /**
     * @brief Attempts to acquire a shared read lock without waiting.
     * @return `true` if a read lock was acquired; otherwise `false`.
     * @note The call fails while a writer owns the mutex, while any writer is
     * waiting, or when the reader count has reached `MaxReaders`.
     * @warning A `true` result must be balanced with one `unlock_read()`.
     */
    CASTLE_NODISCARD bool try_lock_read() CASTLE_NOEXCEPT
    {
        if (__sync_add_and_fetch(&waiting_writers_, 0U) != 0U)
        {
            return false;
        }

        uint32_t state = __sync_add_and_fetch(&state_, 0U);

        if ((state & writer_bit_) != 0U)
        {
            return false;
        }

        uint32_t readers = state & reader_mask_;

        if (readers >= MaxReaders)
        {
            return false;
        }

        return __sync_bool_compare_and_swap(
            &state_,
            state,
            state + 1U);
    }

    /**
     * @brief Releases one shared read lock.
     * @note This function reports no status and performs no ownership checks.
     * @warning Only call `unlock_read()` after a successful `lock_read()` or
     * `try_lock_read()`. An unbalanced call corrupts the internal reader count.
     */
    void unlock_read() CASTLE_NOEXCEPT
    {
        __sync_fetch_and_sub(&state_, 1U);
    }

    /**
     * @brief Acquires the exclusive write lock.
     * @note Writers announce themselves before waiting so new readers stop
     * entering while existing readers drain.
     * @warning A successful write lock must be matched by exactly one
     * `unlock_write()`, and recursive write locking is not supported.
     */
    void lock_write() CASTLE_NOEXCEPT
    {
        __sync_add_and_fetch(&waiting_writers_, 1U);

        // LCOV_EXCL_START
        while (true)
        {
            uint32_t state = __sync_add_and_fetch(&state_, 0U);

            if (state != 0U)
            {
                WaitPolicy::wait();
                continue;
            }

            if (__sync_bool_compare_and_swap(
                    &state_,
                    0U,
                    writer_bit_))
            {
                __sync_fetch_and_sub(&waiting_writers_, 1U);
                return;
            }

            WaitPolicy::wait();
        }
        // LCOV_EXCL_STOP
    }

    /**
     * @brief Attempts to acquire the exclusive write lock without waiting.
     * @return `true` if the write lock was acquired; otherwise `false`.
     * @note The mutex temporarily counts this caller as a waiting writer during
     * the attempt so that new readers are not admitted mid-transition.
     * @warning A `true` result must be balanced with one `unlock_write()`.
     */
    CASTLE_NODISCARD bool try_lock_write() CASTLE_NOEXCEPT
    {
        __sync_add_and_fetch(&waiting_writers_, 1U);

        bool acquired = __sync_bool_compare_and_swap(
            &state_,
            0U,
            writer_bit_);

        __sync_fetch_and_sub(&waiting_writers_, 1U);

        return acquired;
    }

    /**
     * @brief Releases the exclusive write lock.
     * @note This function reports no status and performs no ownership checks.
     * @warning Only call `unlock_write()` after a successful `lock_write()` or
     * `try_lock_write()`. An unbalanced call corrupts the internal state.
     */
    void unlock_write() CASTLE_NOEXCEPT
    {
        __sync_lock_release(&state_);
    }

private:
    CASTLE_VOLATILE uint32_t state_;
    CASTLE_VOLATILE uint32_t waiting_writers_;
};

}

#endif
