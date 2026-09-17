#ifndef CASTLE_EVENTS_EVENT_DISPATCHER_H
#define CASTLE_EVENTS_EVENT_DISPATCHER_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/error/status.h"
#include "castle/utility/forward.h"
#include "castle/utility/tuple.h"

#include "castle/callbacks/function.h"
#include "castle/callbacks/callback_registry.h"
#include "castle/events/event_config.h"

#include <stdint.h>

namespace castle
{
namespace events
{

// -----------------------------------------------------------------------------
// event_dispatcher - compile-time, tag-keyed event dispatcher (non-owning).
//
// Configuration model:
//   The event set is fixed at compile time via a pack of event_config<...>.
//   event_dispatcher only consumes event_tag, max_callback and signature from
//   each config; callback_storage_size / callback_storage_alignment are
//   inplace_event_dispatcher's concern and are ignored here. Using the same
//   event_config type for both flavours keeps a single, uniform configuration
//   surface.
//
// Design:
//   - Each event is identified by a TAG TYPE (event_config::event_tag). The
//     tag acts as a compile-time key — analogous to an integer key in a hash
//     table, but resolved at compile time to a tuple index (O(1), no hashing,
//     no virtual dispatch, no placement new).
//   - Each event owns its own callbacks::callback_registry<max_callback, signature>.
//     Callbacks are non-owning callbacks::i_function<Args...>* pointers, so any variant
//     from function.h (function, function_m, function_ct, function_ct_m,
//     function_ct_im, function_f, function_fr, function_ct_f) can subscribe.
//     Zero heap allocation, deterministic.
//
// Storage:
//   - A castle::tuple of per-tag callbacks::callback_registry instances holds the whole
//     subscriber state — sized independently per event, no runtime allocation.
//
// Subscriptions:
//   - register_callback<Tag>(&cb) returns a callbacks::callback_subscription
//     that carries a back-pointer to the correct registry, so callers can
//     self-unsubscribe: sub.unsubscribe();
//
// Usage:
//   struct TimerExpired   {};   // pure tag, no payload
//   struct VehicleStarted {};   // pure tag, no payload
//
//   using dispatcher_t = event_dispatcher<
//       event_config<TimerExpired,   8, void(uint32_t, uint32_t)>,
//       event_config<VehicleStarted, 4, void(uint8_t)>
//   >;
//
//   dispatcher_t dispatcher;
//
//   void on_timer(uint32_t id, uint32_t ts) { ... }
//   castle::callbacks::function<void(uint32_t, uint32_t)> cb(&on_timer);
//
//   auto sub = dispatcher.register_callback<TimerExpired>(&cb);
//   dispatcher.dispatch_event<TimerExpired>(42u, 1000u);
//   sub.unsubscribe();
// -----------------------------------------------------------------------------
template <typename... EventConfigs>
class event_dispatcher
{
private:
    // -------------------------------------------------------------------------
    // Basic pack sanity.
    // -------------------------------------------------------------------------
    static_assert(sizeof...(EventConfigs) > 0,
                  "event_dispatcher requires at least one event_config");

    // -------------------------------------------------------------------------
    // Reject anything that is not an event_config<...> whose Signature is a
    // function type of the form void(Args...). The partial specialisation
    // matches only that shape, so both "not an event_config", "Signature is
    // not a function type", and "Signature returns non-void" all fall
    // through to the primary false_type — a single, sharp diagnostic at the
    // dispatcher's front door rather than a deeper failure inside
    // callbacks::callback_registry's void-return static_assert.
    // -------------------------------------------------------------------------
    template <typename T>
    struct is_valid_event_config : meta::false_type {};

    template <
        typename EventTag,
        size_type MaxCallback,
        typename... Args,
        size_type StorageSize,
        size_type StorageAlignment>
    struct is_valid_event_config<event_config<EventTag, MaxCallback, void(Args...), StorageSize, StorageAlignment>>
        : meta::true_type {};

    static_assert(
        meta::conjunction<is_valid_event_config<EventConfigs>...>::value,
        "event_dispatcher accepts only event_config<...> template arguments whose Signature is void(Args...)"
    );

