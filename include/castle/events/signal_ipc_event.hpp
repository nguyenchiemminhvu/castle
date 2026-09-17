// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Compile-time configured POSIX signal dispatcher with inline-owned callbacks.
 *
 * Use this component when a process must react to a fixed set of POSIX signals
 * through callbacks stored entirely inside Castle-owned static storage. Each
 * managed signal has its own compile-time capacity and callback storage budget,
 * no Castle heap allocation occurs, and installed callbacks execute
 * immediately from the OS signal handler.
 *
 * @warning User callbacks run in signal context and therefore must be
 * async-signal-safe.
 * @warning Registration, clear, and enable/disable operations are not
 * synchronized against concurrent delivery after install(). Configure the
 * dispatcher before install() or provide external single-writer coordination.
 *
 * @code
 * #include "castle/events/signal_ipc_event.hpp"
 *
 * int main()
 * {
 *     using namespace castle::events;
 *     using ipc = signal_ipc_event<
 *         signal_ipc_config<signal::sigusr1, 1U>
 *     >;
 *
 *     volatile sig_atomic_t fired = 0;
 *     auto sub = ipc::register_callback<signal::sigusr1>([&fired]() { fired = 1; });
 *
 *     ipc::install();
 *     ::raise(SIGUSR1);
 *     ipc::uninstall();
 *     sub.unsubscribe();
 *
 *     return fired == 1 ? 0 : 1;
 * }
 * @endcode
 */
#ifndef CASTLE_EVENTS_SIGNAL_IPC_EVENT_HPP
#define CASTLE_EVENTS_SIGNAL_IPC_EVENT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/tuple.hpp"
#include "castle/utility/bitset.hpp"
#include "castle/atomic/atomic.hpp"

#include "castle/callbacks/function.hpp"
#include "castle/callbacks/function_registry.hpp"
#include "castle/events/signal_ipc_config.hpp"

#include <signal.h>

namespace castle
{
namespace events
{

/**
 * @brief Static, signal-number-keyed callback dispatcher.
 *
 * @tparam SignalConfigs One signal_ipc_config<...> per managed POSIX signal.
 *
 * @note The type is namespace-shaped: all storage and APIs are static, and the
 * class cannot be instantiated.
 */
template <typename... SignalConfigs>
class signal_ipc_event
{
private:
    static_assert(sizeof...(SignalConfigs) > 0,
                  "signal_ipc_event requires at least one signal_ipc_config");

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
                  "signal_ipc_event accepts only signal_ipc_config<...> template arguments");

    template <typename Config>
    struct config_traits;

    template <
        signal Signal,
        size_type MaxCallback,
        size_type StorageSize,
        size_type StorageAlignment>
    struct config_traits<signal_ipc_config<Signal, MaxCallback, StorageSize, StorageAlignment>>
    {
        static CASTLE_CONSTEXPR signal signum = Signal;
        static CASTLE_CONSTEXPR size_type max_callback = MaxCallback;
        static CASTLE_CONSTEXPR size_type storage_size = StorageSize;
        static CASTLE_CONSTEXPR size_type storage_alignment = StorageAlignment;

        static_assert(MaxCallback > 0,
                      "signal_ipc_event: signal_ipc_config::MaxCallback must be > 0");
        static_assert(StorageSize > 0,
                      "signal_ipc_event: signal_ipc_config::StorageSize must be > 0");
        static_assert(StorageAlignment > 0,
                      "signal_ipc_event: signal_ipc_config::StorageAlignment must be > 0");
    };

    template <signal Target, typename... Rest>
    struct contains_signal;

    template <signal Target>
    struct contains_signal<Target> : meta::false_type {};

    template <signal Target, typename First, typename... Rest>
    struct contains_signal<Target, First, Rest...>
        : meta::integral_constant<bool,
              (config_traits<First>::signum == Target)
              || contains_signal<Target, Rest...>::value>
    {};

    template <typename... Configs>
    struct configs_are_unique;

