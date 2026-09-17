// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file wait_policy.hpp
 * @brief Defines Castle's compile-time wait-policy contract for blocking synchronization retries.
 *
 * Use this header when selecting what Castle synchronization primitives should do
 * on each failed retry of a blocking wait loop. The default policy issues only an
 * architecture-specific spin hint, but any type exposing `static void wait() noexcept`
 * can be supplied to mutexes, semaphores, and shared mutexes. The contract is
 * allocation-free, exception-free, deterministic, and resolved entirely at
 * compile time with no RTTI, virtual functions, or STL threading support.
 *
 * `WaitPolicy` does not surface runtime failures. Compatibility is checked with a
 * compile-time `static_assert` in each consuming primitive, and `wait()` itself
 * returns no status.
 *
 * @code
 * #include "castle/sync/mutex.hpp"
 *
 * struct scheduler_wait_policy
 * {
 *     static void wait() noexcept
 *     {
 *         // Call the platform scheduler's yield primitive here.
 *     }
 * };
 *
 * castle::basic_mutex<scheduler_wait_policy> lock;
 * @endcode
 */
#ifndef CASTLE_SYNC_WAIT_POLICY_HPP
#define CASTLE_SYNC_WAIT_POLICY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{

/**
 * @brief Default wait policy that performs a CPU spin hint.
 *
 * @note This policy never blocks on an operating system primitive and never
 * changes guarded object state.
 * @warning Because it only spins, blocked callers continue consuming execution
 * time until the guarded condition becomes true.
 */
struct spin_wait
{
    /**
     * @brief Issues an architecture-specific spin hint for one failed retry.
     * @note The hint is provided by `CASTLE_CPU_RELAX()` and is `noexcept`.
     * @warning This function does not guarantee forward progress on its own; it
     * only reduces the cost of busy-waiting.
     */
    static void wait() CASTLE_NOEXCEPT
    {
        CASTLE_CPU_RELAX();
    }
};

namespace detail
{

template <typename T, typename = void>
struct has_static_wait : meta::false_type {};

/**
 * @brief Checks if a type provides a `static void wait() noexcept` member function.
 * @tparam T The type to check for the presence of a `static void wait() noexcept` member function.
 * @value `true` if `T` provides a `static void wait() noexcept` member function, `false` otherwise.
 */
template <typename T>
struct has_static_wait<T, meta::void_t<decltype(T::wait())>>
    : meta::bool_constant<CASTLE_NOEXCEPT(T::wait())> {};

}

}

#endif
