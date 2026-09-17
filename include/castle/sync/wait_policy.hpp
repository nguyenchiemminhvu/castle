#ifndef CASTLE_SYNC_WAIT_POLICY_HPP
#define CASTLE_SYNC_WAIT_POLICY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{

// ---------------------------------------------------------------------------
// Compile-time customization point for what castle's spinlock-based
// synchronization primitives (castle::semaphore, mutex, shared_mutex, ...) do
// on each failed retry of a blocking wait loop.
//
// Why not a real, Zephyr-k_sem-style "block the task and wake it from a wait
// queue" implementation? That requires an actual kernel: a scheduler that
// owns task state, stacks, and a wait queue, so it can take a task off the
// CPU and put another one on. A header-only, no-heap, no-virtual-function,
// freestanding template library has no such kernel to own, so it cannot
// implement thread scheduling by itself.
//
// What castle *can* do is give an integrator a single, type-checked
// extension point to swap the default CPU-relax spin for their own runtime's
// yield/block primitive, without touching any call site in application code.
// A WaitPolicy is any type providing:
//
//     static void wait() noexcept;
//
// invoked once per failed retry iteration of a blocking wait loop (e.g.
// castle::semaphore::acquire()). This is where a real RTOS integration plugs
// in its actual scheduler:
//
// @code
// struct freertos_wait_policy
// {
//     static void wait() noexcept { taskYIELD(); }
// };
//
// castle::semaphore<4, freertos_wait_policy> pool(4U);
// @endcode
//
// or, for Zephyr:
//
// @code
// struct zephyr_wait_policy
// {
//     static void wait() noexcept { k_yield(); }
// };
// @endcode
// ---------------------------------------------------------------------------
struct spin_wait
{
    // ---------------------------------------------------------------------------
    // Hints to the CPU that the caller is in a busy-wait spin loop.
    //
    // Never blocks, yields to an OS, or changes program state; it only lowers
    // the power/bus-contention cost of spinning where the target architecture
    // exposes such an instruction, falling back to a no-op compiler barrier
    // everywhere else. This is castle's zero-dependency, bare-metal-safe
    // default WaitPolicy.
    // ---------------------------------------------------------------------------
    static void wait() CASTLE_NOEXCEPT
    {
        CASTLE_CPU_RELAX();
    }
};

namespace detail
{

// ---------------------------------------------------------------------------
// SFINAE probe: does `T` provide a callable `static void T::wait() noexcept`?
//
// Shared by every castle synchronization primitive's WaitPolicy
// static_assert, giving a precise compile error instead of a wall of
// template-instantiation noise when a caller supplies an incompatible
// policy type.
// ---------------------------------------------------------------------------
template <typename T, typename = void>
struct has_static_wait : meta::false_type {};

template <typename T>
struct has_static_wait<T, meta::void_t<decltype(T::wait())>>
    : meta::bool_constant<CASTLE_NOEXCEPT(T::wait())> {};

} // namespace detail

} // namespace castle

#endif // CASTLE_SYNC_WAIT_POLICY_HPP
