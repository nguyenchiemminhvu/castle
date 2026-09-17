#pragma once

#include "castle/callbacks/inplace_function.h"
#include "castle/callbacks/inplace_callback_registry.h"
#include "castle/events/event_config.h"
#include "castle/events/event_subscription.h"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

using castle::callbacks::inplace_function;
using castle::callbacks::inplace_callback_registry;
using castle::callbacks::callback_subscription_error;
using castle::callbacks::callback_subscription;

namespace castle
{
namespace events
{

// -----------------------------------------------------------------------------
// inplace_event_dispatcher - compile-time, tag-keyed event dispatcher with
// value-owned callbacks.
//
// Configuration model:
//   The event set is fixed at compile time via a pack of event_config<...>.
//   inplace_event_dispatcher consumes every field on event_config:
//   event_tag, max_callback, signature, callback_storage_size and
//   callback_storage_alignment. Using the same event_config type for both
//   flavours keeps a single, uniform configuration surface — the user can
//   swap event_dispatcher <-> inplace_event_dispatcher just by changing the
//   outer class name, without touching the pack.
//
// Design:
//   - Each event is identified by a TAG TYPE (event_config::event_tag). The
//     tag acts as a compile-time key — analogous to an integer key in a hash
//     table, but resolved at compile time to a tuple index (O(1), no hashing,
//     no virtual dispatch, no placement new for the event slots themselves).
//   - Each event owns its own inplace_callback_registry<max_callback,
//     signature, callback_storage_size, callback_storage_alignment>.
//     The registry stores each subscribed callable BY VALUE in a fixed
//     inline buffer, so no heap allocation happens even for stateful
//     lambdas.
//
// Storage:
//   - A std::tuple of per-tag inplace_callback_registry instances holds the
//     whole subscriber state — sized independently per event, no runtime
//     allocation.
//   - A std::bitset<sizeof...(EventConfigs)> tracks per-event enable state.
//
// Subscriptions:
//   - register_callback<Tag>(callable) returns an
//     callback_subscription carrying a back-pointer to the correct
//     registry, so callers can self-unsubscribe:
//         sub.unsubscribe();
//
// Usage:
//   struct TimerExpired   {};   // pure tag, no payload
//   struct VehicleStarted {};   // pure tag, no payload
//   struct Shutdown       {};   // pure tag, no payload
//
//   using dispatcher_t = inplace_event_dispatcher<
//       event_config<TimerExpired,   8, void(std::uint32_t, std::uint32_t)>,
//       event_config<VehicleStarted, 4, void(std::uint8_t)>,
//       event_config<Shutdown,       2, void(), 32>
//   >;
//
//   dispatcher_t dispatcher;
//   auto sub = dispatcher.register_callback<TimerExpired>(
//       [](std::uint32_t id, std::uint32_t ts) { /* ... */ });
//   dispatcher.dispatch_event<TimerExpired>(42u, 1000u);
//   sub.unsubscribe();
// -----------------------------------------------------------------------------
template <typename... EventConfigs>
class inplace_event_dispatcher
{
private:
    // -------------------------------------------------------------------------
    // Basic pack sanity.
    // -------------------------------------------------------------------------
    static_assert(sizeof...(EventConfigs) > 0,
                  "inplace_event_dispatcher requires at least one event_config");

    // -------------------------------------------------------------------------
    // Reject anything that is not an event_config<...> whose Signature is a
    // function type of the form void(Args...). The partial specialisation
    // matches only that shape, so both "not an event_config", "Signature is
    // not a function type", and "Signature returns non-void" all fall
    // through to the primary false_type — a single, sharp diagnostic at the
    // dispatcher's front door rather than a deeper failure inside
    // inplace_callback_registry's void-return static_assert.
    // -------------------------------------------------------------------------
    template <typename T>
    struct is_valid_event_config : std::false_type {};

    template <
        typename EventTag,
        std::size_t MaxCallback,
        typename... Args,
        std::size_t StorageSize,
        std::size_t StorageAlignment>
    struct is_valid_event_config<event_config<EventTag, MaxCallback, void(Args...), StorageSize, StorageAlignment>>
        : std::true_type {};

    static_assert(std::conjunction<is_valid_event_config<EventConfigs>...>::value,
                  "inplace_event_dispatcher accepts only event_config<...> template arguments "
                  "whose Signature is void(Args...)");

    // -------------------------------------------------------------------------
    // Extract per-config traits. inplace_event_dispatcher uses every field:
    // event_tag, max_callback, signature, storage_size, storage_alignment.
    // -------------------------------------------------------------------------
    template <typename Config>
    struct config_traits;

    template <
        typename EventTag,
        std::size_t MaxCallback,
        typename Signature,
        std::size_t StorageSize,
        std::size_t StorageAlignment>
    struct config_traits<event_config<EventTag, MaxCallback, Signature, StorageSize, StorageAlignment>>
    {
        using event_tag = EventTag;
        using signature = Signature;

        static constexpr std::size_t max_callback = MaxCallback;
        static constexpr std::size_t storage_size = StorageSize;
        static constexpr std::size_t storage_alignment = StorageAlignment;

