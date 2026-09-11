#ifndef CASTLE_EVENTS_INPLACE_SIGNAL_IPC_EVENT_H
#define CASTLE_EVENTS_INPLACE_SIGNAL_IPC_EVENT_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/error/status.h"
#include "castle/utility/forward.h"
#include "castle/utility/tuple.h"
#include "castle/utility/bitset.h"
#include "castle/atomic/atomic.h"

#include "castle/callbacks/inplace_function.h"
#include "castle/callbacks/inplace_callback_registry.h"
#include "castle/events/signal_ipc_config.h"

#include <signal.h>

namespace castle
{
namespace events
{

// -----------------------------------------------------------------------------
// inplace_signal_event - compile-time, signum-keyed OS signal dispatcher
// built on top of callbacks::inplace_callback_registry (owning callbacks).
//
// Configuration model:
//   Each managed signal is described independently by a signal_ipc_config<...>
//   entry so callback capacity AND callback storage (size + alignment) are
//   PER-SIGNAL properties:
//
//       using signals_t = inplace_signal_event<
//           signal_ipc_config<signal::sigint,  3, 64>,
//           signal_ipc_config<signal::sigusr1, 2, 32>
//       >;
//
//   Storage-size / alignment defaults live in signal_ipc_config, so the shortest
//   form still compiles:
//
//       signal_ipc_config<signal::sigusr2, 2>
//
//   This mirrors signal_event's configuration surface: the same
//   signal_ipc_config template describes both the non-owning and the owning
//   dispatcher, only the fields consumed differ.
//
// Design (all-static, namespace-shaped):
//   - The set of signals handled is fixed at compile time via the
//     SignalConfigs... pack. Each signal is resolved to a compile-time
//     tuple index — no runtime search on the hot paths, no hashing, no heap.
//   - Every signal owns its own callbacks::inplace_callback_registry<
//         max_callback, void(), storage_size, storage_alignment>.
//     The registry stores each subscribed callable BY VALUE in a fixed
//     inline buffer, so no heap allocation happens even for stateful
//     lambdas. The signal identity is already encoded at register-time
//     (Signum template argument), so callbacks take no runtime arguments.
//
// Immediate dispatch model:
//   - When a subscribed signal is delivered by the OS, the installed
//     handler walks the matching registry and invokes every active
//     callback DIRECTLY from signal context. There is no deferred queue,
//     no dispatch_pending() step.
//   - The library-owned dispatch path is intentionally minimal: a bounded
//     signum lookup, a bitset test, and a walk of callbacks::inplace_function slots
//     invoked via a stored function pointer — no allocation, no locking,
//     no stdio, no re-entrancy on any library-owned mutex.
//   - A single atomic "ready" gate synchronises setup with signal
//     delivery: install() is the release barrier that publishes every
//     prior register_callback() to the OS handler; the handler performs
//     an acquire-load and bails out if setup is incomplete or if
//     uninstall() has already begun tearing down. This closes the window
//     where a signal fired between arming sigaction and finishing user
//     setup could observe half-initialised registry state, especially
//     when the signal is delivered to a different thread than the one
//     doing setup (POSIX allows arbitrary thread delivery).
//
//   *** ASYNC-SIGNAL-SAFETY WARNING ***
//   Because callbacks execute in signal context, THE USER CALLBACK BODY
//   MUST ITSELF BE ASYNC-SIGNAL-SAFE. In practice this means:
//     - No malloc / new / delete (avoid std::string, std::vector, ...).
//     - No mutex / condition variable / any locking primitive that could
//       already be held by the interrupted thread.
//     - No stdio (printf, std::cout, ...). Use write(2) on STDERR_FILENO
//       if you need diagnostics.
//     - Only the functions listed as async-signal-safe by POSIX
//       (signal-safety(7)) may be called.
//   The callbacks::inplace_function machinery itself is neutral: subscription only
//   copies/moves the callable into its inline buffer at register time
//   (which does not run in signal context), and dispatch is a single
//   indirect call. The unsafe part is whatever code you write inside the
//   target. "inplace" eliminates heap for the callback OBJECT — it does
//   not make the callback BODY signal-safe. If you cannot meet these
//   constraints, do NOT use this handler directly — instead have the
//   callback set a std::atomic_flag / write a byte to a self-pipe and do
//   the real work from a normal thread.
//
// Template parameters:
//   SignalConfigs... - one signal_ipc_config<Signal, MaxCallback,
//                      StorageSize, StorageAlignment> per managed signal.
//                      The pack must contain no duplicate signum values.
//
// Usage:
//   using castle::events::signal;
//   using signals_t = castle::events::inplace_signal_event<
//       signal_ipc_config<signal::sigint,  3, 64>,
//       signal_ipc_config<signal::sigterm, 1, 32>
//   >;
//
//   // Lambda with capture — owned by the registry, no external storage.
//   volatile std::sig_atomic_t stop = 0;
//   auto sub = signals_t::register_callback<signal::sigint>(
//       [&stop]() { stop = 1; }
//   );
//
//   signals_t::install();
//
//   // ... run the program; callbacks fire directly from signal context ...
//
//   sub.unsubscribe();
//   signals_t::uninstall();
// -----------------------------------------------------------------------------
template <typename... SignalConfigs>
class inplace_signal_event
{
private:
    // -------------------------------------------------------------------------
    // Basic pack sanity.
    // -------------------------------------------------------------------------
    static_assert(sizeof...(SignalConfigs) > 0,
                  "inplace_signal_event requires at least one signal_ipc_config");