    // -------------------------------------------------------------------------
    // Extract per-config traits. event_dispatcher only looks at event_tag,
    // max_callback and signature — storage_size / storage_alignment are
    // inplace-only fields.
    // -------------------------------------------------------------------------
    template <typename Config>
    struct config_traits;

    template <
        typename EventTag,
        size_type MaxCallback,
        typename Signature,
        size_type StorageSize,
        size_type StorageAlignment>
    struct config_traits<event_config<EventTag, MaxCallback, Signature, StorageSize, StorageAlignment>>
    {
        using event_tag = EventTag;
        using signature = Signature;

        static CASTLE_CONSTEXPR size_type max_callback = MaxCallback;

        static_assert(MaxCallback > 0,
                      "event_dispatcher: event_config::MaxCallback must be > 0");
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
    struct contains_tag<Target> : meta::false_type {};

    // Recursive case.
    template <typename Target, typename First, typename... Rest>
    struct contains_tag<Target, First, Rest...>
        : meta::integral_constant<bool,
              meta::is_same<typename config_traits<First>::event_tag, Target>::value
              || contains_tag<Target, Rest...>::value>
    {};

    template <typename... Configs>
    struct configs_are_unique;

    // Single-element base case — trivially unique. Doubles as the recursion
    // terminator because the EventConfigs... pack is guaranteed non-empty by
    // the static_assert above.
    template <typename Head>
    struct configs_are_unique<Head> : meta::true_type {};

    // Recursive case: Head unique against Tail, plus Tail unique amongst
    // itself.
    template <typename Head, typename Next, typename... Tail>
    struct configs_are_unique<Head, Next, Tail...>
        : meta::integral_constant<bool,
              !contains_tag<typename config_traits<Head>::event_tag, Next, Tail...>::value
              && configs_are_unique<Next, Tail...>::value>
    {};

    static_assert(configs_are_unique<EventConfigs...>::value,
                  "event_dispatcher: the EventConfigs... pack must not contain duplicate event_tag types");

    // -------------------------------------------------------------------------
    // Per-event registry type — one non-owning callbacks::callback_registry sized by
    // that config's max_callback and signature.
    // -------------------------------------------------------------------------
    template <typename Config>
    using registry_type_for = callbacks::callback_registry<
        config_traits<Config>::max_callback,
        typename config_traits<Config>::signature>;

    using registry_tuple = castle::tuple<registry_type_for<EventConfigs>...>;

public:
    using error = castle::status;
    using subscription = callbacks::callback_subscription;

    event_dispatcher()
    {
    }

    ~event_dispatcher() CASTLE_DEFAULT;

    // Non-copyable, non-movable. Dispatcher identity is tied to the addresses
    // of the registries embedded in its tuple — outstanding subscriptions
    // reference those addresses via i_unsubscribable*.
    event_dispatcher(CASTLE_CONST event_dispatcher&) CASTLE_DELETE;
    event_dispatcher& operator=(CASTLE_CONST event_dispatcher&) CASTLE_DELETE;

    event_dispatcher(event_dispatcher&&) CASTLE_DELETE;
    event_dispatcher& operator=(event_dispatcher&&) CASTLE_DELETE;

    // -------------------------------------------------------------------------
    // Compile-time capacity queries.
    // -------------------------------------------------------------------------
    static CASTLE_CONSTEXPR size_type event_capacity() CASTLE_NOEXCEPT
    {
        return sizeof...(EventConfigs);
    }

    template <typename Tag>
    static CASTLE_CONSTEXPR size_type callback_capacity() CASTLE_NOEXCEPT
    {
        return config_traits<config_for<Tag>>::max_callback;
    }

    // -------------------------------------------------------------------------
    // Register a non-owning callback for the event identified by Tag.
    //
    // The callback signature MUST match the Signature declared in
    // event_config<Tag, N, Signature>. This is enforced by the type of the
    // callback pointer: registry_type::callback_type == callbacks::i_function<Signature>.
    //
    // The caller owns the lifetime of the callbacks::i_function object and must keep it
    // alive until the returned subscription is unsubscribed (or the
    // dispatcher is destroyed).
    // -------------------------------------------------------------------------
    template <typename Tag, typename CallbackPtr>
    subscription register_callback(CallbackPtr callback, error* out_error = nullptr) CASTLE_NOEXCEPT
    {
        // Delegate signature checking to the registry: CallbackPtr must be
        // convertible to the exact callbacks::i_function<Signature>* expected by Tag.
        status inner_error = status::ok;

        subscription sub = registry<Tag>().subscribe(callback, &inner_error);

        if (out_error != nullptr)
        {
            *out_error = inner_error;
        }

        return sub;
    }

