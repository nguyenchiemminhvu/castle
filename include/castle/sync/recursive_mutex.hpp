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

// The address of a thread-local object is unique per thread: the linker/runtime
// gives every thread its own instance, so no two threads ever observe the same
// address. This needs no OS thread-id API, no heap, and no STL.
inline thread_id_type current_thread_id() CASTLE_NOEXCEPT
{
    static thread_local unsigned char marker = 0U;
    return &marker;
}

inline bool thread_id_equal(CASTLE_CONST thread_id_type& lhs, CASTLE_CONST thread_id_type& rhs) CASTLE_NOEXCEPT
{
    return lhs == rhs;
}

} // namespace detail

// ---------------------------------------------------------------------------
// A recursive spinlock-based mutex.
//
// The owning thread may acquire the lock multiple times; it must call
// unlock() the same number of times before another thread can acquire it.
//
// Ownership and recursion count are guarded by a plain castle::basic_mutex,
// so the implementation stays free of heap allocation, exceptions, virtual
// dispatch, RTTI, and STL containers/threading facilities.
//
// WaitPolicy is invoked once per failed lock() retry (both for the internal
// guard and for the outer "another thread owns this" spin); it defaults to
// castle::spin_wait (a portable CPU-relax hint). See
// castle/sync/wait_policy.hpp to plug in a real RTOS scheduler's
// yield/block primitive instead of spinning.
// ---------------------------------------------------------------------------
template <typename WaitPolicy = spin_wait>
class basic_recursive_mutex
{
    static_assert(detail::has_static_wait<WaitPolicy>::value,
                  "WaitPolicy must provide a public, callable `static void wait() noexcept`.");

public:
    using wait_policy_type = WaitPolicy;

    // ---------------------------------------------------------------------------
    // Construct a new basic_recursive_mutex object and guarantee it starts unlocked.
    // ---------------------------------------------------------------------------
    basic_recursive_mutex() CASTLE_NOEXCEPT
        : guard_()
        , owner_()
        , count_(0U)
    {
    }

    ~basic_recursive_mutex() CASTLE_DEFAULT;

    // Non-copyable semantics matching the library safety goals
    basic_recursive_mutex(CASTLE_CONST basic_recursive_mutex&) CASTLE_DELETE;
    basic_recursive_mutex& operator=(CASTLE_CONST basic_recursive_mutex&) CASTLE_DELETE;

    // Non-movable semantics to prevent accidental transfer of lock ownership
    basic_recursive_mutex(basic_recursive_mutex&&) CASTLE_DELETE;
    basic_recursive_mutex& operator=(basic_recursive_mutex&&) CASTLE_DELETE;

    // ---------------------------------------------------------------------------
    // Acquires the mutex lock. Blocks (busy-waits) until the calling thread either
    // owns the lock already or the lock becomes free.
    // ---------------------------------------------------------------------------
    void lock() CASTLE_NOEXCEPT
    {
        CASTLE_CONST detail::thread_id_type self = detail::current_thread_id();

        while (true) // LCOV_EXCL_BR_LINE
        {
            guard_.lock();

            if ((count_ == 0U) || detail::thread_id_equal(owner_, self))
            {
                owner_ = self;
                ++count_;
                guard_.unlock();
                return;
            }

            guard_.unlock();

            // Some other thread owns the lock; spin/busy-wait and retry.
            WaitPolicy::wait();
        }
    }

    // ---------------------------------------------------------------------------
    // Tries to acquire the mutex lock without blocking.
    // return true if the calling thread now owns the lock, false otherwise.
    // ---------------------------------------------------------------------------
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

    // ---------------------------------------------------------------------------
    // Releases one level of recursive ownership. The lock becomes available to
    // other threads once the count returns to zero.
    //
    // A call from a non-owning thread, or when the count is already zero, is
    // ignored rather than corrupting the recursion count.
    // ---------------------------------------------------------------------------
    void unlock() CASTLE_NOEXCEPT
    {
        CASTLE_CONST detail::thread_id_type self = detail::current_thread_id();

        guard_.lock();

        if (!detail::thread_id_equal(owner_, self) || (count_ == 0U))
        {
            guard_.unlock();
            return;
        }

        --count_;

        guard_.unlock();
    }

private:
    castle::basic_mutex<WaitPolicy> guard_; ///< Protects owner_ and count_
    detail::thread_id_type owner_;          ///< Valid only while count_ > 0
    size_type count_;                       ///< Recursion depth held by owner_
};

// ---------------------------------------------------------------------------
// castle::recursive_mutex is basic_recursive_mutex<> using castle::spin_wait,
// castle's zero-dependency, bare-metal-safe default WaitPolicy. See
// castle/sync/wait_policy.hpp to plug in a real RTOS scheduler's
// yield/block primitive instead of spinning.
// ---------------------------------------------------------------------------
using recursive_mutex = basic_recursive_mutex<>;

} // namespace castle

#endif // CASTLE_SYNC_RECURSIVE_MUTEX_HPP