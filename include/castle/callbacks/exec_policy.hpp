// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file exec_policy.hpp
 * @brief Defines poll-driven callback execution policies such as once, every_n, on_change, throttle, and periodic.
 *
 * Use this header when a callback should not run on every call site invocation, but instead follow a deterministic
 * control-flow rule. Each policy stores the callback by value, never allocates, and exposes an explicit execute/poll
 * entry point that the caller drives from normal application code.
 *
 * Key constraints:
 * - No heap allocation.
 * - No background threads or timers; time-based policies are poll-driven.
 * - single_thread mode performs no synchronization and assumes external serialization.
 * - concurrent mode uses Castle atomics and, where needed, Castle mutex/scoped_mutex for short state transitions only.
 * - Callbacks execute in the caller's thread.
 * - Reset/rearm and observer methods that read non-atomic state still require external coordination where noted below.
 *
 * @code
 * #include "castle/callbacks/exec_policy.hpp"
 * #include "castle/chrono/literals.hpp"
 *
 * struct fake_clock
 * {
 *     using duration = castle::chrono::milliseconds;
 *     using time_point = castle::chrono::time_point<fake_clock, duration>;
 *
 *     static time_point now() noexcept
 *     {
 *         return time_point(duration(now_ms));
 *     }
 *
 *     static int64_t now_ms;
 * };
 *
 * int64_t fake_clock::now_ms = 0;
 *
 * int main()
 * {
 *     using namespace castle::chrono::literals::chrono_literals;
 *
 *     int hits = 0;
 *     auto once = castle::callbacks::policy::make_once([&hits]() noexcept { ++hits; });
 *     once.execute();
 *
 *     auto gate = castle::callbacks::policy::make_throttle<
 *         castle::callbacks::policy::single_thread,
 *         fake_clock>(10_ms, [&hits]() noexcept { ++hits; });
 *     gate.execute();
 *
 *     return hits == 2 ? 0 : 1;
 * }
 * @endcode
 */
#ifndef CASTLE_CALLBACKS_EXEC_POLICY_HPP
#define CASTLE_CALLBACKS_EXEC_POLICY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/atomic/atomic.hpp"
#include "castle/sync/mutex.hpp"
#include "castle/sync/scoped_mutex.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/optional.hpp"
#include "castle/chrono/chrono.hpp"

#include <stddef.h>
#include <stdint.h>

namespace castle
{
namespace callbacks
{
namespace policy
{

/**
 * @brief Concurrency mode that stores policy state in plain values with no synchronization.
 *
 * @note Use this mode only when all accesses are externally serialized.
 */
struct single_thread
{
    /**
     * @brief Storage type used for policy state.
     * @tparam T Stored value type.
     */
    template <typename T>
    using atomic_type = T;

    /**
     * @brief Reads a stored value.
     * @tparam T Stored value type.
     * @param value Value to read.
     * @return A copy of @p value.
     */
    template <typename T>
    static T load(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return value;
    }

    /**
     * @brief Writes a stored value.
     * @tparam T Stored value type.
     * @param value Destination value.
     * @param desired New value to store.
     */
    template <typename T>
    static void store(T& value, CASTLE_CONST T& desired) CASTLE_NOEXCEPT
    {
        value = desired;
    }

    /**
     * @brief Performs a plain compare-and-exchange on a stored value.
     * @tparam T Stored value type.
     * @param value Destination value.
     * @param expected Expected current value; overwritten with the actual value on failure.
     * @param desired Replacement value written on success.
     * @return True when the exchange succeeds.
     */
    template <typename T>
    static bool compare_exchange(T& value, T& expected, CASTLE_CONST T& desired) CASTLE_NOEXCEPT
    {
        if (value != expected)
        {
            expected = value;
            return false;
        }

        value = desired;
        return true;
    }