        static_assert(MaxCallback > 0,
                      "inplace_event_dispatcher: event_config::MaxCallback must be > 0");
        static_assert(StorageSize > 0,
                      "inplace_event_dispatcher: event_config::CallbackStorageSize must be > 0");
        static_assert(StorageAlignment > 0,
                      "inplace_event_dispatcher: event_config::CallbackStorageAlignment must be > 0");
    };

    // -------------------------------------------------------------------------
    // Duplicate-tag detection.
    //
    // Two event_config entries with the same event_tag would create two
    // independent registries for the same event, only the first of which is
    // reachable via compile-time lookup — almost certainly a bug: enable /
    // disable / clear operations would silently miss the shadowed slot.
    // -------------------------------------------------------------------------
    template <typename Target, typename... Rest>
    struct contains_tag;

    // Empty-pack base case (partial specialisation: Target still open).
    template <typename Target>
    struct contains_tag<Target> : std::false_type {};

    // Recursive case.
    template <typename Target, typename First, typename... Rest>
    struct contains_tag<Target, First, Rest...>
        : std::integral_constant<bool,
              std::is_same<typename config_traits<First>::event_tag, Target>::value
              || contains_tag<Target, Rest...>::value>
    {};

    template <typename... Configs>
    struct configs_are_unique;

    // Single-element base case — trivially unique. Doubles as the recursion
    // terminator because the EventConfigs... pack is guaranteed non-empty by
    // the static_assert above.
    template <typename Head>
    struct configs_are_unique<Head> : std::true_type {};

    // Recursive case: Head unique against Tail, plus Tail unique amongst
    // itself.
    template <typename Head, typename Next, typename... Tail>
    struct configs_are_unique<Head, Next, Tail...>
        : std::integral_constant<bool,
              !contains_tag<typename config_traits<Head>::event_tag, Next, Tail...>::value
              && configs_are_unique<Next, Tail...>::value>
    {};

    static_assert(configs_are_unique<EventConfigs...>::value,
                  "inplace_event_dispatcher: the EventConfigs... pack must not contain duplicate event_tag types");

    // -------------------------------------------------------------------------
    // Per-event registry type — one owning inplace_callback_registry sized
    // by that config's max_callback / signature / storage_size /
    // storage_alignment.
    // -------------------------------------------------------------------------
    template <typename Config>
    using registry_type_for = inplace_callback_registry<
        config_traits<Config>::max_callback,
        typename config_traits<Config>::signature,
        config_traits<Config>::storage_size,
        config_traits<Config>::storage_alignment>;

    using registry_tuple = std::tuple<registry_type_for<EventConfigs>...>;

public:
    using error = event_subscription_error;
    using subscription = callback_subscription;

    inplace_event_dispatcher()
    {
        // All events start enabled. bitset default-initialises to 0.
        enabled_.set();
    }

    ~inplace_event_dispatcher() = default;

    // Non-copyable, non-movable. Dispatcher identity is tied to the addresses
    // of the registries embedded in its tuple — outstanding subscriptions
    // reference those addresses via i_unsubscribable*.
    inplace_event_dispatcher(const inplace_event_dispatcher&) = delete;
    inplace_event_dispatcher& operator=(const inplace_event_dispatcher&) = delete;

    inplace_event_dispatcher(inplace_event_dispatcher&&) = delete;
    inplace_event_dispatcher& operator=(inplace_event_dispatcher&&) = delete;

    // -------------------------------------------------------------------------
    // Compile-time capacity queries.
    // -------------------------------------------------------------------------
    static constexpr std::size_t event_capacity() noexcept
    {
        return sizeof...(EventConfigs);
    }

    template <typename Tag>
    static constexpr std::size_t callback_capacity() noexcept
    {
        return config_traits<config_for<Tag>>::max_callback;
    }

    template <typename Tag>
    static constexpr std::size_t callback_storage_size() noexcept
    {
        return config_traits<config_for<Tag>>::storage_size;
    }

    template <typename Tag>
    static constexpr std::size_t callback_storage_alignment() noexcept
    {
        return config_traits<config_for<Tag>>::storage_alignment;
    }

    // -------------------------------------------------------------------------
    // Register a callback for the event identified by Tag.
    //
    // Accepts any callable convertible to the inplace_function signature
    // declared by event_config<Tag, N, Signature, ...>. The callback is
    // stored by value inside the registry's inplace_function buffer, so
    // stateful lambdas / captures are supported (subject to the per-tag
    // callback_storage_size / callback_storage_alignment).
    //
    // Returns a subscription handle. On failure the handle is !valid() and
    // out_error (if provided) is set.
    // -------------------------------------------------------------------------
    template <typename Tag, typename Callback>
    subscription register_callback(Callback&& callback, error* out_error = nullptr)
    {
        callback_subscription_error inner_error =
            callback_subscription_error::ok;

        subscription sub = registry<Tag>().subscribe(
            std::forward<Callback>(callback),
            &inner_error);

        if (out_error != nullptr)
        {
            *out_error = convert_error(inner_error);
        }

        return sub;
    }

