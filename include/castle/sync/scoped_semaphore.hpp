#ifndef CASTLE_SYNC_SCOPED_SEMAPHORE_HPP
#define CASTLE_SYNC_SCOPED_SEMAPHORE_HPP

#include "castle/core/compiler.hpp"
#include "castle/sync/semaphore.hpp"

#include <stdint.h>

namespace castle
{

// ---------------------------------------------------------------------------
// RAII semaphore permit guard.
//
// Acquires one permit from the given semaphore during construction
// (blocking, like semaphore::acquire()) and returns it during destruction.
//
// WaitPolicy must match the guarded semaphore's WaitPolicy; it defaults to
// castle::spin_wait, same as castle::semaphore.
//
// Usage:
//
// @code
// castle::semaphore<4> pool(4U);
//
// {
//     castle::scoped_semaphore<4> guard(pool);
//
//     // one of up to 4 permits is held here
// }
// // permit automatically returned here
// @endcode
// ---------------------------------------------------------------------------
template <uint32_t MaxCount, typename WaitPolicy = spin_wait>
class scoped_semaphore
{
public:
    explicit scoped_semaphore(semaphore<MaxCount, WaitPolicy>& sem) CASTLE_NOEXCEPT
        : semaphore_(sem)
    {
        semaphore_.acquire();
    }

    ~scoped_semaphore() CASTLE_NOEXCEPT
    {
        // acquire() always returned a single, previously-available permit,
        // so returning exactly one permit here can never exceed MaxCount.
        (void)semaphore_.release();
    }

    scoped_semaphore(CASTLE_CONST scoped_semaphore&) CASTLE_DELETE;
    scoped_semaphore& operator=(CASTLE_CONST scoped_semaphore&) CASTLE_DELETE;

    scoped_semaphore(scoped_semaphore&&) CASTLE_DELETE;
    scoped_semaphore& operator=(scoped_semaphore&&) CASTLE_DELETE;

private:
    semaphore<MaxCount, WaitPolicy>& semaphore_;
};

} // namespace castle

#endif // CASTLE_SYNC_SCOPED_SEMAPHORE_HPP