    /**
     * @brief Adds to a stored value and returns the previous value.
     * @tparam T Stored value type.
     * @param value Destination value.
     * @param amount Increment amount.
     * @return The value before incrementing.
     */
    template <typename T>
    static T fetch_add(T& value, T amount) CASTLE_NOEXCEPT
    {
        CASTLE_CONST T previous = value;
        value += amount;
        return previous;
    }
};

/**
 * @brief Concurrency mode that stores policy state in castle::atomic and uses atomic operations with fixed memory orders.
 *
 * @note This mode only synchronizes state that is actually stored through atomic_type<T>. Policies that also carry
 * non-atomic members document any remaining coordination requirements explicitly.
 */
struct concurrent
{
    /**
     * @brief Storage type used for policy state.
     * @tparam T Stored value type.
     */
    template <typename T>
    using atomic_type = castle::atomic<T>;

    /**
     * @brief Atomically reads a stored value with acquire semantics.
     * @tparam T Stored value type.
     * @param value Atomic value to read.
     * @return The loaded value.
     */
    template <typename T>
    static T load(CASTLE_CONST castle::atomic<T>& value) CASTLE_NOEXCEPT
    {
        return value.load(castle::memory_order_acquire);
    }

    /**
     * @brief Atomically writes a stored value with release semantics.
     * @tparam T Stored value type.
     * @param value Atomic value to update.
     * @param desired New value to store.
     */
    template <typename T>
    static void store(castle::atomic<T>& value, CASTLE_CONST T& desired) CASTLE_NOEXCEPT
    {
        value.store(desired, castle::memory_order_release);
    }

    /**
     * @brief Atomically compares and exchanges a stored value.
     * @tparam T Stored value type.
     * @param value Atomic value to update.
     * @param expected Expected current value; overwritten with the actual value on failure.
     * @param desired Replacement value written on success.
     * @return True when the exchange succeeds.
     */
    template <typename T>
    static bool compare_exchange(castle::atomic<T>& value, T& expected, CASTLE_CONST T& desired) CASTLE_NOEXCEPT
    {
        return value.compare_exchange_strong(expected, desired,
                                              castle::memory_order_acq_rel,
                                              castle::memory_order_acquire);
    }

    /**
     * @brief Atomically adds to a stored value and returns the previous value.
     * @tparam T Stored value type.
     * @param value Atomic value to update.
     * @param amount Increment amount.
     * @return The value before incrementing.
     */
    template <typename T>
    static T fetch_add(castle::atomic<T>& value, T amount) CASTLE_NOEXCEPT
    {
        return value.fetch_add(amount, castle::memory_order_relaxed);
    }
};

/**
 * @brief Forward declaration for the synchronization helper used by policies that protect non-atomic state.
 * @tparam Mode Concurrency mode.
 */
template <typename Mode>
class synchronization;

/**
 * @brief Synchronization helper for single_thread mode.
 *
 * The guard is empty and performs no locking.
 */
template <>
class synchronization<single_thread>
{
public:
    /**
     * @brief No-op guard used to keep locking code uniform across modes.
     */
    class guard
    {
    public:
        /**
         * @brief Constructs a no-op guard.
         * @param owner Owning synchronization object.
         */
        explicit guard(synchronization&) CASTLE_NOEXCEPT {}
    };

    /**
     * @brief Returns a no-op guard.
     * @return Empty guard object.
     */
    guard lock() CASTLE_NOEXCEPT
    {
        return guard(*this);
    }
};

/**
 * @brief Synchronization helper for concurrent mode.
 *
 * The guard locks an internal Castle mutex for the duration of its lifetime.
 */
template <>
class synchronization<concurrent>
{
public:
    /**
     * @brief RAII guard that holds the internal mutex while alive.
     */
    class guard
    {
    public:
        /**
         * @brief Locks the owning synchronization object.
         * @param owner Synchronization object whose mutex should be held.
         */
        explicit guard(synchronization& owner) CASTLE_NOEXCEPT
            : scoped_(owner.mutex_)
        {
        }

        /**
         * @brief Copy construction is disabled.
         */
        guard(CASTLE_CONST guard&) CASTLE_DELETE;

        /**
         * @brief Copy assignment is disabled.
         */
        guard& operator=(CASTLE_CONST guard&) CASTLE_DELETE;

    private:
        castle::scoped_mutex scoped_;
    };

