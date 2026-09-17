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

// =============================================================================
// castle::callbacks::policy
// -----------------------------------------------------------------------------
// Header-only "control-flow" policies that decide *when* a bound callback
// actually runs. Designed for embedded / resource constrained C++17 code:
//
//   * Zero heap allocation. The callable is stored BY VALUE via a template
//     parameter. No std::function, no shared_ptr/unique_ptr, no owned
//     threads or timers.
//   * Bind once. The callable is captured at construction and never rebound;
//     execute() only forwards its call-site arguments.
//   * Thread-safety is a compile-time parameter: single_thread (no
//     synchronization, default) or concurrent (atomic-based, no mutex). There
//     is exactly one implementation of each policy; the concurrency mode only
//     swaps the storage/primitives it operates on.
//   * Time-based policies (throttle, periodic, armed_window) are poll-driven:
//     the caller pumps execute()/poll() from a cooperative event loop. They
//     own no threads.
//
// API
// -----------------------------------------------------------------------------
// Every control-flow rule is a single class template:
//
//   template <typename Callback, typename Mode = single_thread, ...>
//   class <rule-name>;
//
// so the rule name (once, armed_window, every_n, on_change, throttle,
// periodic) is also the class name - there is no extra namespace or factory
// indirection to look through. Class template argument deduction (CTAD)
// covers the common case, so most call sites never spell out Callback:
//
//   castle::callbacks::policy::once init{[]{ boot_hw(); }};
//   init.execute();
//
//   using namespace castle::chrono::literals::chrono_literals;
//   castle::callbacks::policy::throttle gate{500_ms, [](CASTLE_CONST char* m){ log(m); }};
//
// Selecting the concurrent mode (or a custom Clock) requires a template
// argument that CTAD cannot infer from the constructor call alone, so a
// matching make_*() factory is provided for that case:
//
//   auto flag = castle::callbacks::policy::make_once<
//       castle::callbacks::policy::concurrent>([]{ log_once(); });
//
//   auto ack = castle::callbacks::policy::make_armed_window<
//       castle::callbacks::policy::concurrent>(200_ms, []{ send_ack(); });
//
// Interoperability with castle::callbacks::function / delegate
// -----------------------------------------------------------------------------
// The Callback template parameter can be anything invocable and stored by
// value. Recommended:
//
//   1) A raw callable (lambda, functor, function pointer). Zero indirection,
//      the closure type is inlined into the policy's storage.
//
//   2) A castle::callbacks::function<Sig, N> for type erasure with SBO.
//      Still no heap allocation. Useful when a stable, non-generic stored
//      type is needed (e.g. a member of another class).
//
//        using cb_t = castle::callbacks::function<void()>;
//        castle::callbacks::policy::once<cb_t> init{cb_t{big_lambda}};
//
//   3) One of the compile-time bound castle::callbacks::delegate_*_ct targets
//      - potentially zero storage per policy.
//
// The polymorphic castle::callbacks::delegate_base<Sig> hierarchy is not
// storable by value (it is an abstract interface); bind it through
// castle::callbacks::function instead if type erasure across a stable ABI is
// required.
// =============================================================================

namespace castle
{
namespace callbacks
{
namespace policy
{

// =============================================================================
// Concurrency modes
// =============================================================================
//
// A mode supplies the small, uniform vocabulary every policy below uses to
// manage its internal state:
//
//   atomic_type<T>                              - storage type for a T
//   load(value)                                 - read
//   store(value, desired)                       - write
//   compare_exchange(value, expected, desired)  - CAS; updates expected on
//                                                  failure
//   fetch_add(value, amount)                    - read-modify-write, returns
//                                                  the previous value
//
// single_thread is the default: atomic_type<T> is just T, so a single_thread
// policy costs nothing beyond the plain value and carries no atomic object.
//
// concurrent maps atomic_type<T> onto castle::atomic<T> and each operation
// onto the matching atomic instruction with an appropriate memory order.
// There is no runtime branch such as `if (thread_safe) ...` anywhere in this
// header - the compiler always sees exactly one of the two implementations.
//
// =============================================================================

struct single_thread
{
    template <typename T>
    using atomic_type = T;

