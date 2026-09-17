// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file event_dispatcher.hpp
 * @brief Fixed-capacity compile-time event dispatcher that owns callbacks by value.
 * @details Use this dispatcher when the event set is known at compile time and
 * each subscriber should be stored inside the dispatcher without heap
 * allocation. Every event tag has its own fixed callback registry sized by the
 * corresponding `event_tag_config`, and dispatch resolves tags at compile time
 * instead of performing runtime lookup. Registration order is preserved during
 * dispatch. The component does not synchronize access; callers must serialize
 * concurrent registration, clearing, and dispatch.
 *
 * @code
 * #include "castle/events/event_dispatcher.hpp"
 * #include "castle/events/event_tag_config.hpp"
 *
 * struct TickEvent {};
 *
 * using dispatcher_type = castle::events::event_dispatcher<
 *     castle::events::event_tag_config<TickEvent, 4U, void(uint32_t), 32U>>;
 *
 * dispatcher_type dispatcher;
 * auto sub = dispatcher.register_callback<TickEvent>([](uint32_t) {});
 * dispatcher.dispatch_event<TickEvent>(1U);
 * sub.unsubscribe();
 * @endcode
 */
#ifndef CASTLE_EVENTS_EVENT_DISPATCHER_HPP
#define CASTLE_EVENTS_EVENT_DISPATCHER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/tuple.hpp"

#include "castle/callbacks/function.hpp"
#include "castle/callbacks/function_registry.hpp"
#include "castle/events/event_tag_config.hpp"

#include <stdint.h>

namespace castle
{
namespace events
{

/**
 * @brief Tag-keyed event dispatcher with fixed per-event subscriber storage.
 * @tparam EventConfigs One or more `event_tag_config` entries that define the
 * event set, callback capacities, signatures, and inline callback storage.
 * @note Callbacks are stored by value in per-event `callbacks::function_registry` instances.
 * @warning This dispatcher is not internally synchronized.
 */
template <typename... EventConfigs>
class event_dispatcher
{
private:
    static_assert(sizeof...(EventConfigs) > 0,
                  "event_dispatcher requires at least one event_tag_config");

    template <typename T>
    struct is_valid_event_config : meta::false_type {};

    template <
        typename EventTag,
        size_type MaxCallback,
        typename... Args,
        size_type StorageSize,
        size_type StorageAlignment>
    struct is_valid_event_config<event_tag_config<EventTag, MaxCallback, void(Args...), StorageSize, StorageAlignment>>
        : meta::true_type {};

    static_assert(meta::conjunction<is_valid_event_config<EventConfigs>...>::value,
                  "event_dispatcher accepts only event_tag_config<...> template arguments "
                  "whose Signature is void(Args...)");

    template <typename Config>
    struct config_traits;

    template <
        typename EventTag,
        size_type MaxCallback,
        typename Signature,
        size_type StorageSize,
        size_type StorageAlignment>
    struct config_traits<event_tag_config<EventTag, MaxCallback, Signature, StorageSize, StorageAlignment>>
    {
        using event_tag = EventTag;
        using signature = Signature;

        static CASTLE_CONSTEXPR size_type max_callback = MaxCallback;
        static CASTLE_CONSTEXPR size_type storage_size = StorageSize;
        static CASTLE_CONSTEXPR size_type storage_alignment = StorageAlignment;

        static_assert(MaxCallback > 0,
                      "event_dispatcher: event_tag_config::MaxCallback must be > 0");
        static_assert(StorageSize > 0,
                      "event_dispatcher: event_tag_config::CallbackStorageSize must be > 0");
        static_assert(StorageAlignment > 0,
                      "event_dispatcher: event_tag_config::CallbackStorageAlignment must be > 0");
    };

    template <typename Target, typename... Rest>
    struct contains_tag;

    template <typename Target>
    struct contains_tag<Target> : meta::false_type {};

    template <typename Target, typename First, typename... Rest>
    struct contains_tag<Target, First, Rest...>
        : meta::integral_constant<bool,
              meta::is_same<typename config_traits<First>::event_tag, Target>::value
              || contains_tag<Target, Rest...>::value>
    {};

    template <typename... Configs>
    struct configs_are_unique;

    template <typename Head>
    struct configs_are_unique<Head> : meta::true_type {};

    template <typename Head, typename Next, typename... Tail>
    struct configs_are_unique<Head, Next, Tail...>
        : meta::integral_constant<bool,
              !contains_tag<typename config_traits<Head>::event_tag, Next, Tail...>::value
              && configs_are_unique<Next, Tail...>::value>
    {};

    static_assert(configs_are_unique<EventConfigs...>::value,
                  "event_dispatcher: the EventConfigs... pack must not contain duplicate event_tag types");

    template <typename Config>
    using registry_type_for = callbacks::function_registry<
        config_traits<Config>::max_callback,
        typename config_traits<Config>::signature,
        config_traits<Config>::storage_size,
        config_traits<Config>::storage_alignment>;

    using registry_tuple = castle::tuple<registry_type_for<EventConfigs>...>;

public:
    /**
     * @brief Status type used by registration and dispatch operations.
     */
    using error = castle::status;

    /**
     * @brief Subscription handle returned when a callback is registered.
     */
    using subscription = callbacks::subscription;

    /**
     * @brief Constructs an empty dispatcher with one fixed registry per event tag.
     */
    event_dispatcher()
    {
    }