    // -------------------------------------------------------------------------
    // Dispatch an event identified by Tag with the payload declared by
    // Signature in event_config<Tag, N, Signature>.
    //
    // Perfect-forwarded to callbacks::callback_registry::invoke(Args...).
    // Returns error::event_disabled if the event tag is currently disabled.
    // (An unknown Tag fails to compile — no runtime "not found".)
    // -------------------------------------------------------------------------
    template <typename Tag, typename... CallArgs>
    error dispatch_event(CallArgs&&... args)
    {
        registry<Tag>().invoke(CASTLE_FORWARD<CallArgs>(args)...);

        return error::ok;
    }

    // -------------------------------------------------------------------------
    // Clear all subscriptions for a specific event tag. Outstanding
    // subscription handles for that tag become stale.
    // -------------------------------------------------------------------------
    template <typename Tag>
    void clear_event() CASTLE_NOEXCEPT
    {
        registry<Tag>().clear();
    }

    // -------------------------------------------------------------------------
    // Clear all subscriptions across every event tag.
    // -------------------------------------------------------------------------
    void clear() CASTLE_NOEXCEPT
    {
        clear_all_impl(castle::sequence::index_sequence_for<EventConfigs...>{});
    }

    template <typename Tag>
    size_type subscriber_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return registry<Tag>().size();
    }

private:
    // -------------------------------------------------------------------------
    // Compile-time tag -> tuple index inside the EventConfigs... pack.
    //
    // Implemented as a CASTLE_CONSTEXPR function template with `CASTLE_IF_CONSTEXPR` so
    // only the branch corresponding to the actual match state is
    // instantiated — no runaway recursion after the first hit, and the
    // "unknown tag" diagnostic surfaces exactly once, at the moment
    // index_of<Tag>() is used.
    // -------------------------------------------------------------------------
    template <typename Target, size_type I, typename First, typename... Rest>
    static CASTLE_CONSTEXPR size_type index_of_scan() CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (meta::is_same<typename config_traits<First>::event_tag, Target>::value)
        {
            return I;
        }
        else CASTLE_IF_CONSTEXPR (sizeof...(Rest) > 0)
        {
            return index_of_scan<Target, I + 1, Rest...>();
        }
        else
        {
            static_assert(meta::is_same<typename config_traits<First>::event_tag, Target>::value,
                          "event_dispatcher: Tag is not present in the EventConfigs pack");
            return 0;
        }
    }

    template <typename Tag>
    static CASTLE_CONSTEXPR size_type index_of() CASTLE_NOEXCEPT
    {
        return index_of_scan<Tag, 0, EventConfigs...>();
    }

    // event_config bound to a specific tag — resolves via the tuple.
    template <typename Tag>
    using config_for = castle::tuple_element_t<index_of<Tag>(), castle::tuple<EventConfigs...>>;

    // Direct access to the callbacks::callback_registry for a specific tag.
    template <typename Tag>
    registry_type_for<config_for<Tag>>& registry() CASTLE_NOEXCEPT
    {
        return castle::get<index_of<Tag>()>(registries_);
    }

    template <typename Tag>
    CASTLE_CONST registry_type_for<config_for<Tag>>& registry() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::get<index_of<Tag>()>(registries_);
    }

    // Sequence a clear() call across every tuple element without fold
    // expressions in the body (index_sequence-driven expansion is C++14
    // friendly).
    template <size_type... Is>
    void clear_all_impl(castle::sequence::index_sequence<Is...>) CASTLE_NOEXCEPT
    {
        int dummy[] = {
            (castle::get<Is>(registries_).clear(), 0)...
        };
        (void)dummy;
    }

private:
    // Per-event callback registries. Each registry is sized independently by
    // its event_config::max_callback and typed by its signature — this is the
    // whole subscriber storage for the dispatcher.
    registry_tuple registries_{};
};

} // namespace events
} // namespace castle

#endif // CASTLE_EVENTS_EVENT_DISPATCHER_H
