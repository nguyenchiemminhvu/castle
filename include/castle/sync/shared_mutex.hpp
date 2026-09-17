#ifndef CASTLE_SYNC_SHARED_MUTEX_HPP
#define CASTLE_SYNC_SHARED_MUTEX_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/sync/wait_policy.hpp"

#include <stdint.h>

namespace castle
{

// ---------------------------------------------------------------------------
// A bounded reader/writer spinlock.
//
// Multiple readers may hold the lock simultaneously, up to MaxReaders.
// Only one writer may hold the lock, and a writer excludes all readers.
//
// Writer preference:
// Once a writer has announced that it is waiting, new readers are prevented
// from entering. Existing readers are allowed to leave normally, after which
// the waiting writer can acquire the lock.
//
// WaitPolicy is invoked once per failed retry of lock_read()/lock_write();
// it defaults to castle::spin_wait (a portable CPU-relax hint). See
// castle/sync/wait_policy.hpp to plug in a real RTOS scheduler's
// yield/block primitive instead of spinning.
// ---------------------------------------------------------------------------
template <uint32_t MaxReaders, typename WaitPolicy = spin_wait>
class shared_mutex
{
private:
    static_assert(MaxReaders <= 31, "MaxReaders must be at most 31.");
    static_assert(detail::has_static_wait<WaitPolicy>::value,
                  "WaitPolicy must provide a public, callable `static void wait() noexcept`.");

    // The most significant bit indicates that a writer owns the lock.
    static const uint32_t writer_bit_ = 0x80000000UL;

    // Remaining bits contain the active reader count.
    static const uint32_t reader_mask_ = 0x7FFFFFFFUL;

public:
    using wait_policy_type = WaitPolicy;

    // -----------------------------------------------------------------------
    // Construct a new shared mutex in the unlocked state.
    // -----------------------------------------------------------------------
    shared_mutex()
        : state_(0U)
        , waiting_writers_(0U)
    {
        __sync_lock_release(&state_);
        __sync_lock_release(&waiting_writers_);
    }

    ~shared_mutex() CASTLE_DEFAULT;

    // Non-copyable semantics.
    shared_mutex(CASTLE_CONST shared_mutex&) CASTLE_DELETE;
    shared_mutex& operator=(CASTLE_CONST shared_mutex&) CASTLE_DELETE;

    // Non-movable semantics.
    shared_mutex(shared_mutex&&) CASTLE_DELETE;
    shared_mutex& operator=(shared_mutex&&) CASTLE_DELETE;

    // -----------------------------------------------------------------------
    // Acquires a read lock.
    //
    // Multiple readers may hold the lock simultaneously.
    //
    // A new reader will wait while:
    //   - a writer currently owns the lock, or
    //   - one or more writers are waiting.
    //
    // This gives writers preference and prevents continuous reader admission
    // from starving a writer.
    // -----------------------------------------------------------------------
    void lock_read() CASTLE_NOEXCEPT
    {
        while (true) // LCOV_EXCL_BR_LINE
        {
            // Do not admit new readers while a writer is waiting.
            if (__sync_add_and_fetch(&waiting_writers_, 0U) != 0U)
            {
                WaitPolicy::wait();
                continue;
            }

            uint32_t state = __sync_add_and_fetch(&state_, 0U);

            // A writer currently owns the lock.
            if ((state & writer_bit_) != 0U)
            {
                WaitPolicy::wait();
                continue;
            }

            uint32_t readers = state & reader_mask_;

            // The compile-time reader limit has been reached.
            if (readers >= MaxReaders)
            {
                WaitPolicy::wait();
                continue;
            }

            // Attempt to add this reader atomically.
            //
            // If a writer acquires the lock, or another reader changes the
            // count, the CAS fails and the operation is retried.
            if (__sync_bool_compare_and_swap(
                    &state_,
                    state,
                    state + 1U))
            {
                return;
            }

            WaitPolicy::wait();
        }
    }

    // -----------------------------------------------------------------------
    // Attempts to acquire a read lock without blocking.
    //
    // Returns true if the read lock was acquired.
    // Returns false otherwise.
    // -----------------------------------------------------------------------
    CASTLE_NODISCARD bool try_lock_read() CASTLE_NOEXCEPT
    {
        // A waiting writer has priority over a new reader.
        if (__sync_add_and_fetch(&waiting_writers_, 0U) != 0U)
        {
            return false;
        }

        uint32_t state = __sync_add_and_fetch(&state_, 0U);

        // A writer owns the lock.
        if ((state & writer_bit_) != 0U)
        {
            return false;
        }

        uint32_t readers = state & reader_mask_;

        // Maximum simultaneous readers reached.
        if (readers >= MaxReaders)
        {
            return false;
        }

        return __sync_bool_compare_and_swap(
            &state_,
            state,
            state + 1U);
    }

    // -----------------------------------------------------------------------
    // Releases a read lock.
    // -----------------------------------------------------------------------
    void unlock_read() CASTLE_NOEXCEPT
    {
        __sync_fetch_and_sub(&state_, 1U);
    }

    // -----------------------------------------------------------------------
    // Acquires the exclusive write lock.
    //
    // A writer announces itself before waiting. This prevents new readers
    // from entering and allows the existing readers to drain.
    // -----------------------------------------------------------------------
    void lock_write() CASTLE_NOEXCEPT
    {
        __sync_add_and_fetch(&waiting_writers_, 1U);

        while (true) // LCOV_EXCL_BR_LINE
        {
            uint32_t state = __sync_add_and_fetch(&state_, 0U);

            // The lock is available only when there are no readers and no
            // writer currently owning the lock.
            if (state != 0U)
            {
                WaitPolicy::wait();
                continue;
            }

            // Atomically transition:
            //
            //     no owner -> writer owns lock
            //
            // A reader cannot enter between the state check and this CAS.
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
    }

    // -----------------------------------------------------------------------
    // Attempts to acquire the exclusive write lock without blocking.
    //
    // A writer is considered waiting while the operation is being attempted,
    // so new readers are temporarily prevented from entering.
    // -----------------------------------------------------------------------
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

    // -----------------------------------------------------------------------
    // Releases the exclusive write lock.
    // -----------------------------------------------------------------------
    void unlock_write() CASTLE_NOEXCEPT
    {
        __sync_lock_release(&state_);
    }

private:
    CASTLE_VOLATILE uint32_t state_;           ///< Writer bit + active reader count
    CASTLE_VOLATILE uint32_t waiting_writers_; ///< Number of writers waiting
};

} // namespace castle

#endif // CASTLE_SYNC_SHARED_MUTEX_HPP