    template <typename T>
    static T load(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return value;
    }

    template <typename T>
    static void store(T& value, CASTLE_CONST T& desired) CASTLE_NOEXCEPT
    {
        value = desired;
    }

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

    template <typename T>
    static T fetch_add(T& value, T amount) CASTLE_NOEXCEPT
    {
        CASTLE_CONST T previous = value;
        value += amount;
        return previous;
    }
};


struct concurrent
{
    template <typename T>
    using atomic_type = castle::atomic<T>;

    template <typename T>
    static T load(CASTLE_CONST castle::atomic<T>& value) CASTLE_NOEXCEPT
    {
        return value.load(castle::memory_order_acquire);
    }

    template <typename T>
    static void store(castle::atomic<T>& value, CASTLE_CONST T& desired) CASTLE_NOEXCEPT
    {
        value.store(desired, castle::memory_order_release);
    }

    template <typename T>
    static bool compare_exchange(castle::atomic<T>& value, T& expected, CASTLE_CONST T& desired) CASTLE_NOEXCEPT
    {
        return value.compare_exchange_strong(expected, desired,
                                              castle::memory_order_acq_rel,
                                              castle::memory_order_acquire);
    }

    template <typename T>
    static T fetch_add(castle::atomic<T>& value, T amount) CASTLE_NOEXCEPT
    {
        return value.fetch_add(amount, castle::memory_order_relaxed);
    }
};


// =============================================================================
// synchronization
// =============================================================================
//
// Most policies (once, armed_window, every_n) need only one atomic value and
// therefore talk to Mode::atomic_type<T> directly - no lock required.
//
// A few policies carry state that cannot in general be represented as a
// single castle::atomic<T> payload:
//
//   on_change -> castle::optional<T>            (T is arbitrary)
//   throttle  -> castle::optional<time_point>
//   periodic  -> duration + time_point pair
//
// Those use synchronization<Mode> for a short critical section around the
// state transition only; the callback itself always runs outside the lock.
//
// single_thread gets an empty guard (no lock, no atomic member): the default
// policy therefore carries no synchronization overhead at all. concurrent
// reuses castle::mutex / castle::scoped_mutex (the same spinlock every other
// castle::sync primitive is built on) rather than hand-rolling another CAS
// loop: one audited spinlock implementation, and integrators who swap
// castle::spin_wait for a real RTOS yield/block WaitPolicy get that benefit
// here for free.
//
// =============================================================================

template <typename Mode>
class synchronization;


template <>
class synchronization<single_thread>
{
public:
    class guard
    {
    public:
        explicit guard(synchronization&) CASTLE_NOEXCEPT {}
    };

    guard lock() CASTLE_NOEXCEPT
    {
        return guard(*this);
    }
};


template <>
class synchronization<concurrent>
{
public:
    class guard
    {
    public:
        explicit guard(synchronization& owner) CASTLE_NOEXCEPT
            : scoped_(owner.mutex_)
        {
        }

        guard(CASTLE_CONST guard&) CASTLE_DELETE;
        guard& operator=(CASTLE_CONST guard&) CASTLE_DELETE;

    private:
        castle::scoped_mutex scoped_;
    };

    guard lock() CASTLE_NOEXCEPT
    {
        return guard(*this);
    }

private:
    castle::mutex mutex_;
};


// =============================================================================
// once - fire the callback exactly once during the object's lifetime.
// =============================================================================
//
// The "fired" latch is set before the callback runs, so a recursive
// execute() (called from inside the callback itself) is a no-op.
//
// reset() is a coordination operation: in concurrent mode the caller must
// ensure no execute() is in flight while resetting.
//
//   castle::callbacks::policy::once init{[]{ boot_hw(); }};
//   init.execute();          // runs boot_hw()
//   init.execute();          // no-op, already fired
//
// =============================================================================

template <typename Callback, typename Mode = single_thread>
class once
{
public:
    using callback_type    = Callback;
    using concurrency_type = Mode;

