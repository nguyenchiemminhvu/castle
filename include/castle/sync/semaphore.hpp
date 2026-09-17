#ifndef CASTLE_SYNC_SEMAPHORE_HPP
#define CASTLE_SYNC_SEMAPHORE_HPP

#include "castle/core/compiler.hpp"
#include "castle/sync/wait_policy.hpp"

#include <stdint.h>

namespace castle
{

// ---------------------------------------------------------------------------
// A bounded counting semaphore.
//
// Gates access to a resource pool of up to MaxCount interchangeable units
// (buffers, worker slots, hardware channels, ...). acquire() takes one unit,
// blocking (busy-wait) while none are available; release() returns one or
// more units, never exceeding the compile-time capacity.
//
// Unlike castle::mutex, a semaphore has no notion of ownership: any context
// may call release(), including an ISR that never called acquire(). This
// makes it usable as an ISR-to-task signal (see castle::binary_semaphore)
// in addition to classic resource-counting use, matching the role of
// Zephyr's k_sem / POSIX sem_t.
//
// The implementation uses GCC/Clang's legacy __sync built-ins directly, like
// castle::mutex and castle::shared_mutex, and requires no heap allocation,
// virtual functions, RTTI, exceptions, or STL dependency.
//
// WaitPolicy is invoked once per failed retry iteration of acquire()'s wait
// loop; it defaults to castle::spin_wait (a portable CPU-relax hint). See
// castle/sync/wait_policy.hpp to plug in a real RTOS scheduler's
// yield/block primitive instead of spinning.
// ---------------------------------------------------------------------------
template <uint32_t MaxCount, typename WaitPolicy = spin_wait>
class semaphore
{
    static_assert(MaxCount > 0U, "MaxCount must be greater than zero.");
    static_assert(detail::has_static_wait<WaitPolicy>::value,
                  "WaitPolicy must provide a public, callable `static void wait() noexcept`.");

public:
    using value_type = uint32_t;
    using wait_policy_type = WaitPolicy;

    // ---------------------------------------------------------------------------
    // Construct a semaphore with an initial permit count.
    //
    // initial_count is clamped to MaxCount rather than invoking undefined
    // behavior, matching castle's fail-safe-by-construction philosophy.
    // ---------------------------------------------------------------------------
    explicit semaphore(value_type initial_count) CASTLE_NOEXCEPT
        : count_((initial_count > MaxCount) ? MaxCount : initial_count)
    {
    }

    ~semaphore() CASTLE_DEFAULT;

    // Non-copyable semantics matching the library safety goals
    semaphore(CASTLE_CONST semaphore&) CASTLE_DELETE;
    semaphore& operator=(CASTLE_CONST semaphore&) CASTLE_DELETE;

    // Non-movable semantics to prevent accidental transfer of permit ownership
    semaphore(semaphore&&) CASTLE_DELETE;
    semaphore& operator=(semaphore&&) CASTLE_DELETE;

    // ---------------------------------------------------------------------------
    // Acquires one permit. Blocks (busy-waits) until a permit is available.
    //
    // WaitPolicy::wait() runs once per failed retry, e.g. to hint the CPU to
    // relax while spinning (the default) or, in an RTOS-integrated build, to
    // yield the calling task to the real scheduler instead of spinning.
    // ---------------------------------------------------------------------------
    void acquire() CASTLE_NOEXCEPT
    {
        while (true) // LCOV_EXCL_BR_LINE
        {
            value_type current = __sync_add_and_fetch(&count_, 0U);

            if (current == 0U)
            {
                WaitPolicy::wait();
                continue; // Spin/Busy-wait loop
            }

            if (__sync_bool_compare_and_swap(&count_, current, current - 1U))
            {
                return;
            }

            WaitPolicy::wait();
        }
    }

    // ---------------------------------------------------------------------------
    // Tries to acquire one permit without blocking.
    // return true if a permit was acquired, false otherwise.
    // ---------------------------------------------------------------------------
    CASTLE_NODISCARD bool try_acquire() CASTLE_NOEXCEPT
    {
        value_type current = __sync_add_and_fetch(&count_, 0U);

        if (current == 0U)
        {
            return false;
        }

        return __sync_bool_compare_and_swap(&count_, current, current - 1U);
    }

    // ---------------------------------------------------------------------------
    // Returns `update` permit(s) to the semaphore.
    //
    // return true if the permits were returned, false if doing so would raise
    // the count above MaxCount. The count is left unchanged in that case
    // rather than silently saturating or invoking undefined behavior.
    // ---------------------------------------------------------------------------
    CASTLE_NODISCARD bool release(value_type update = 1U) CASTLE_NOEXCEPT
    {
        while (true) // LCOV_EXCL_BR_LINE
        {
            value_type current = __sync_add_and_fetch(&count_, 0U);

            if ((MaxCount - current) < update)
            {
                return false;
            }

            if (__sync_bool_compare_and_swap(&count_, current, current + update))
            {
                return true;
            }

            WaitPolicy::wait();
        }
    }

    // ---------------------------------------------------------------------------
    // Returns a snapshot of the number of permits currently available.
    //
    // The value may already be stale by the time it is observed; useful for
    // diagnostics and tests, not for making acquire/release decisions.
    // ---------------------------------------------------------------------------
    CASTLE_NODISCARD value_type count() CASTLE_NOEXCEPT
    {
        return __sync_add_and_fetch(&count_, 0U);
    }

    // ---------------------------------------------------------------------------
    // Returns the compile-time capacity of the semaphore.
    // ---------------------------------------------------------------------------
    CASTLE_NODISCARD static CASTLE_CONSTEXPR value_type max() CASTLE_NOEXCEPT
    {
        return MaxCount;
    }

private:
    CASTLE_VOLATILE value_type count_; ///< Number of permits currently available
};

// ---------------------------------------------------------------------------
// A semaphore with a single permit: an unowned, ISR-safe on/off signal.
//
// Unlike castle::mutex, there is no owning-thread requirement, so a
// binary_semaphore may be release()-d from an interrupt handler that never
// called acquire().
// ---------------------------------------------------------------------------
using binary_semaphore = semaphore<1U>;

} // namespace castle

#endif // CASTLE_SYNC_SEMAPHORE_HPP