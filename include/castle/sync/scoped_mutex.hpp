#ifndef CASTLE_SYNC_SCOPED_MUTEX_HPP
#define CASTLE_SYNC_SCOPED_MUTEX_HPP

#include "castle/sync/mutex.hpp"

namespace castle
{

// ----------------------------------------------------------------------------
// RAII mutex guard.
//
// Automatically locks the given mutex during construction and
// unlocks it during destruction.
//
// This class is intentionally:
// - non-copyable
// - non-movable
// - allocation-free
// - exception-free
//
// WaitPolicy must match the guarded mutex's WaitPolicy; it defaults to
// castle::spin_wait, same as castle::mutex.
//
// Usage:
//
// @code
// castle::mutex lock;
//
// {
//     castle::scoped_mutex guard(lock);
//
//     // protected section
// }
// // mutex automatically released here
// @endcode
// ----------------------------------------------------------------------------
template <typename WaitPolicy = spin_wait>
class basic_scoped_mutex
{
public:
    explicit basic_scoped_mutex(basic_mutex<WaitPolicy>& mutex) CASTLE_NOEXCEPT
        : mutex_(mutex)
    {
        mutex_.lock();
    }

    ~basic_scoped_mutex() CASTLE_NOEXCEPT
    {
        mutex_.unlock();
    }

    basic_scoped_mutex(CASTLE_CONST basic_scoped_mutex&) CASTLE_DELETE;
    basic_scoped_mutex& operator=(CASTLE_CONST basic_scoped_mutex&) CASTLE_DELETE;

    basic_scoped_mutex(basic_scoped_mutex&&) CASTLE_DELETE;
    basic_scoped_mutex& operator=(basic_scoped_mutex&&) CASTLE_DELETE;

private:
    basic_mutex<WaitPolicy>& mutex_;
};

// ---------------------------------------------------------------------------
// castle::scoped_mutex is basic_scoped_mutex<> for castle::mutex
// (basic_mutex<castle::spin_wait>). See castle/sync/wait_policy.hpp to plug
// in a custom WaitPolicy for both the mutex and its guard.
// ---------------------------------------------------------------------------
using scoped_mutex = basic_scoped_mutex<>;

}  // namespace castle

#endif  // CASTLE_SYNC_SCOPED_MUTEX_HPP