    template <typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, once>::value>>
    explicit once(C&& cb) CASTLE_NOEXCEPT(meta::is_nothrow_constructible<Callback, C&&>::value)
        : cb_(CASTLE_FORWARD<C>(cb))
    {
    }

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

    template <typename... Args>
    void operator()(Args&&... args)
    {
        execute(CASTLE_FORWARD<Args>(args)...);
    }

    void reset() CASTLE_NOEXCEPT
    {
        Mode::store(fired_, false);
    }

    bool has_fired() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Mode::load(fired_);
    }

private:
    Callback cb_;
    typename Mode::template atomic_type<bool> fired_{false};
};

// CTAD: deduce Callback from the constructor argument; Mode stays single_thread.
template <typename C>
once(C&&) -> once<meta::decay_t<C>>;

// Factory for selecting a Mode that CTAD cannot infer (e.g. concurrent).
template <typename Mode = single_thread, typename C>
once<meta::decay_t<C>, Mode> make_once(C&& cb)
{
    return once<meta::decay_t<C>, Mode>(CASTLE_FORWARD<C>(cb));
}


// =============================================================================
// armed_window - fire the callback AT MOST ONCE, and only while the object is
// still inside its validity window. The window opens at construction (or on
// reset()/rearm()) and closes `duration` later.
//
//     [ t0 .............. t0 + duration ]   <- armed
//                                        \___ after this point: disarmed,
//                                             execute() is a permanent no-op
//                                             until rearm()/reset() is called.
//
//   * Before the deadline AND not yet fired -> execute() fires exactly once,
//     latches "fired" and disarms further calls.
//   * Before the deadline but already fired  -> execute() is a no-op.
//   * After the deadline (fired or not)      -> execute() is a no-op and
//     latches "expired" (deterministic timeout, no timer thread needed).
//
//   using namespace castle::chrono::literals::chrono_literals;
//   castle::callbacks::policy::armed_window ack{200_ms, []{ send_ack(); }};
//   ack.execute();   // fires the first time, latches to "fired"
//   ack.execute();   // no-op (already fired)
// =============================================================================

template <typename Callback, typename Mode = single_thread, typename Clock = castle::chrono::steady_clock>
class armed_window
{
public:
    using callback_type    = Callback;
    using concurrency_type = Mode;
    using clock            = Clock;
    using duration         = typename Clock::duration;
    using time_point       = typename Clock::time_point;

    // Latch encoded in a single atomic so the "check + fire" transition is a
    // lock-free CAS in concurrent mode.
    enum class state : uint8_t
    {
        armed   = 0, // window open, callback has not fired yet
        fired   = 1, // callback fired inside the window, disarmed
        expired = 2  // window closed without firing, disarmed
    };

    template <typename Rep, typename Period, typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, armed_window>::value>>
    armed_window(castle::chrono::duration<Rep, Period> window, C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
        , duration_(castle::chrono::duration_cast<duration>(window))
        , deadline_(Clock::now() + duration_)
        , state_(state::armed)
    {
    }

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
            // Deadline elapsed before the first successful fire: latch as
            // expired so subsequent execute() calls are constant-time no-ops.
            Mode::compare_exchange(state_, expected, state::expired);
            return;
        }

        if (!Mode::compare_exchange(state_, expected, state::fired))
        {
            return;
        }

        cb_(CASTLE_FORWARD<Args>(args)...);
    }

    template <typename... Args>
    void operator()(Args&&... args)
    {
        execute(CASTLE_FORWARD<Args>(args)...);
    }

    // Open a fresh window of the SAME duration starting from now. Also clears
    // any prior fired/expired latch. Equivalent to "re-arm the one-shot".
    void reset() CASTLE_NOEXCEPT
    {
        deadline_ = Clock::now() + duration_;
        Mode::store(state_, state::armed);
    }