    // -------------------------------------------------------------------------
    // Dispatch an event identified by Tag with the payload declared by
    // Signature in event_config<Tag, N, Signature, ...>.
    //
    // Perfect-forwarded to inplace_callback_registry::invoke(Args...).
    // Returns error::event_disabled if the event tag is currently disabled.
    // (An unknown Tag fails to compile — no runtime "not found".)
    // -------------------------------------------------------------------------
    template <typename Tag, typename... CallArgs>
    error dispatch_event(CallArgs&&... args)
    {
        constexpr std::size_t idx = index_of<Tag>();

        if (!enabled_.test(idx))
        {
            return error::event_disabled;
        }

        registry<Tag>().invoke(std::forward<CallArgs>(args)...);

        return error::ok;
    }

    // -------------------------------------------------------------------------
    // Enable / disable / query an event tag. Disabled events accept
    // subscribes but dispatch_event() returns event_disabled without
    // invoking any callbacks.
    // -------------------------------------------------------------------------
    template <typename Tag>
    void enable_event() noexcept
    {
        enabled_.set(index_of<Tag>());
    }

    template <typename Tag>
    void disable_event() noexcept
    {
        enabled_.reset(index_of<Tag>());
    }

    template <typename Tag>
    bool is_event_enabled() const noexcept
    {
        return enabled_.test(index_of<Tag>());
    }

    // -------------------------------------------------------------------------
    // Clear all subscriptions for a specific event tag. Outstanding
    // subscription handles for that tag become stale. Cleared callback
    // objects are destroyed (the registry owns them).
    // -------------------------------------------------------------------------
    template <typename Tag>
    void clear_event() noexcept
    {
        registry<Tag>().clear();
    }

    // -------------------------------------------------------------------------
    // Clear all subscriptions across every event tag.
    // -------------------------------------------------------------------------
    void clear() noexcept
    {
        clear_all_impl(std::index_sequence_for<EventConfigs...>{});
    }

    template <typename Tag>
    std::size_t subscriber_count() const noexcept
    {
        return registry<Tag>().size();
    }

private:
    // -------------------------------------------------------------------------
    // Compile-time tag -> tuple index inside the EventConfigs... pack.
    //
    // Implemented as a constexpr function template with `if constexpr` so
    // only the branch corresponding to the actual match state is
    // instantiated — no runaway recursion after the first hit, and the
    // "unknown tag" diagnostic surfaces exactly once, at the moment
    // index_of<Tag>() is used.
    // -------------------------------------------------------------------------
    template <typename Target, std::size_t I, typename First, typename... Rest>
    static constexpr std::size_t index_of_scan() noexcept
    {
        if constexpr (std::is_same<typename config_traits<First>::event_tag, Target>::value)
        {
            return I;
        }
        else if constexpr (sizeof...(Rest) > 0)
        {
            return index_of_scan<Target, I + 1, Rest...>();
        }
        else
        {
            static_assert(std::is_same<typename config_traits<First>::event_tag, Target>::value,
                          "inplace_event_dispatcher: Tag is not present in the EventConfigs pack");
            return 0;
        }
    }

    template <typename Tag>
    static constexpr std::size_t index_of() noexcept
    {
        return index_of_scan<Tag, 0, EventConfigs...>();
    }

    // event_config bound to a specific tag — resolves via the tuple.
    template <typename Tag>
    using config_for = std::tuple_element_t<index_of<Tag>(), std::tuple<EventConfigs...>>;

    // Direct access to the inplace_callback_registry for a specific tag.
    template <typename Tag>
    registry_type_for<config_for<Tag>>& registry() noexcept
    {
        return std::get<index_of<Tag>()>(registries_);
    }

    template <typename Tag>
    const registry_type_for<config_for<Tag>>& registry() const noexcept
    {
        return std::get<index_of<Tag>()>(registries_);
    }

    // Sequence a clear() call across every tuple element without fold
    // expressions in the body (index_sequence-driven expansion is C++14
    // friendly).
    template <std::size_t... Is>
    void clear_all_impl(std::index_sequence<Is...>) noexcept
    {
        int dummy[] = {
            (std::get<Is>(registries_).clear(), 0)...
        };
        (void)dummy;
    }

    // Map callback_subscription_error to event_subscription_error.
    static constexpr error convert_error(
        callback_subscription_error error_code) noexcept
    {
        switch (error_code)
        {
            case callback_subscription_error::ok:
            {
                return error::ok;
            }
            case callback_subscription_error::full:
            {
                return error::full;
            }
            case callback_subscription_error::invalid_callback:
            {
                return error::invalid_callback;
            }
            case callback_subscription_error::invalid_subscription:
            {
                return error::invalid_subscription;
            }
        }
        return error::invalid_subscription;
    }

private:
    // Per-event callback registries. Each registry is sized independently by
    // its event_config and OWNS its callbacks by value via inplace_function's
    // inline buffer — zero heap.
    registry_tuple registries_{};

    // Per-event enable/disable flags, indexed by index_of<Tag>().
    std::bitset<sizeof...(EventConfigs)> enabled_{};
};

} // namespace events
} // namespace castle