    // -------------------------------------------------------------------------
    // Reject anything that is not a signal_ipc_config<...>. Matching the
    // partial specialisation of the primary signal_ipc_config template gives a
    // clean static_assert on misuse.
    // -------------------------------------------------------------------------
    template <typename T>
    struct is_valid_signal_config : meta::false_type {};

    template <
        signal Signal,
        size_type MaxCallback,
        size_type StorageSize,
        size_type StorageAlignment>
    struct is_valid_signal_config<signal_ipc_config<Signal, MaxCallback, StorageSize, StorageAlignment>>
        : meta::true_type {};

    static_assert(meta::conjunction<is_valid_signal_config<SignalConfigs>...>::value,
                  "inplace_signal_event accepts only signal_ipc_config<...> template arguments");

    // -------------------------------------------------------------------------
    // Extract per-config traits. inplace_signal_event uses every field:
    // signum, max_callback, storage_size, storage_alignment.
    // -------------------------------------------------------------------------
    template <typename Config>
    struct config_traits;

    template <
        signal Signal,
        size_type MaxCallback,
        size_type StorageSize,
        size_type StorageAlignment>
    struct config_traits<signal_ipc_config<Signal, MaxCallback, StorageSize, StorageAlignment>>
    {
        static constexpr signal signum = Signal;
        static constexpr size_type max_callback = MaxCallback;
        static constexpr size_type storage_size = StorageSize;
        static constexpr size_type storage_alignment = StorageAlignment;

        static_assert(MaxCallback > 0,
                      "inplace_signal_event: signal_ipc_config::MaxCallback must be > 0");
        static_assert(StorageSize > 0,
                      "inplace_signal_event: signal_ipc_config::StorageSize must be > 0");
        static_assert(StorageAlignment > 0,
                      "inplace_signal_event: signal_ipc_config::StorageAlignment must be > 0");
    };

    // -------------------------------------------------------------------------
    // Duplicate-signal detection.
    //
    // Two signal_ipc_config entries with the same signum would create two
    // independent registries for the same OS signal, only the first of which
    // is reachable via compile-time lookup — almost certainly a bug.
    //
    // The recursion uses only partial specialisations — C++ forbids
    // explicit (full) specialisation of a nested template at class scope.
    // -------------------------------------------------------------------------
    template <signal Target, typename... Rest>
    struct contains_signal;

    // Empty-pack base case (partial specialisation: Target still open).
    template <signal Target>
    struct contains_signal<Target> : meta::false_type {};

    // Recursive case.
    template <signal Target, typename First, typename... Rest>
    struct contains_signal<Target, First, Rest...>
        : meta::integral_constant<bool,
              (config_traits<First>::signum == Target)
              || contains_signal<Target, Rest...>::value>
    {};

    template <typename... Configs>
    struct configs_are_unique;

    // Single-element base case — trivially unique. This doubles as the
    // recursion terminator because the SignalConfigs... pack is guaranteed
    // non-empty by the static_assert above.
    template <typename Head>
    struct configs_are_unique<Head> : meta::true_type {};

    // Recursive case: Head unique against Tail, plus Tail unique amongst
    // itself.
    template <typename Head, typename Next, typename... Tail>
    struct configs_are_unique<Head, Next, Tail...>
        : meta::integral_constant<bool,
              !contains_signal<config_traits<Head>::signum, Next, Tail...>::value
              && configs_are_unique<Next, Tail...>::value>
    {};

    static_assert(configs_are_unique<SignalConfigs...>::value,
                  "inplace_signal_event: the SignalConfigs... pack must not contain duplicate signum values");

