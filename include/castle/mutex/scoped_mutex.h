#ifndef CASTLE_MUTEX_SCOPED_MUTEX_H
#define CASTLE_MUTEX_SCOPED_MUTEX_H

#include "castle/mutex/mutex.h"

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
class scoped_mutex
{
public:
    explicit scoped_mutex(mutex& mutex) CASTLE_NOEXCEPT
        : mutex_(mutex)
    {
        mutex_.lock();
    }

    ~scoped_mutex() CASTLE_NOEXCEPT
    {
        mutex_.unlock();
    }

    scoped_mutex(CASTLE_CONST scoped_mutex&) CASTLE_DELETE;
    scoped_mutex& operator=(CASTLE_CONST scoped_mutex&) CASTLE_DELETE;

    scoped_mutex(scoped_mutex&&) CASTLE_DELETE;
    scoped_mutex& operator=(scoped_mutex&&) CASTLE_DELETE;

private:
    mutex& mutex_;
};

}  // namespace castle

#endif  // CASTLE_MUTEX_SCOPED_MUTEX_H
