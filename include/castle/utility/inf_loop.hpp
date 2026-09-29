// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file inf_loop.hpp
 * @brief Defines a policy-driven, non-returning execution loop.
 *
 * The loop delegates each wait/yield operation to a compile-time policy. This
 * keeps scheduler integration platform-specific while the loop itself remains
 * allocation-free, exception-free, and free of virtual dispatch.
 */
#ifndef CASTLE_UTILITY_INF_LOOP_HPP
#define CASTLE_UTILITY_INF_LOOP_HPP

#include "castle/core/compiler.hpp"
#include "castle/sync/wait_policy.hpp"

namespace castle
{

/**
 * @brief Enters a non-returning loop and invokes a wait policy on every pass.
 *
 * The supplied policy determines how the calling task gives up execution. It
 * may yield to a scheduler, block on an event, suspend the task, or enter a
 * platform-specific low-power wait state. The policy is invoked again whenever
 * it returns.
 *
 * @tparam WaitPolicy Type providing `static void wait() noexcept`.
 *
 * @note No default policy is provided intentionally. In particular,
 * `castle::spin_wait` only emits a CPU spin hint and does not yield or block.
 *
 * @warning This function never returns. Use it only when the current execution
 * path is intentionally terminal (for example, a task that must remain parked
 * or a fatal-error path).
 *
 * @code
 * struct scheduler_wait
 * {
 *     static void wait() noexcept
 *     {
 *         platform_scheduler_wait();
 *     }
 * };
 *
 * castle::infinite_loop<scheduler_wait>();
 * @endcode
 */
template <typename WaitPolicy>
CASTLE_NORETURN CASTLE_INLINE void infinite_loop() CASTLE_NOEXCEPT
{
    static_assert(
        detail::has_static_wait<WaitPolicy>::value,
        "WaitPolicy must provide static void wait() noexcept");

    for (;;)
    {
        WaitPolicy::wait();
    }
}

}

#endif // CASTLE_UTILITY_INF_LOOP_HPP