    // -------------------------------------------------------------------------
    // Per-signal registry type — one owning callbacks::inplace_callback_registry sized
    // by that config's max_callback / storage_size / storage_alignment.
    // -------------------------------------------------------------------------
    template <typename Config>
    using registry_type_for = callbacks::inplace_callback_registry<
        config_traits<Config>::max_callback,
        void(),
        config_traits<Config>::storage_size,
        config_traits<Config>::storage_alignment>;

    using registry_tuple = castle::tuple<registry_type_for<SignalConfigs>...>;

public:
    using error = castle::status;
    using subscription = callbacks::callback_subscription;

    static constexpr size_type signal_count = sizeof...(SignalConfigs);

    // The class is a pure static namespace — no instances, ever. Enforcing
    // this at the type level is clearer than a "singleton runtime check".
    inplace_signal_event() CASTLE_DELETE;
    ~inplace_signal_event() CASTLE_DELETE;
    inplace_signal_event(CASTLE_CONST inplace_signal_event&) CASTLE_DELETE;
    inplace_signal_event& operator=(CASTLE_CONST inplace_signal_event&) CASTLE_DELETE;
    inplace_signal_event(inplace_signal_event&&) CASTLE_DELETE;
    inplace_signal_event& operator=(inplace_signal_event&&) CASTLE_DELETE;

    // -------------------------------------------------------------------------
    // Compile-time capacity queries.
    // -------------------------------------------------------------------------
    static constexpr size_type signal_capacity() noexcept
    {
        return signal_count;
    }

    template <signal Signum>
    static constexpr size_type callback_capacity() noexcept
    {
        return config_traits<config_for<Signum>>::max_callback;
    }

    template <signal Signum>
    static constexpr size_type callback_storage_size() noexcept
    {
        return config_traits<config_for<Signum>>::storage_size;
    }

    template <signal Signum>
    static constexpr size_type callback_storage_alignment() noexcept
    {
        return config_traits<config_for<Signum>>::storage_alignment;
    }

    // -------------------------------------------------------------------------
    // Install the OS-level handler for every signal in the pack. Signals
    // start enabled. Safe to call multiple times — subsequent calls simply
    // re-arm sigaction with the same handler.
    //
    // Ordering contract: any register_callback() / enable_signal() /
    // clear_signal() call sequenced-before install() is guaranteed to be
    // visible to the OS handler on any thread (release-store publishes
    // the setup; the handler's acquire-load synchronises with it).
    // Registrations issued AFTER install() are inherently racy against
    // in-flight signal delivery and should be treated as single-writer.
    // -------------------------------------------------------------------------
    static error install() noexcept
    {
        // Force any concurrent os_handler invocation to bail before we
        // touch enabled_ / sigaction state.
        ready_.store(false, castle::memory_order_release);

        enabled_.set();

        struct sigaction sa{};
        sa.sa_handler = &inplace_signal_event::os_handler;
        sigemptyset(&sa.sa_mask);
        // SA_RESTART: transparently restart interrupted syscalls in user
        // code; the library never blocks in the handler itself.
        sa.sa_flags = SA_RESTART;

        for (size_type i = 0; i < signal_count; ++i)
        {
            if (::sigaction(signal_list_[i], &sa, nullptr) != 0)
            {
                // Roll back so the process is not left in a half-armed
                // state (some signals delivered to our handler, others
                // still to whatever was previously installed).
                uninstall();
                return error::system_call_error;
            }
        }

        // Release: publishes all prior setup (registries, enable flags,
        // sigaction arming) to any thread that observes ready_ == true
        // via the matching acquire-load in os_handler.
        ready_.store(true, castle::memory_order_release);
        return error::ok;
    }

    // -------------------------------------------------------------------------
    // Restore SIG_DFL for every managed signal. Safe to call multiple
    // times. Existing subscriptions are kept in their registries (so a
    // subsequent install() re-arms transparently); call clear() first if
    // you also want to drop them.
    //
    // Ordering: the ready gate is closed BEFORE sigaction is rewound, so
    // an in-flight os_handler on another thread observes ready_ == false
    // via its acquire-load and bails out without touching the registries.
    // -------------------------------------------------------------------------
    static void uninstall() noexcept
    {
        // Close the gate first so any concurrent handler bails before we
        // start mutating OS-level dispositions.
        ready_.store(false, castle::memory_order_release);

        struct sigaction sa{};
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        for (size_type i = 0; i < signal_count; ++i)
        {
            // Ignore failure — no meaningful recovery available and no user
            // callback context to report to.
            (void)::sigaction(signal_list_[i], &sa, nullptr);
        }
    }