    /**
     * @brief Destroys the dispatcher and any callbacks stored by value.
     */
    ~event_dispatcher() CASTLE_DEFAULT;

    /**
     * @brief Copy construction is disabled because subscriptions reference embedded registries.
     */
    event_dispatcher(CASTLE_CONST event_dispatcher&) CASTLE_DELETE;
    /**
     * @brief Copy assignment is disabled because subscriptions reference embedded registries.
     */
    event_dispatcher& operator=(CASTLE_CONST event_dispatcher&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled because subscriptions reference embedded registries.
     */
    event_dispatcher(event_dispatcher&&) CASTLE_DELETE;
    /**
     * @brief Move assignment is disabled because subscriptions reference embedded registries.
     */
    event_dispatcher& operator=(event_dispatcher&&) CASTLE_DELETE;

    /**
     * @brief Returns the number of event tags compiled into the dispatcher.
     * @return Size of the `EventConfigs...` pack.
     */
    static CASTLE_CONSTEXPR size_type event_capacity() CASTLE_NOEXCEPT
    {
        return sizeof...(EventConfigs);
    }

    /**
     * @brief Returns the subscriber capacity for one event tag.
     * @tparam Tag Event tag to query.
     * @return Maximum number of callbacks that may be registered for `Tag`.
     */
    template <typename Tag>
    static CASTLE_CONSTEXPR size_type callback_capacity() CASTLE_NOEXCEPT
    {
        return config_traits<config_for<Tag>>::max_callback;
    }

    /**
     * @brief Returns the inline callback storage size configured for one event tag.
     * @tparam Tag Event tag to query.
     * @return Inline callback storage size in bytes for `Tag`.
     */
    template <typename Tag>
    static CASTLE_CONSTEXPR size_type callback_storage_size() CASTLE_NOEXCEPT
    {
        return config_traits<config_for<Tag>>::storage_size;
    }

    /**
     * @brief Returns the inline callback storage alignment configured for one event tag.
     * @tparam Tag Event tag to query.
     * @return Inline callback storage alignment in bytes for `Tag`.
     */
    template <typename Tag>
    static CASTLE_CONSTEXPR size_type callback_storage_alignment() CASTLE_NOEXCEPT
    {
        return config_traits<config_for<Tag>>::storage_alignment;
    }

    /**
     * @brief Registers a callback for the event identified by `Tag`.
     * @tparam Tag Event tag to subscribe.
     * @tparam Callback Callable type converted into the configured in-place callback wrapper.
     * @param callback Callable to store by value.
     * @param out_error Optional output pointer that receives the registration status.
     * @return Valid subscription on success; invalid subscription on failure.
     * @note Subscription order determines dispatch order.
     * @warning The callable and any captures must fit the configured inline storage for `Tag`.
     */
    template <typename Tag, typename Callback>
    subscription register_callback(Callback&& callback, error* out_error = nullptr)
    {
        status inner_error = status::ok;

        subscription sub = registry<Tag>().subscribe(
            CASTLE_FORWARD<Callback>(callback),
            &inner_error);

        if (out_error != nullptr)
        {
            *out_error = inner_error;
        }

        return sub;
    }

    /**
     * @brief Dispatches one event to every subscriber registered for `Tag`.
     * @tparam Tag Event tag to dispatch.
     * @tparam CallArgs Argument types forwarded to the registered callbacks.
     * @param args Event payload arguments forwarded to each callback.
     * @return `error::ok` after the event has been dispatched.
     * @note Callbacks run synchronously in registration order on the calling thread.
     */
    template <typename Tag, typename... CallArgs>
    error dispatch_event(CallArgs&&... args)
    {
        registry<Tag>().invoke(CASTLE_FORWARD<CallArgs>(args)...);

        return error::ok;
    }

    /**
     * @brief Removes every callback registered for one event tag.
     * @tparam Tag Event tag whose registry should be cleared.
     * @note Outstanding subscriptions for `Tag` become stale after this call.
     */
    template <typename Tag>
    void clear_event() CASTLE_NOEXCEPT
    {
        registry<Tag>().clear();
    }

    /**
     * @brief Removes every callback from every configured event tag.
     * @note Outstanding subscriptions become stale after this call.
     */
    void clear() CASTLE_NOEXCEPT
    {
        clear_all_impl(castle::sequence::index_sequence_for<EventConfigs...>{});
    }

    /**
     * @brief Returns the number of currently registered callbacks for one event tag.
     * @tparam Tag Event tag to query.
     * @return Active subscriber count for `Tag`.
     */
    template <typename Tag>
    size_type subscriber_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return registry<Tag>().size();
    }

private:
    /**
     * @brief Computes the index of a target event tag within a list of event_tag_config types at compile time.
     * @tparam Target The event tag type whose index is being computed.
     * @tparam I Current index in the scan.
     * @tparam First The first event_tag_config type in the current scan.
     * @tparam Rest The remaining event_tag_config types in the current scan.
     */
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

    template <typename Tag>
    using config_for = castle::tuple_element_t<index_of<Tag>(), castle::tuple<EventConfigs...>>;

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

    template <size_type... Is>
    void clear_all_impl(castle::sequence::index_sequence<Is...>) CASTLE_NOEXCEPT
    {
        int dummy[] = {
            (castle::get<Is>(registries_).clear(), 0)...
        };
        (void)dummy;
    }

private:
    registry_tuple registries_{};
};

} 
} 

#endif 
