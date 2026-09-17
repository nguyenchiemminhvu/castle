// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file semaphore.hpp
 * @brief Defines a bounded counting semaphore with compile-time capacity and wait policy.
 *
 * Use this header when access must be limited to a fixed number of interchangeable
 * resources, or when an ownership-free signal is needed between contexts. The
 * semaphore owns all state internally, performs no heap allocation, throws no
 * exceptions, uses no RTTI or virtual functions, and depends on no STL
 * synchronization facilities. `acquire()` has no timeout or error code and waits
 * until a permit becomes available, while `try_acquire()` and `release()` surface
 * success through boolean return values.
 *
 * Semaphores do not track an owner. Any context may call `release()`, including a
 * context that never called `acquire()`, as long as the total permit count is not
 * driven above `MaxCount`.
 *
 * @code
 * #include "castle/sync/semaphore.hpp"
 *
 * int main()
 * {
 *     castle::semaphore<2> slots(2U);
 *
 *     slots.acquire();
 *     CASTLE_UNUSED bool returned = slots.release();
 *     (void)returned;
 * }
 * @endcode
 */
#ifndef CASTLE_SYNC_SEMAPHORE_HPP
#define CASTLE_SYNC_SEMAPHORE_HPP

#include "castle/core/compiler.hpp"
#include "castle/sync/wait_policy.hpp"

#include <stdint.h>

namespace castle
{

/**
 * @brief Bounded counting semaphore with ownership-free permit release.
 *
 * @tparam MaxCount Maximum number of permits the semaphore may hold.
 * @tparam WaitPolicy Type providing `static void wait() noexcept`, invoked once
 * per failed blocking retry.
 * @note Concurrent `acquire()`, `try_acquire()`, `release()`, and `count()`
 * calls are safe when callers follow the semaphore protocol.
 * @warning `acquire()` waits indefinitely while no permit is available, and
 * callers must check the return value of `release()` to detect overflow.
 */
template <uint32_t MaxCount, typename WaitPolicy = spin_wait>
class semaphore
{
    static_assert(MaxCount > 0U, "MaxCount must be greater than zero.");
    static_assert(detail::has_static_wait<WaitPolicy>::value,
                  "WaitPolicy must provide a public, callable `static void wait() noexcept`.");

public:
    /** @brief Unsigned type used for permit counts. */
    using value_type = uint32_t;

    /** @brief Wait policy used by this semaphore specialization. */
    using wait_policy_type = WaitPolicy;

    /**
     * @brief Constructs a semaphore with the requested initial permit count.
     * @param initial_count Starting number of available permits.
     * @note Values larger than `MaxCount` are clamped to `MaxCount`.
     * @warning The object must outlive every context that may access it.
     */
    explicit semaphore(value_type initial_count) CASTLE_NOEXCEPT
        : count_((initial_count > MaxCount) ? MaxCount : initial_count)
    {
    }

    /**
     * @brief Destroys the semaphore object.
     * @note Destruction is trivial and reports no errors.
     * @warning Callers must ensure no context still relies on this semaphore
     * when it is destroyed.
     */
    ~semaphore() CASTLE_DEFAULT;

    /**
     * @brief Copy construction is disabled.
     * @note Semaphore state is intentionally non-copyable.
     * @warning Attempting to copy a semaphore is a compile-time error.
     */
    semaphore(CASTLE_CONST semaphore&) CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @note Semaphore state is intentionally non-copyable.
     * @warning Attempting to copy-assign a semaphore is a compile-time error.
     */
    semaphore& operator=(CASTLE_CONST semaphore&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @note Permit state is never transferred implicitly.
     * @warning Attempting to move a semaphore is a compile-time error.
     */
    semaphore(semaphore&&) CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @note Permit state is never transferred implicitly.
     * @warning Attempting to move-assign a semaphore is a compile-time error.
     */
    semaphore& operator=(semaphore&&) CASTLE_DELETE;

    /**
     * @brief Acquires one permit, waiting until one is available.
     * @note `WaitPolicy::wait()` runs once per failed retry. This function has no
     * timeout, error code, or failure callback.
     * @warning If no other context ever returns a permit, this function waits indefinitely.
     */
    void acquire() CASTLE_NOEXCEPT
    {
        while (true)
        {
            value_type current = __sync_add_and_fetch(&count_, 0U);

            if (current == 0U)
            {
                WaitPolicy::wait();
                continue;
            }

            if (__sync_bool_compare_and_swap(&count_, current, current - 1U)) // LCOV_EXCL_BR_LINE
            {
                return;
            }

            WaitPolicy::wait(); // LCOV_EXCL_LINE
        }
    }

    /**
     * @brief Attempts to acquire one permit without waiting.
     * @return `true` if a permit was acquired; otherwise `false`.
     * @note This is the only acquisition path that reports immediate contention.
     * @warning A `true` result consumes one permit and must eventually be
     * balanced by application logic returning a permit if appropriate.
     */
    CASTLE_NODISCARD bool try_acquire() CASTLE_NOEXCEPT
    {
        value_type current = __sync_add_and_fetch(&count_, 0U);

        if (current == 0U)
        {
            return false;
        }

        return __sync_bool_compare_and_swap(&count_, current, current - 1U);
    }

    /**
     * @brief Returns one or more permits to the semaphore.
     * @param update Number of permits to return. A value of `0` leaves the count
     * unchanged and returns `true`.
     * @return `true` if the permit count was updated; otherwise `false` when the
     * requested update would exceed `MaxCount`.
     * @note On failure the permit count is left unchanged.
     * @warning Callers should not ignore the return value, because overflow is
     * reported only through this boolean result.
     */
    CASTLE_NODISCARD bool release(value_type update = 1U) CASTLE_NOEXCEPT
    {
        while (true) // LCOV_EXCL_LINE
        {
            value_type current = __sync_add_and_fetch(&count_, 0U);

            if ((MaxCount - current) < update)
            {
                return false;
            }

            if (__sync_bool_compare_and_swap(&count_, current, current + update)) // LCOV_EXCL_BR_LINE
            {
                return true;
            }

            WaitPolicy::wait(); // LCOV_EXCL_LINE
        }
    }

    /**
     * @brief Returns a snapshot of the currently available permits.
     * @return Current permit count observed at the time of the call.
     * @note The returned value may become stale immediately after it is read.
     * @warning Do not treat this snapshot as a reservation or as proof that a
     * later `acquire()` will succeed.
     */
    CASTLE_NODISCARD value_type count() CASTLE_NOEXCEPT
    {
        return __sync_add_and_fetch(&count_, 0U);
    }

    /**
     * @brief Returns the compile-time semaphore capacity.
     * @return `MaxCount`.
     * @note This function is `constexpr` and does not inspect runtime state.
     * @warning Capacity does not imply current availability; use `count()` to
     * inspect the current permit snapshot instead.
     */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR value_type max() CASTLE_NOEXCEPT
    {
        return MaxCount;
    }

private:
    CASTLE_VOLATILE value_type count_;
};

/**
 * @brief Convenience alias for `semaphore<1U>`.
 * @note This provides a single-permit signal that still has no ownership requirement.
 * @warning `release()` can fail if a permit is returned while the semaphore
 * already holds its single permit.
 */
using binary_semaphore = semaphore<1U>;

}

#endif