    /**
     * @brief Locks the internal mutex and returns the guard.
     * @return Guard that releases the lock on destruction.
     */
    guard lock() CASTLE_NOEXCEPT
    {
        return guard(*this);
    }

private:
    castle::mutex mutex_;
};

/**
 * @brief Executes a callback at most once until reset() is called.
 * @tparam Callback Stored callback type.
 * @tparam Mode Concurrency mode used for the fired latch.
 *
 * The latch is set before the callback runs, so recursive calls from inside the callback do nothing.
 *
 * @warning reset() is only a state write. In concurrent use, callers must coordinate reset() with any in-flight
 * execute() if they require stronger guarantees than "future calls may fire again".
 */
template <typename Callback, typename Mode = single_thread>
class once
{
public:
    using callback_type    = Callback;
    using concurrency_type = Mode;

    /**
     * @brief Constructs the policy from a callback object.
     * @tparam C Source callable type.
     * @param cb Callback stored by value.
     */
    template <typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, once>::value>>
    explicit once(C&& cb) CASTLE_NOEXCEPT(meta::is_nothrow_constructible<Callback, C&&>::value)
        : cb_(CASTLE_FORWARD<C>(cb))
    {
    }

    /**
     * @brief Executes the callback only on the first successful call since construction or reset().
     * @param args Arguments forwarded to the callback.
     */
    template <typename... Args>
    void execute(Args&&... args)
    {
        bool expected = false;

        if (!Mode::compare_exchange(fired_, expected, true))
        {
            return;
        }

        cb_(CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Alias for execute().
     * @param args Arguments forwarded to the callback.
     */
    template <typename... Args>
    void operator()(Args&&... args)
    {
        execute(CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Clears the fired latch so the next execute() may run the callback again.
     */
    void reset() CASTLE_NOEXCEPT
    {
        Mode::store(fired_, false);
    }

    /**
     * @brief Reports whether the callback has already fired since the last reset.
     * @return True when the once latch is set.
     */
    bool has_fired() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Mode::load(fired_);
    }

private:
    Callback cb_;
    typename Mode::template atomic_type<bool> fired_{false};
};

/**
 * @brief Deduction guide for once.
 * @tparam C Source callable type.
 */
template <typename C>
once(C&&) -> once<meta::decay_t<C>>;

/**
 * @brief Creates a once policy while allowing explicit selection of the concurrency mode.
 * @tparam Mode Concurrency mode.
 * @tparam C Source callable type.
 * @param cb Callback stored by value.
 * @return once<meta::decay_t<C>, Mode>.
 */
template <typename Mode = single_thread, typename C>
once<meta::decay_t<C>, Mode> make_once(C&& cb)
{
    return once<meta::decay_t<C>, Mode>(CASTLE_FORWARD<C>(cb));
}

/**
 * @brief Executes a callback at most once, and only before a deadline measured from construction, reset(), or rearm().
 * @tparam Callback Stored callback type.
 * @tparam Mode Concurrency mode used for the state latch.
 * @tparam Clock Clock type supplying `now()`.
 *
 * The policy begins in the armed state. The first execute() before the deadline transitions to fired and runs the
 * callback. Once the deadline passes, execute() transitions to expired and never fires until reset() or rearm().
 *
 * @warning In concurrent mode, the state latch is synchronized but duration_ and deadline_ are plain members. Calls to
 * reset(), rearm(), deadline(), or window() must not race with execute() unless the caller provides external
 * coordination.
 */
template <typename Callback, typename Mode = single_thread, typename Clock = castle::chrono::steady_clock>
class armed_window
{
public:
    using callback_type    = Callback;
    using concurrency_type = Mode;
    using clock            = Clock;
    using duration         = typename Clock::duration;
    using time_point       = typename Clock::time_point;

    /**
     * @brief Current one-shot state.
     */
    enum class state : uint8_t
    {
        armed   = 0,
        fired   = 1,
        expired = 2
    };

    /**
     * @brief Constructs an armed one-shot window starting now.
     * @tparam Rep Source duration representation type.
     * @tparam Period Source duration period.
     * @tparam C Source callable type.
     * @param window Window duration measured from construction time.
     * @param cb Callback stored by value.
     */
    template <typename Rep, typename Period, typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, armed_window>::value>>
    armed_window(castle::chrono::duration<Rep, Period> window, C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
        , duration_(castle::chrono::duration_cast<duration>(window))
        , deadline_(Clock::now() + duration_)
        , state_(state::armed)
    {
    }

    /**
     * @brief Executes the callback once if the window is still armed and unexpired.
     * @param args Arguments forwarded to the callback.
     * @note After the deadline, the first execute() that observes expiry latches the state to expired.
     */
    template <typename... Args>
    void execute(Args&&... args)
    {
        state expected = Mode::load(state_);

        if (expected != state::armed)
        {
            return;
        }

        if (Clock::now() >= deadline_)
        {
            Mode::compare_exchange(state_, expected, state::expired);
            return;
        }

        // LCOV_EXCL_START
        if (!Mode::compare_exchange(state_, expected, state::fired))
        {
            return;
        }
        // LCOV_EXCL_STOP

        cb_(CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Alias for execute().
     * @param args Arguments forwarded to the callback.
     */
    template <typename... Args>
    void operator()(Args&&... args)
    {
        execute(CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Rearms the window using the existing duration, starting from now.
     */
    void reset() CASTLE_NOEXCEPT
    {
        deadline_ = Clock::now() + duration_;
        Mode::store(state_, state::armed);
    }

    /**
     * @brief Rearms the window using a new duration, starting from now.
     * @tparam Rep Source duration representation type.
     * @tparam Period Source duration period.
     * @param window New duration for the next armed window.
     */
    template <typename Rep, typename Period>
    void rearm(castle::chrono::duration<Rep, Period> window) CASTLE_NOEXCEPT
    {
        duration_ = castle::chrono::duration_cast<duration>(window);
        deadline_ = Clock::now() + duration_;
        Mode::store(state_, state::armed);
    }

    /**
     * @brief Forces the window to expire without invoking the callback.
     */
    void expire() CASTLE_NOEXCEPT
    {
        state expected = state::armed;
        Mode::compare_exchange(state_, expected, state::expired);
    }

    /**
     * @brief Reports whether the policy is still armed.
     * @return True when the state latch is armed.
     */
    bool armed()   CASTLE_CONST CASTLE_NOEXCEPT { return Mode::load(state_) == state::armed; }

    /**
     * @brief Reports whether the callback has already fired.
     * @return True when the state latch is fired.
     */
    bool fired()   CASTLE_CONST CASTLE_NOEXCEPT { return Mode::load(state_) == state::fired; }

    /**
     * @brief Reports whether the window has expired without allowing another fire.
     * @return True when the state latch is expired.
     */
    bool expired() CASTLE_CONST CASTLE_NOEXCEPT { return Mode::load(state_) == state::expired; }

    /**
     * @brief Returns the configured window duration.
     * @return Current duration used when arming.
     * @warning This reads a plain member and must not race with rearm() in concurrent use.
     */
    duration   window()   CASTLE_CONST CASTLE_NOEXCEPT { return duration_; }

    /**
     * @brief Returns the currently scheduled deadline.
     * @return Current deadline time point.
     * @warning This reads a plain member and must not race with reset(), rearm(), or execute() coordination that
     * expects a consistent deadline.
     */
    time_point deadline() CASTLE_CONST CASTLE_NOEXCEPT { return deadline_; }

private:
    Callback   cb_;
    duration   duration_;
    time_point deadline_;
    typename Mode::template atomic_type<state> state_;
};

/**
 * @brief Deduction guide for armed_window.
 * @tparam Rep Source duration representation type.
 * @tparam Period Source duration period.
 * @tparam C Source callable type.
 */
template <typename Rep, typename Period, typename C>
armed_window(castle::chrono::duration<Rep, Period>, C&&) -> armed_window<meta::decay_t<C>>;

/**
 * @brief Creates an armed_window policy while allowing explicit selection of Mode and Clock.
 * @tparam Mode Concurrency mode.
 * @tparam Clock Clock type.
 * @tparam Rep Source duration representation type.
 * @tparam Period Source duration period.
 * @tparam C Source callable type.
 * @param window Window duration.
 * @param cb Callback stored by value.
 * @return armed_window<meta::decay_t<C>, Mode, Clock>.
 */
template <typename Mode = single_thread, typename Clock = castle::chrono::steady_clock,
          typename Rep, typename Period, typename C>
armed_window<meta::decay_t<C>, Mode, Clock>
make_armed_window(castle::chrono::duration<Rep, Period> window, C&& cb)
{
    return armed_window<meta::decay_t<C>, Mode, Clock>(window, CASTLE_FORWARD<C>(cb));
}

/**
 * @brief Executes a callback on every Nth call.
 * @tparam Callback Stored callback type.
 * @tparam Mode Concurrency mode used for the call counter.
 *
 * Passing `n == 0` is normalized to 1, so the callback fires on every call.
 */
template <typename Callback, typename Mode = single_thread>
class every_n
{
public:
    using callback_type    = Callback;
    using concurrency_type = Mode;

    /**
     * @brief Constructs the policy with a runtime interval.
     * @tparam C Source callable type.
     * @param n Fire interval. Zero is treated as one.
     * @param cb Callback stored by value.
     */
    template <typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, every_n>::value>>
    every_n(size_type n, C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
        , n_(n == 0U ? 1U : n)
    {
    }

    /**
     * @brief Increments the counter and fires on every Nth call.
     * @param args Arguments forwarded to the callback when it fires.
     */
    template <typename... Args>
    void execute(Args&&... args)
    {
        CASTLE_CONST size_type previous = Mode::fetch_add(counter_, static_cast<size_type>(1U));

        if (((previous + static_cast<size_type>(1U)) % n_) != 0U)
        {
            return;
        }

        Mode::store(counter_, static_cast<size_type>(0U));
        cb_(CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Alias for execute().
     * @param args Arguments forwarded to the callback when it fires.
     */
    template <typename... Args>
    void operator()(Args&&... args)
    {
        execute(CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Resets the call counter to zero.
     */
    void reset() CASTLE_NOEXCEPT
    {
        Mode::store(counter_, static_cast<size_type>(0U));
    }

    /**
     * @brief Returns the normalized interval.
     * @return Interval N used to determine when to fire.
     */
    size_type interval() CASTLE_CONST CASTLE_NOEXCEPT { return n_; }

private:
    Callback  cb_;
    size_type n_;
    typename Mode::template atomic_type<size_type> counter_{0U};
};

/**
 * @brief Deduction guide for every_n.
 * @tparam C Source callable type.
 */
template <typename C>
every_n(size_type, C&&) -> every_n<meta::decay_t<C>>;

/**
 * @brief Creates an every_n policy while allowing explicit selection of the concurrency mode.
 * @tparam Mode Concurrency mode.
 * @tparam C Source callable type.
 * @param n Fire interval. Zero is treated as one.
 * @param cb Callback stored by value.
 * @return every_n<meta::decay_t<C>, Mode>.
 */
template <typename Mode = single_thread, typename C>
every_n<meta::decay_t<C>, Mode> make_every_n(size_type n, C&& cb)
{
    return every_n<meta::decay_t<C>, Mode>(n, CASTLE_FORWARD<C>(cb));
}

/**
 * @brief Creates an every_n policy with a compile-time interval.
 * @tparam N Fire interval, which must be greater than zero.
 * @tparam Mode Concurrency mode.
 * @tparam C Source callable type.
 * @param cb Callback stored by value.
 * @return every_n<meta::decay_t<C>, Mode>.
 */
template <size_type N, typename Mode = single_thread, typename C>
every_n<meta::decay_t<C>, Mode> make_every_n_ct(C&& cb)
{
    static_assert(N > 0U, "make_every_n_ct requires N > 0");
    return every_n<meta::decay_t<C>, Mode>(N, CASTLE_FORWARD<C>(cb));
}

/**
 * @brief Executes a callback only when the observed value differs from the previously stored value.
 * @tparam T Observed value type.
 * @tparam Callback Stored callback type.
 * @tparam Mode Concurrency mode.
 *
 * The first execute() fires unless an initial value was provided at construction.
 *
 * @warning In concurrent mode, the state transition is protected, but the callback reads `last_` after the guard is
 * released. Callers must externally prevent concurrent reset()/execute() that would mutate the policy while the
 * callback is consuming the new value.
 */
template <typename T, typename Callback, typename Mode = single_thread>
class on_change : private synchronization<Mode>
{
    static_assert(!meta::is_reference<T>::value,
                  "on_change requires a value type for T");

    using synchronization_type = synchronization<Mode>;

public:
    using value_type       = T;
    using callback_type    = Callback;
    using concurrency_type = Mode;

    /**
     * @brief Constructs the policy without an initial value.
     * @tparam C Source callable type.
     * @param cb Callback stored by value.
     * @note The first execute() always fires because no previous value exists.
     */
    template <typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, on_change>::value>>
    explicit on_change(C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
    {
    }

    /**
     * @brief Constructs the policy with an initial value.
     * @tparam V Source value type.
     * @tparam C Source callable type.
     * @param initial Initial stored value.
     * @param cb Callback stored by value.
     * @note The callback fires only after a later value differs from @p initial.
     */
    template <typename V, typename C>
    on_change(V&& initial, C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
        , last_(meta::in_place, CASTLE_FORWARD<V>(initial))
    {
    }

    /**
     * @brief Stores a new value and fires only if it differs from the previous one.
     * @tparam U Source value type.
     * @param new_value New observed value.
     */
    template <typename U>
    void execute(U&& new_value)
    {
        bool changed = false;

        {
            CASTLE_UNUSED typename synchronization_type::guard guard = this->lock();

            if (!last_.has_value() || (*last_ != new_value))
            {
                last_.emplace(CASTLE_FORWARD<U>(new_value));
                changed = true;
            }
        }

        if (changed)
        {
            cb_(*last_);
        }
    }

    /**
     * @brief Alias for execute().
     * @tparam U Source value type.
     * @param new_value New observed value.
     */
    template <typename U>
    void operator()(U&& new_value)
    {
        execute(CASTLE_FORWARD<U>(new_value));
    }

    /**
     * @brief Clears the remembered value.
     *
     * @note After reset(), the next execute() always fires.
     */
    void reset() CASTLE_NOEXCEPT(meta::is_nothrow_destructible<T>::value)
    {
        CASTLE_UNUSED typename synchronization_type::guard guard = this->lock();
        last_.reset();
    }

    /**
     * @brief Reports whether a remembered value is currently stored.
     * @return True when the policy currently has a previous value.
     */
    bool has_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_UNUSED typename synchronization_type::guard guard = const_cast<on_change*>(this)->lock();
        return last_.has_value();
    }

private:
    Callback            cb_;
    castle::optional<T> last_;
};

/**
 * @brief Deduction guide for on_change with an initial value.
 * @tparam V Source value type.
 * @tparam C Source callable type.
 */
template <typename V, typename C>
on_change(V&&, C&&) -> on_change<meta::decay_t<V>, meta::decay_t<C>>;

/**
 * @brief Creates an on_change policy with an initial value while allowing explicit selection of the concurrency mode.
 * @tparam Mode Concurrency mode.
 * @tparam V Source value type.
 * @tparam C Source callable type.
 * @param initial Initial stored value.
 * @param cb Callback stored by value.
 * @return on_change<meta::decay_t<V>, meta::decay_t<C>, Mode>.
 */
template <typename Mode = single_thread, typename V, typename C>
on_change<meta::decay_t<V>, meta::decay_t<C>, Mode> make_on_change(V&& initial, C&& cb)
{
    return on_change<meta::decay_t<V>, meta::decay_t<C>, Mode>(
        CASTLE_FORWARD<V>(initial), CASTLE_FORWARD<C>(cb));
}

/**
 * @brief Creates an on_change policy without an initial value.
 * @tparam T Observed value type.
 * @tparam Mode Concurrency mode.
 * @tparam C Source callable type.
 * @param cb Callback stored by value.
 * @return on_change<T, meta::decay_t<C>, Mode>.
 */
template <typename T, typename Mode = single_thread, typename C>
on_change<T, meta::decay_t<C>, Mode> make_on_change(C&& cb)
{
    return on_change<T, meta::decay_t<C>, Mode>(CASTLE_FORWARD<C>(cb));
}

/**
 * @brief Executes a callback only if enough time has elapsed since the last fire.
 * @tparam Callback Stored callback type.
 * @tparam Mode Concurrency mode.
 * @tparam Clock Clock type supplying `now()`.
 *
 * Extra calls inside the interval are dropped rather than queued.
 */
template <typename Callback, typename Mode = single_thread, typename Clock = castle::chrono::steady_clock>
class throttle : private synchronization<Mode>
{
    using synchronization_type = synchronization<Mode>;

public:
    using callback_type    = Callback;
    using concurrency_type = Mode;
    using clock            = Clock;
    using duration         = typename Clock::duration;
    using time_point       = typename Clock::time_point;

    /**
     * @brief Constructs the policy with a minimum interval between firings.
     * @tparam Rep Source duration representation type.
     * @tparam Period Source duration period.
     * @tparam C Source callable type.
     * @param interval Minimum elapsed time required before firing again.
     * @param cb Callback stored by value.
     */
    template <typename Rep, typename Period, typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, throttle>::value>>
    throttle(castle::chrono::duration<Rep, Period> interval, C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
        , interval_(castle::chrono::duration_cast<duration>(interval))
    {
    }

    /**
     * @brief Fires the callback only when the interval has elapsed since the previous fire.
     * @param args Arguments forwarded to the callback when it fires.
     */
    template <typename... Args>
    void execute(Args&&... args)
    {
        bool fire = false;
        CASTLE_CONST time_point now = Clock::now();

        {
            CASTLE_UNUSED typename synchronization_type::guard guard = this->lock();

            if (!last_.has_value() || ((now - *last_) >= interval_))
            {
                last_ = now;
                fire  = true;
            }
        }

        if (fire)
        {
            cb_(CASTLE_FORWARD<Args>(args)...);
        }
    }

    /**
     * @brief Alias for execute().
     * @param args Arguments forwarded to the callback when it fires.
     */
    template <typename... Args>
    void operator()(Args&&... args)
    {
        execute(CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Clears the last-fire timestamp so the next execute() fires immediately.
     */
    void reset() CASTLE_NOEXCEPT
    {
        CASTLE_UNUSED typename synchronization_type::guard guard = this->lock();
        last_.reset();
    }

    /**
     * @brief Returns the configured minimum interval.
     * @return Interval that must elapse between firings.
     */
    duration interval() CASTLE_CONST CASTLE_NOEXCEPT { return interval_; }

private:
    Callback                     cb_;
    duration                     interval_;
    castle::optional<time_point> last_;
};

/**
 * @brief Deduction guide for throttle.
 * @tparam Rep Source duration representation type.
 * @tparam Period Source duration period.
 * @tparam C Source callable type.
 */
template <typename Rep, typename Period, typename C>
throttle(castle::chrono::duration<Rep, Period>, C&&) -> throttle<meta::decay_t<C>>;

/**
 * @brief Creates a throttle policy while allowing explicit selection of Mode and Clock.
 * @tparam Mode Concurrency mode.
 * @tparam Clock Clock type.
 * @tparam Rep Source duration representation type.
 * @tparam Period Source duration period.
 * @tparam C Source callable type.
 * @param interval Minimum elapsed time required before firing again.
 * @param cb Callback stored by value.
 * @return throttle<meta::decay_t<C>, Mode, Clock>.
 */
template <typename Mode = single_thread, typename Clock = castle::chrono::steady_clock,
          typename Rep, typename Period, typename C>
throttle<meta::decay_t<C>, Mode, Clock>
make_throttle(castle::chrono::duration<Rep, Period> interval, C&& cb)
{
    return throttle<meta::decay_t<C>, Mode, Clock>(interval, CASTLE_FORWARD<C>(cb));
}

/**
 * @brief Fires a callback once for each elapsed period when poll() is called.
 * @tparam Callback Stored callback type.
 * @tparam Mode Concurrency mode.
 * @tparam Clock Clock type supplying `now()`.
 *
 * The schedule advances by exact multiples of the configured period. If multiple periods elapsed since the previous
 * poll(), poll() invokes the callback once per elapsed period so the schedule does not drift.
 *
 * @warning next_deadline() reads a plain member. In concurrent use, callers must externally coordinate unsynchronized
 * observer calls with poll() and reset().
 */
template <typename Callback, typename Mode = single_thread, typename Clock = castle::chrono::steady_clock>
class periodic : private synchronization<Mode>
{
    using synchronization_type = synchronization<Mode>;

public:
    using callback_type    = Callback;
    using concurrency_type = Mode;
    using clock            = Clock;
    using duration         = typename Clock::duration;
    using time_point       = typename Clock::time_point;

    /**
     * @brief Constructs the periodic schedule starting at now + period.
     * @tparam Rep Source duration representation type.
     * @tparam Period Source duration period.
     * @tparam C Source callable type.
     * @param period Desired period between scheduled firings.
     * @param cb Callback stored by value.
     */
    template <typename Rep, typename Period, typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, periodic>::value>>
    periodic(castle::chrono::duration<Rep, Period> period, C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
        , period_(castle::chrono::duration_cast<duration>(period))
        , next_deadline_(Clock::now() + period_)
    {
    }

    /**
     * @brief Polls the schedule and fires once per elapsed period.
     * @param args Arguments forwarded to each callback invocation.
     * @note The callback runs outside the synchronization guard after elapsed periods are counted.
     */
    template <typename... Args>
    void poll(Args&&... args)
    {
        size_type elapsed_periods = 0U;

        {
            CASTLE_UNUSED typename synchronization_type::guard guard = this->lock();

            CASTLE_CONST time_point now = Clock::now();

            while (now >= next_deadline_)
            {
                ++elapsed_periods;
                next_deadline_ += period_;
            }
        }

        while (elapsed_periods != 0U)
        {
            --elapsed_periods;
            cb_(CASTLE_FORWARD<Args>(args)...);
        }
    }

    /**
     * @brief Alias for poll().
     * @param args Arguments forwarded to each callback invocation.
     */
    template <typename... Args>
    void execute(Args&&... args)
    {
        poll(CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Alias for poll().
     * @param args Arguments forwarded to each callback invocation.
     */
    template <typename... Args>
    void operator()(Args&&... args)
    {
        poll(CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Resets the schedule so the next deadline becomes now + period.
     */
    void reset() CASTLE_NOEXCEPT
    {
        CASTLE_UNUSED typename synchronization_type::guard guard = this->lock();
        next_deadline_ = Clock::now() + period_;
    }

    /**
     * @brief Returns the configured period.
     * @return Period used to advance the schedule.
     */
    duration   period()        CASTLE_CONST CASTLE_NOEXCEPT { return period_; }

    /**
     * @brief Returns the next scheduled deadline.
     * @return Current next deadline.
     * @warning This reads a plain member and must not race with poll() or reset() in concurrent use.
     */
    time_point next_deadline() CASTLE_CONST CASTLE_NOEXCEPT { return next_deadline_; }

private:
    Callback   cb_;
    duration   period_;
    time_point next_deadline_;
};

/**
 * @brief Deduction guide for periodic.
 * @tparam Rep Source duration representation type.
 * @tparam Period Source duration period.
 * @tparam C Source callable type.
 */
template <typename Rep, typename Period, typename C>
periodic(castle::chrono::duration<Rep, Period>, C&&) -> periodic<meta::decay_t<C>>;

/**
 * @brief Creates a periodic policy while allowing explicit selection of Mode and Clock.
 * @tparam Mode Concurrency mode.
 * @tparam Clock Clock type.
 * @tparam Rep Source duration representation type.
 * @tparam Period Source duration period.
 * @tparam C Source callable type.
 * @param period Desired period between scheduled firings.
 * @param cb Callback stored by value.
 * @return periodic<meta::decay_t<C>, Mode, Clock>.
 */
template <typename Mode = single_thread, typename Clock = castle::chrono::steady_clock,
          typename Rep, typename Period, typename C>
periodic<meta::decay_t<C>, Mode, Clock>
make_periodic(castle::chrono::duration<Rep, Period> period, C&& cb)
{
    return periodic<meta::decay_t<C>, Mode, Clock>(period, CASTLE_FORWARD<C>(cb));
}

} // namespace policy
} // namespace callbacks
} // namespace castle

#endif // CASTLE_CALLBACKS_EXEC_POLICY_HPP