    // Open a fresh window of a NEW duration starting from now.
    template <typename Rep, typename Period>
    void rearm(castle::chrono::duration<Rep, Period> window) CASTLE_NOEXCEPT
    {
        duration_ = castle::chrono::duration_cast<duration>(window);
        deadline_ = Clock::now() + duration_;
        Mode::store(state_, state::armed);
    }

    // Force the window closed WITHOUT invoking the callback. Useful to cancel
    // a pending one-shot when an external condition already handled it.
    void expire() CASTLE_NOEXCEPT
    {
        state expected = state::armed;
        Mode::compare_exchange(state_, expected, state::expired);
    }

    bool armed()   CASTLE_CONST CASTLE_NOEXCEPT { return Mode::load(state_) == state::armed; }
    bool fired()   CASTLE_CONST CASTLE_NOEXCEPT { return Mode::load(state_) == state::fired; }
    bool expired() CASTLE_CONST CASTLE_NOEXCEPT { return Mode::load(state_) == state::expired; }

    duration   window()   CASTLE_CONST CASTLE_NOEXCEPT { return duration_; }
    time_point deadline() CASTLE_CONST CASTLE_NOEXCEPT { return deadline_; }

private:
    Callback   cb_;
    duration   duration_;
    time_point deadline_;
    typename Mode::template atomic_type<state> state_;
};

// CTAD: default clock (castle::chrono::steady_clock), single_thread mode.
template <typename Rep, typename Period, typename C>
armed_window(castle::chrono::duration<Rep, Period>, C&&) -> armed_window<meta::decay_t<C>>;

// Factory for selecting Mode and/or a custom Clock.
template <typename Mode = single_thread, typename Clock = castle::chrono::steady_clock,
          typename Rep, typename Period, typename C>
armed_window<meta::decay_t<C>, Mode, Clock>
make_armed_window(castle::chrono::duration<Rep, Period> window, C&& cb)
{
    return armed_window<meta::decay_t<C>, Mode, Clock>(window, CASTLE_FORWARD<C>(cb));
}


// =============================================================================
// every_n - fire the callback once every N execute() calls.
//
// The counter/threshold check is a single fetch_add + modulo in both modes,
// so concurrent callers race on the counter only; at most one caller ever
// observes the "Nth call" condition and fires.
//
//   castle::callbacks::policy::every_n heartbeat{100, []{ toggle_led(); }};
//   heartbeat.execute(); // fires on the 100th call, then every 100th after
// =============================================================================

template <typename Callback, typename Mode = single_thread>
class every_n
{
public:
    using callback_type    = Callback;
    using concurrency_type = Mode;

    template <typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, every_n>::value>>
    every_n(size_type n, C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
        , n_(n == 0U ? 1U : n)
    {
    }

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

    template <typename... Args>
    void operator()(Args&&... args)
    {
        execute(CASTLE_FORWARD<Args>(args)...);
    }

    void reset() CASTLE_NOEXCEPT
    {
        Mode::store(counter_, static_cast<size_type>(0U));
    }

    size_type interval() CASTLE_CONST CASTLE_NOEXCEPT { return n_; }

private:
    Callback  cb_;
    size_type n_;
    typename Mode::template atomic_type<size_type> counter_{0U};
};

// CTAD: deduce Callback; N is a runtime constructor argument.
template <typename C>
every_n(size_type, C&&) -> every_n<meta::decay_t<C>>;

// Factory for selecting a Mode that CTAD cannot infer.
template <typename Mode = single_thread, typename C>
every_n<meta::decay_t<C>, Mode> make_every_n(size_type n, C&& cb)
{
    return every_n<meta::decay_t<C>, Mode>(n, CASTLE_FORWARD<C>(cb));
}

// Factory with a compile-time constant N. Prefer this when the ratio is fixed
// at build time: the modulo becomes a strength-reduced immediate for
// power-of-two values.
//
//   auto stats = castle::callbacks::policy::make_every_n_ct<64>([]{ dump(); });
template <size_type N, typename Mode = single_thread, typename C>
every_n<meta::decay_t<C>, Mode> make_every_n_ct(C&& cb)
{
    static_assert(N > 0U, "make_every_n_ct requires N > 0");
    return every_n<meta::decay_t<C>, Mode>(N, CASTLE_FORWARD<C>(cb));
}


// =============================================================================
// on_change - fire only when the observed value differs from the previously
// stored one. State is held in-place via castle::optional (no heap).
//
// The comparison and state update form one critical section in concurrent
// mode (see synchronization<Mode>); the callback is invoked after releasing
// the lock, so it may itself call back into the policy without deadlocking.
//
//   castle::callbacks::policy::on_change<int> watch{[](int v){ log(v); }};
//   watch(3); // fires (no previous value)
//   watch(3); // no-op, unchanged
//   watch(4); // fires
// =============================================================================

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