    static bool is_installed() noexcept
    {
        return ready_.load(castle::memory_order_acquire);
    }

    // -------------------------------------------------------------------------
    // Register a callback for the given signal. Signature is void() — the
    // signal identity is already fixed by the Signum template argument, so
    // no runtime payload is passed.
    //
    // The registry OWNS the callback (copied/moved into the callbacks::inplace_function
    // inline buffer), so lambdas with captures, functor objects, and
    // std::bind results can all be subscribed without external lifetime
    // management. The callable must satisfy callbacks::inplace_function's size /
    // alignment constraints declared by signal_ipc_config for this signum —
    // checked by a static_assert inside callbacks::inplace_function.
    //
    // NOTE: the callback body will execute in signal context — see the
    // async-signal-safety warning at the top of this file.
    // -------------------------------------------------------------------------
    template <signal Signum, typename Callable>
    static subscription register_callback(Callable&& callback, error* out_error = nullptr)
    {
        constexpr size_type idx = index_of<Signum>();
        status inner_error = status::ok;

        subscription sub = castle::get<idx>(registries_).subscribe(
            CASTLE_FORWARD<Callable>(callback), &inner_error);

        if (out_error != nullptr)
        {
            *out_error = inner_error;
        }

        return sub;
    }

    // -------------------------------------------------------------------------
    // Per-signal enable / disable / query. A disabled signal still runs
    // the OS handler (we cannot inhibit delivery from user space cheaply)
    // but the handler returns without invoking any callback. Useful for
    // temporary suppression without touching sigaction / sigprocmask.
    // -------------------------------------------------------------------------
    template <signal Signum>
    static void enable_signal() noexcept
    {
        enabled_.set(index_of<Signum>());
    }

    template <signal Signum>
    static void disable_signal() noexcept
    {
        enabled_.reset(index_of<Signum>());
    }

    template <signal Signum>
    static bool is_signal_enabled() noexcept
    {
        return enabled_.test(index_of<Signum>());
    }

    // -------------------------------------------------------------------------
    // Clear subscriptions for one signal / all signals. Outstanding
    // subscription handles for cleared slots become stale. Cleared
    // callback objects are destroyed (the registry owns them).
    // -------------------------------------------------------------------------
    template <signal Signum>
    static void clear_signal() noexcept
    {
        castle::get<index_of<Signum>()>(registries_).clear();
    }

    static void clear() noexcept
    {
        clear_all_impl(castle::sequence::index_sequence_for<SignalConfigs...>{});
    }

    template <signal Signum>
    static size_type subscriber_count() noexcept
    {
        return castle::get<index_of<Signum>()>(registries_).size();
    }

private:
    // -------------------------------------------------------------------------
    // Compile-time signum -> tuple index inside the SignalConfigs... pack.
    //
    // Implemented as a constexpr function template with `if constexpr` so
    // only the branch corresponding to the actual match state is
    // instantiated — no runaway recursion after the first hit, and the
    // "unknown signal" diagnostic surfaces exactly once, at the moment
    // index_of<Signum>() is used.
    // -------------------------------------------------------------------------
    template <signal Target, size_type I, typename First, typename... Rest>
    static constexpr size_type index_of_scan() noexcept
    {
        if constexpr (config_traits<First>::signum == Target)
        {
            return I;
        }
        else if constexpr (sizeof...(Rest) > 0)
        {
            return index_of_scan<Target, I + 1, Rest...>();
        }
        else
        {
            static_assert(config_traits<First>::signum == Target,
                          "inplace_signal_event: signal is not present in the SignalConfigs pack");
            return 0;
        }
    }

    template <signal Signum>
    static constexpr size_type index_of() noexcept
    {
        return index_of_scan<Signum, 0, SignalConfigs...>();
    }

    // signal_ipc_config bound to a specific signum — resolves via the tuple.
    template <signal Signum>
    using config_for = castle::tuple_element_t<index_of<Signum>(), castle::tuple<SignalConfigs...>>;

    // Runtime lookup used *only* from the async-signal handler. A tiny
    // linear scan over signal_list_ — async-signal-safe by construction
    // (no library calls, no allocation, just integer comparisons).
    static size_type index_of_runtime(int signum) noexcept
    {
        for (size_type i = 0; i < signal_count; ++i)
        {
            if (signal_list_[i] == signum)
            {
                return i;
            }
        }
        return signal_count; // sentinel: unknown signal
    }

