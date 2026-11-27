// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file scoped_semaphore.hpp
 * @brief Defines an RAII guard that holds one semaphore permit for the current scope.
 *
 * Use this header when one permit from a `semaphore` should be acquired on
 * construction and returned automatically on every scope exit path. The guard
 * performs no heap allocation, throws no exceptions, uses no RTTI or virtual
 * functions, and has no move or copy semantics. Construction has no error code
 * or timeout; it directly calls `semaphore::acquire()` and therefore waits until
 * a permit is available.
 *
 * The guard always manages exactly one permit. It must not outlive the
 * referenced semaphore, and it should be used only with semaphores whose permit
 * lifetime is naturally scoped to the current block.
 *
 * @code
 * #include "castle/sync/scoped_semaphore.hpp"
 *
 * int main()
 * {
 *     castle::semaphore<2> pool(2U);
 *
 *     {
 *         castle::scoped_semaphore<2> guard(pool);
 *         (void)guard;
 *     }
 * }
 * @endcode
 */
#ifndef CASTLE_SYNC_SCOPED_SEMAPHORE_HPP
#define CASTLE_SYNC_SCOPED_SEMAPHORE_HPP

#include "castle/core/compiler.hpp"
#include "castle/sync/semaphore.hpp"

#include <stdint.h>

namespace castle
{

/**
 * @brief RAII guard that acquires one semaphore permit in its constructor and releases it in its destructor.
 *
 * @tparam MaxCount Compile-time capacity of the guarded semaphore.
 * @tparam WaitPolicy Wait policy used by the guarded semaphore.
 * @note The guard is deterministic, allocation-free, and always manages a
 * single permit.
 * @warning The guard must not outlive the referenced semaphore object.
 */
template <uint32_t MaxCount, typename WaitPolicy = spin_wait>
class scoped_semaphore
{
public:
    /**
     * @brief Acquires one permit from the referenced semaphore.
     * @param sem Semaphore to acquire immediately.
     * @note This constructor directly calls `sem.acquire()` and therefore has no
     * separate failure signal.
     * @warning Construction waits indefinitely while no permit is available.
     */
    explicit scoped_semaphore(semaphore<MaxCount, WaitPolicy>& sem) CASTLE_NOEXCEPT
        : semaphore_(sem)
    {
        semaphore_.acquire();
    }

    /**
     * @brief Returns the held permit to the referenced semaphore.
     * @note Destruction performs exactly one `release()` call.
     * @warning Destroy the guard only while the referenced semaphore object is
     * still valid.
     */
    ~scoped_semaphore() CASTLE_NOEXCEPT
    {
        (void)semaphore_.release();
    }

    /**
     * @brief Copy construction is disabled.
     * @note Scoped permit guards are intentionally non-copyable.
     * @warning Attempting to copy a scoped semaphore guard is a compile-time error.
     */
    scoped_semaphore(CASTLE_CONST scoped_semaphore&) CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @note Scoped permit guards are intentionally non-copyable.
     * @warning Attempting to copy-assign a scoped semaphore guard is a compile-time error.
     */
    scoped_semaphore& operator=(CASTLE_CONST scoped_semaphore&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @note Scoped permit guards never transfer permit responsibility implicitly.
     * @warning Attempting to move a scoped semaphore guard is a compile-time error.
     */
    scoped_semaphore(scoped_semaphore&&) CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @note Scoped permit guards never transfer permit responsibility implicitly.
     * @warning Attempting to move-assign a scoped semaphore guard is a compile-time error.
     */
    scoped_semaphore& operator=(scoped_semaphore&&) CASTLE_DELETE;

private:
    semaphore<MaxCount, WaitPolicy>& semaphore_;
};

}

#endif