    // Construct without an initial value; the first execute() always fires.
    template <typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, on_change>::value>>
    explicit on_change(C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
    {
    }

    // Construct with an initial value; execute(v) fires only if v != initial.
    template <typename V, typename C>
    on_change(V&& initial, C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
        , last_(meta::in_place, CASTLE_FORWARD<V>(initial))
    {
    }

    template <typename U>
    void execute(U&& new_value)
    {
        bool changed = false;

        {
            CASTLE_UNUSED typename synchronization_type::guard guard = this->lock();

            if (last_.has_value() && (*last_ == new_value))
            {
                return;
            }

            last_.emplace(CASTLE_FORWARD<U>(new_value));
            changed = true;
        }

        if (changed)
        {
            cb_(*last_);
        }
    }

    template <typename U>
    void operator()(U&& new_value)
    {
        execute(CASTLE_FORWARD<U>(new_value));
    }

    void reset() CASTLE_NOEXCEPT(meta::is_nothrow_destructible<T>::value)
    {
        CASTLE_UNUSED typename synchronization_type::guard guard = this->lock();
        last_.reset();
    }

    bool has_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_UNUSED typename synchronization_type::guard guard = const_cast<on_change*>(this)->lock();
        return last_.has_value();
    }

private:
    Callback            cb_;
    castle::optional<T> last_;
};

// CTAD: with an initial value, both T and Callback are deduced.
template <typename V, typename C>
on_change(V&&, C&&) -> on_change<meta::decay_t<V>, meta::decay_t<C>>;

// Factory with an initial value; T is deduced from `initial`.
template <typename Mode = single_thread, typename V, typename C>
on_change<meta::decay_t<V>, meta::decay_t<C>, Mode> make_on_change(V&& initial, C&& cb)
{
    return on_change<meta::decay_t<V>, meta::decay_t<C>, Mode>(
        CASTLE_FORWARD<V>(initial), CASTLE_FORWARD<C>(cb));
}

// Factory without an initial value; T cannot be deduced, so it is explicit.
//
//   auto w = castle::callbacks::policy::make_on_change<int>([](int v){ ... });
template <typename T, typename Mode = single_thread, typename C>
on_change<T, meta::decay_t<C>, Mode> make_on_change(C&& cb)
{
    return on_change<T, meta::decay_t<C>, Mode>(CASTLE_FORWARD<C>(cb));
}


// =============================================================================
// throttle - rate-limit. Fire only if `interval` has elapsed since the last
// fire; extra invocations within the interval are dropped (not queued).
// Poll-driven: no thread, no timer. Suitable for main-loop pumping.
//
// The optional time_point is protected by synchronization<Mode> because a
// time_point is not assumed to be a suitable castle::atomic payload. The
// critical section only decides whether this invocation owns the fire; the
// callback runs outside it.
//
//   using namespace castle::chrono::literals::chrono_literals;
//   castle::callbacks::policy::throttle gate{500_ms, [](CASTLE_CONST char* m){ log(m); }};
// =============================================================================

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