    // -------------------------------------------------------------------------
    // Runtime dispatch into the heterogeneous registry tuple.
    //
    // Each signal_ipc_config may declare a different MaxCallback / StorageSize
    // / StorageAlignment, so the registries have different concrete types
    // and cannot be indexed by a runtime integer via castle::get. A bounded
    // recursion visits the tuple and invokes the registry whose index
    // matches target_index — the recursion depth is fixed at compile time
    // and every step is trivial, so the compiler collapses this into a
    // jump table / cascaded compare.
    // -------------------------------------------------------------------------
    template <size_type Index>
    static void invoke_impl(size_type target_index) noexcept
    {
        if constexpr (Index < signal_count)
        {
            if (Index == target_index)
            {
                castle::get<Index>(registries_).invoke();
                return;
            }

            invoke_impl<Index + 1>(target_index);
        }
    }

    // Sequence clear() across every tuple element without fold expressions
    // in the body (index_sequence-driven expansion is C++14 friendly).
    template <size_type... Is>
    static void clear_all_impl(castle::sequence::index_sequence<Is...>) noexcept
    {
        int dummy[] = {
            (castle::get<Is>(registries_).clear(), 0)...
        };
        (void)dummy;
    }

    // -------------------------------------------------------------------------
    // OS-installed handler. Runs in signal context and dispatches
    // callbacks IMMEDIATELY. The library-owned portion of this path is
    // strictly async-signal-safe: a bounded signum lookup, a bitset test,
    // and a walk of callbacks::inplace_function slots invoked via a stored function
    // pointer. It never allocates, never locks, never touches stdio.
    // User callback bodies must uphold the same discipline (see file-level
    // warning).
    // -------------------------------------------------------------------------
    static void os_handler(int signum) noexcept
    {
        // Acquire: pairs with the release-store in install(). If setup is
        // not yet complete (or uninstall() has begun), bail out before
        // touching any registry state.
        if (!ready_.load(castle::memory_order_acquire))
        {
            return;
        }

        CASTLE_CONST size_type idx = index_of_runtime(signum);
        if (idx >= signal_count)
        {
            return;
        }

        if (!enabled_.test(idx))
        {
            return;
        }

        invoke_impl<0>(idx);
    }

    // -------------------------------------------------------------------------
    // Storage. All static — one set of slots per instantiation, matching
    // the process-global nature of POSIX signal disposition.
    // -------------------------------------------------------------------------

    // Per-signal callback registries. Each registry is sized independently
    // by its signal_ipc_config and OWNS its callbacks by value via
    // callbacks::inplace_function's inline buffer — zero heap.
    static inline registry_tuple registries_{};

    // Per-signal enable flags — indexed by index_of<Signum>().
    static inline castle::bitset<signal_count> enabled_{};

    // Publish/teardown gate. Set to true by install() with release
    // ordering after every prior setup step is complete; set to false by
    // uninstall() with release ordering before rewinding sigaction.
    // os_handler reads it with acquire ordering as its very first step.
    //
    // Requires castle::atomic<bool> to be lock-free — which C++ guarantees
    // via is_always_lock_free on all mainstream targets, and which is the
    // de-facto condition under which castle::atomic operations are
    // async-signal-safe. If a platform ever fails this static_assert, the
    // gate would have to be reimplemented in terms of
    // volatile std::sig_atomic_t + std::atomic_signal_fence, losing the
    // cross-thread acquire/release contract.
    static_assert(castle::atomic<bool>::is_always_lock_free,
                  "inplace_signal_event: castle::atomic<bool> must be lock-free "
                  "for async-signal-safe release/acquire gating");
    static inline castle::atomic<bool> ready_{false};

    // Compile-time list of managed signal numbers as raw ints — the form
    // consumed by <csignal> (sigaction, SIG_DFL, ...) and by the async-
    // signal handler's runtime lookup. Populated by projecting each
    // signal_ipc_config's signum through to_signum().
    static constexpr int signal_list_[signal_count] = {
        to_signum(config_traits<SignalConfigs>::signum)...
    };
};

// Out-of-class definition for the constexpr signal_list_ array — required
// pre-C++17 for ODR-use; harmless in C++17 where the in-class initializer
// is already implicitly inline. Kept explicit for maximum toolchain
// portability on older embedded compilers.
template <typename... SignalConfigs>
constexpr int inplace_signal_event<SignalConfigs...>::signal_list_[];

} // namespace events
} // namespace castle

#endif // CASTLE_EVENTS_INPLACE_SIGNAL_IPC_EVENT_H