    template <typename Head>
    struct configs_are_unique<Head> : meta::true_type {};

    template <typename Head, typename Next, typename... Tail>
    struct configs_are_unique<Head, Next, Tail...>
        : meta::integral_constant<bool,
              !contains_signal<config_traits<Head>::signum, Next, Tail...>::value
              && configs_are_unique<Next, Tail...>::value>
    {};

    static_assert(configs_are_unique<SignalConfigs...>::value,
                  "signal_ipc_event: the SignalConfigs... pack must not contain duplicate signum values");

    template <typename Config>
    using registry_type_for = callbacks::function_registry<
        config_traits<Config>::max_callback,
        void(),
        config_traits<Config>::storage_size,
        config_traits<Config>::storage_alignment>;

    using registry_tuple = castle::tuple<registry_type_for<SignalConfigs>...>;

public:
    /** @brief Error code type used by registration and installation APIs. */
    using error = castle::status;

    /** @brief Subscription handle returned by register_callback(). */
    using subscription = callbacks::subscription;

    /** @brief Number of managed signals in this instantiation. */
    static CASTLE_CONSTEXPR size_type signal_count = sizeof...(SignalConfigs);

    /** @brief Instantiation is disabled because the type exposes only static storage and static APIs. */
    signal_ipc_event() CASTLE_DELETE;
    ~signal_ipc_event() CASTLE_DELETE;
    signal_ipc_event(CASTLE_CONST signal_ipc_event&) CASTLE_DELETE;
    signal_ipc_event& operator=(CASTLE_CONST signal_ipc_event&) CASTLE_DELETE;
    signal_ipc_event(signal_ipc_event&&) CASTLE_DELETE;
    signal_ipc_event& operator=(signal_ipc_event&&) CASTLE_DELETE;

    /**
     * @brief Query how many distinct signals this type manages.
     *
     * @return signal_count.
     */
    static CASTLE_CONSTEXPR size_type signal_capacity() noexcept
    {
        return signal_count;
    }

    /**
     * @brief Query the callback capacity for one configured signal.
     *
     * @tparam Signum Signal to inspect.
     * @return Maximum number of callbacks that register_callback<Signum>() can
     * hold simultaneously.
     */
    template <signal Signum>
    static CASTLE_CONSTEXPR size_type callback_capacity() noexcept
    {
        return config_traits<config_for<Signum>>::max_callback;
    }

    /**
     * @brief Query the inline callback storage size for one configured signal.
     *
     * @tparam Signum Signal to inspect.
     * @return Inline storage size used by that signal's function_registry.
     */
    template <signal Signum>
    static CASTLE_CONSTEXPR size_type callback_storage_size() noexcept
    {
        return config_traits<config_for<Signum>>::storage_size;
    }

    /**
     * @brief Query the inline callback storage alignment for one configured signal.
     *
     * @tparam Signum Signal to inspect.
     * @return Inline storage alignment used by that signal's function_registry.
     */
    template <signal Signum>
    static CASTLE_CONSTEXPR size_type callback_storage_alignment() noexcept
    {
        return config_traits<config_for<Signum>>::storage_alignment;
    }

    /**
     * @brief Install the dispatcher as the POSIX handler for every managed signal.
     *
     * @return error::ok on success, or error::system_call_error when a
     * sigaction() call fails.
     *
     * @note install() enables every configured signal before arming sigaction().
     * @note Registrations sequenced before install() are published to the
     * handler through ready_'s release/acquire handshake.
     */
    static error install() noexcept
    {
        ready_.store(false, castle::memory_order_release);

        enabled_.set();

        struct sigaction sa{};
        sa.sa_handler = &signal_ipc_event::os_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        for (size_type i = 0; i < signal_count; ++i)
        {
            // LCOV_EXCL_START
            if (::sigaction(signal_list_[i], &sa, nullptr) != 0)
            {
                uninstall();
                return error::system_call_error;
            }
            // LCOV_EXCL_STOP
        }

        ready_.store(true, castle::memory_order_release);
        return error::ok;
    }