    template <typename Rep, typename Period, typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, throttle>::value>>
    throttle(castle::chrono::duration<Rep, Period> interval, C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
        , interval_(castle::chrono::duration_cast<duration>(interval))
    {
    }

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

    template <typename... Args>
    void operator()(Args&&... args)
    {
        execute(CASTLE_FORWARD<Args>(args)...);
    }

    // Force the next execute() to fire regardless of the elapsed interval.
    void reset() CASTLE_NOEXCEPT
    {
        CASTLE_UNUSED typename synchronization_type::guard guard = this->lock();
        last_.reset();
    }

    duration interval() CASTLE_CONST CASTLE_NOEXCEPT { return interval_; }

private:
    Callback                     cb_;
    duration                     interval_;
    castle::optional<time_point> last_;
};

// CTAD: default clock (castle::chrono::steady_clock), single_thread mode.
template <typename Rep, typename Period, typename C>
throttle(castle::chrono::duration<Rep, Period>, C&&) -> throttle<meta::decay_t<C>>;

// Factory for selecting Mode and/or a custom Clock.
template <typename Mode = single_thread, typename Clock = castle::chrono::steady_clock,
          typename Rep, typename Period, typename C>
throttle<meta::decay_t<C>, Mode, Clock>
make_throttle(castle::chrono::duration<Rep, Period> interval, C&& cb)
{
    return throttle<meta::decay_t<C>, Mode, Clock>(interval, CASTLE_FORWARD<C>(cb));
}


// =============================================================================
// periodic - poll-driven periodic callback.
//
// The schedule is t0 + period, t0 + 2*period, t0 + 3*period, ... . If several
// periods elapse between polls, the callback fires once per elapsed period so
// the schedule does not drift.
//
// In concurrent mode the schedule calculation (how many periods elapsed, and
// advancing the deadline) is protected by a short synchronization<Mode>
// section; the callback runs outside the lock, so multiple concurrent
// callers distribute the catch-up callbacks among themselves without
// serializing the callback body. This avoids requiring
// castle::atomic<Clock::time_point>, which would impose an unnecessary
// restriction on Clock::time_point.
//
//   using namespace castle::chrono::literals::chrono_literals;
//   castle::callbacks::policy::periodic tick{1_s, []{ housekeeping(); }};
//   tick.poll(); // call from the event loop
// =============================================================================

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

    template <typename Rep, typename Period, typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, periodic>::value>>
    periodic(castle::chrono::duration<Rep, Period> period, C&& cb)
        : cb_(CASTLE_FORWARD<C>(cb))
        , period_(castle::chrono::duration_cast<duration>(period))
        , next_deadline_(Clock::now() + period_)
    {
    }

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

    template <typename... Args>
    void execute(Args&&... args)
    {
        poll(CASTLE_FORWARD<Args>(args)...);
    }

    template <typename... Args>
    void operator()(Args&&... args)
    {
        poll(CASTLE_FORWARD<Args>(args)...);
    }

    // Snap the next deadline to now + period (discards accumulated lag).
    void reset() CASTLE_NOEXCEPT
    {
        CASTLE_UNUSED typename synchronization_type::guard guard = this->lock();
        next_deadline_ = Clock::now() + period_;
    }

    duration   period()        CASTLE_CONST CASTLE_NOEXCEPT { return period_; }
    time_point next_deadline() CASTLE_CONST CASTLE_NOEXCEPT { return next_deadline_; }

private:
    Callback   cb_;
    duration   period_;
    time_point next_deadline_;
};

// CTAD: default clock (castle::chrono::steady_clock), single_thread mode.
template <typename Rep, typename Period, typename C>
periodic(castle::chrono::duration<Rep, Period>, C&&) -> periodic<meta::decay_t<C>>;

// Factory for selecting Mode and/or a custom Clock.
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