    /**
     * @brief Restore SIG_DFL for every managed signal.
     *
     * @note Registered callbacks remain stored and become active again after a
     * later install() unless clear_signal() or clear() is called.
     */
    static void uninstall() noexcept
    {
        ready_.store(false, castle::memory_order_release);

        struct sigaction sa{};
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        for (size_type i = 0; i < signal_count; ++i)
        {
            (void)::sigaction(signal_list_[i], &sa, nullptr);
        }
    }

    /**
     * @brief Report whether install() has published the handler as ready.
     *
     * @return true when ready_ is set, otherwise false.
     */
    static bool is_installed() noexcept
    {
        return ready_.load(castle::memory_order_acquire);
    }

    /**
     * @brief Register a callback for one configured signal.
     *
     * @tparam Signum Signal to subscribe to.
     * @tparam Callable Callable type stored by value in the signal's registry.
     * @param callback Callable to subscribe.
     * @param out_error Optional output for the registry result.
     * @return A subscription handle. The handle is invalid when the registry is
     * full or the callback is invalid.
     *
     * @warning The callback body itself must be async-signal-safe because it
     * executes directly from os_handler().
     */
    template <signal Signum, typename Callable>
    static subscription register_callback(Callable&& callback, error* out_error = nullptr)
    {
        CASTLE_CONSTEXPR size_type idx = index_of<Signum>();
        status inner_error = status::ok;

        subscription sub = castle::get<idx>(registries_).subscribe(
            CASTLE_FORWARD<Callable>(callback), &inner_error);

        if (out_error != nullptr)
        {
            *out_error = inner_error;
        }

        return sub;
    }

    /**
     * @brief Enable callback invocation for one configured signal.
     *
     * @tparam Signum Signal to enable.
     *
     * @note This only flips the internal enabled_ bit. The process still owns
     * the installed POSIX handler while the dispatcher is installed.
     */
    template <signal Signum>
    static void enable_signal() noexcept
    {
        enabled_.set(index_of<Signum>());
    }

    /**
     * @brief Disable callback invocation for one configured signal.
     *
     * @tparam Signum Signal to disable.
     *
     * @note The POSIX handler still receives the signal and returns early.
     */
    template <signal Signum>
    static void disable_signal() noexcept
    {
        enabled_.reset(index_of<Signum>());
    }

    /**
     * @brief Report whether one configured signal is enabled.
     *
     * @tparam Signum Signal to query.
     * @return true when callback dispatch is enabled for Signum.
     */
    template <signal Signum>
    static bool is_signal_enabled() noexcept
    {
        return enabled_.test(index_of<Signum>());
    }

    /**
     * @brief Remove every callback registered for one signal.
     *
     * @tparam Signum Signal whose registry should be cleared.
     *
     * @note Outstanding subscription handles for cleared slots become stale.
     */
    template <signal Signum>
    static void clear_signal() noexcept
    {
        castle::get<index_of<Signum>()>(registries_).clear();
    }

    /**
     * @brief Remove every callback registered for every configured signal.
     */
    static void clear() noexcept
    {
        clear_all_impl(castle::sequence::index_sequence_for<SignalConfigs...>{});
    }

    /**
     * @brief Count active subscribers for one signal.
     *
     * @tparam Signum Signal to inspect.
     * @return Number of active callbacks currently stored for Signum.
     */
    template <signal Signum>
    static size_type subscriber_count() noexcept
    {
        return castle::get<index_of<Signum>()>(registries_).size();
    }

private:
    /**
     * @brief Recursively scan the SignalConfigs pack to find the index of a target signal.
     * @tparam Target Signal to locate.
     * @tparam I Current index in the scan.
     * @tparam First First signal configuration in the current scan.
     * @tparam Rest Remaining signal configurations in the current scan.
     * @return Index of the target signal within the SignalConfigs pack.
     */
    template <signal Target, size_type I, typename First, typename... Rest>
    static CASTLE_CONSTEXPR size_type index_of_scan() noexcept
    {
        if CASTLE_CONSTEXPR (config_traits<First>::signum == Target)
        {
            return I;
        }
        else if CASTLE_CONSTEXPR (sizeof...(Rest) > 0)
        {
            return index_of_scan<Target, I + 1, Rest...>();
        }
        else
        {
            static_assert(config_traits<First>::signum == Target,
                          "signal_ipc_event: signal is not present in the SignalConfigs pack");
            return 0;
        }
    }

    template <signal Signum>
    static CASTLE_CONSTEXPR size_type index_of() noexcept
    {
        return index_of_scan<Signum, 0, SignalConfigs...>();
    }

    template <signal Signum>
    using config_for = castle::tuple_element_t<index_of<Signum>(), castle::tuple<SignalConfigs...>>;

    /**
     * @brief Find the index of a signal at runtime based on its native POSIX signal number.
     * @param signum Native POSIX signal number to locate.
     * @return Index of the signal within the SignalConfigs pack, or signal_count if not found.
     */
    static size_type index_of_runtime(int signum) noexcept
    {
        // LCOV_EXCL_START
        for (size_type i = 0; i < signal_count; ++i)
        {
            if (signal_list_[i] == signum)
            {
                return i;
            }
        }
        return signal_count;
        // LCOV_EXCL_STOP
    }

    /**
     * @brief Recursively invoke the callback for the signal at the specified index.
     * @param target_index Index of the signal to invoke the callback for.
     * @note This function is intended for internal use by the signal dispatcher and should not be called directly.
     */
    template <size_type Index>
    static void invoke_impl(size_type target_index) noexcept
    {
        // LCOV_EXCL_START
        if CASTLE_CONSTEXPR (Index < signal_count)
        {
            if (Index == target_index)
            {
                castle::get<Index>(registries_).invoke();
                return;
            }

            invoke_impl<Index + 1>(target_index);
        }
        // LCOV_EXCL_STOP
    } // LCOV_EXCL_LINE

    /**
     * @brief Recursively clear all signal registries.
     * @note This function is intended for internal use by the signal dispatcher and should not be called directly.
     */
    template <size_type... Is>
    static void clear_all_impl(castle::sequence::index_sequence<Is...>) noexcept
    {
        int dummy[] = {
            (castle::get<Is>(registries_).clear(), 0)...
        };
        (void)dummy;
    }

    /**
     * @brief Operating system signal handler for the managed signals.
     * @param signum Native POSIX signal number received by the handler.
     * @note This function is intended for internal use by the signal dispatcher and should not be called directly.
     */
    static void os_handler(int signum) noexcept
    {
        // LCOV_EXCL_START
        // Acquire pairs with install() so signal delivery never observes
        // partially published registry state.
        if (!ready_.load(castle::memory_order_acquire))
        {
            return;
        }

        CASTLE_CONST size_type idx = index_of_runtime(signum);
        if (idx >= signal_count)
        {
            return;
        }
        // LCOV_EXCL_STOP

        if (!enabled_.test(idx))
        {
            return;
        }

        invoke_impl<0>(idx);
    }

    static inline registry_tuple registries_{};
    static inline castle::bitset<signal_count> enabled_{};

    static_assert(castle::atomic<bool>::is_always_lock_free,
                  "signal_ipc_event: castle::atomic<bool> must be lock-free "
                  "for async-signal-safe release/acquire gating");
    static inline castle::atomic<bool> ready_{false};

    static CASTLE_CONSTEXPR int signal_list_[signal_count] = {
        to_signum(config_traits<SignalConfigs>::signum)...
    };
};

template <typename... SignalConfigs>
CASTLE_CONSTEXPR int signal_ipc_event<SignalConfigs...>::signal_list_[];

} // namespace events
} // namespace castle

#endif // CASTLE_EVENTS_SIGNAL_IPC_EVENT_HPP
