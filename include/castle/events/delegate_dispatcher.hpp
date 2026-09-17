// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file delegate_dispatcher.hpp
 * @brief Fixed-capacity compile-time event dispatcher for non-owning delegates.
 * @details Use this dispatcher when callback objects already exist elsewhere and
 * should be referenced without copying into dispatcher-owned storage. Each
 * `event_tag_config` produces one fixed-capacity `callbacks::delegate_registry`,
 * dispatch resolves event tags at compile time, and callback invocation follows
 * subscription order. The component performs no heap allocation and no internal
 * synchronization; callers must keep delegate objects alive and serialize
 * concurrent mutation or dispatch.
 *
 * @code
 * #include "castle/events/delegate_dispatcher.hpp"
 * #include "castle/events/event_tag_config.hpp"
 * #include "castle/callbacks/delegate.hpp"
 *
 * struct TickEvent {};
 *
 * using dispatcher_type = castle::events::delegate_dispatcher<
 *     castle::events::event_tag_config<TickEvent, 4U, void(uint32_t)>>;
 *
 * void on_tick(uint32_t) {}
 * castle::callbacks::delegate_ptr<void(uint32_t)> callback(&on_tick);
 * dispatcher_type dispatcher;
 * auto sub = dispatcher.register_callback<TickEvent>(&callback);
 * dispatcher.dispatch_event<TickEvent>(1U);
 * @endcode
 */
#ifndef CASTLE_EVENTS_DELEGATE_DISPATCHER_HPP
#define CASTLE_EVENTS_DELEGATE_DISPATCHER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/tuple.hpp"

#include "castle/callbacks/delegate.hpp"
#include "castle/callbacks/delegate_registry.hpp"
#include "castle/events/event_tag_config.hpp"

#include <stdint.h>

namespace castle
{
namespace events
{

/**
 * @brief Tag-keyed event dispatcher that stores non-owning delegate pointers.
 * @tparam EventConfigs One or more `event_tag_config` entries that define the
 * event set, callback capacities, and callback signatures.
 * @note Registered callbacks are borrowed, not owned.
 * @warning The referenced delegate objects must outlive their subscriptions.
 */
template <typename... EventConfigs>
class delegate_dispatcher
{
private:
    static_assert(sizeof...(EventConfigs) > 0,
                  "delegate_dispatcher requires at least one event_tag_config");

    /**
     * @brief Checks if a type is a valid event_tag_config for the dispatcher.
     * @tparam T The type to check.
     * @value `true` if `T` is a valid event_tag_config, `false` otherwise.
     */
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

    static_assert(
        meta::conjunction<is_valid_event_config<EventConfigs>...>::value,
        "delegate_dispatcher accepts only event_tag_config<...> template arguments whose Signature is void(Args...)"
    );

    /**
     * @brief Extracts traits from an event_tag_config type.
     * @tparam Config The event_tag_config type to extract traits from.
     * @note This struct provides type aliases and static constants for the event_tag_config.
     */
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

        static_assert(MaxCallback > 0,
                      "delegate_dispatcher: event_tag_config::MaxCallback must be > 0");
    };

    /**
     * @brief Checks if a type is contained within a list of event_tag_config types.
     * @tparam Target The type to check for presence in the list.
     * @value `true` if `Target` is present in the list of event_tag_config types, `false` otherwise.
     */
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

    /**
     * @brief Checks if a list of event_tag_config types contains only unique event tags.
     * @tparam Configs The list of event_tag_config types to check for uniqueness.
     */
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
                  "delegate_dispatcher: the EventConfigs... pack must not contain duplicate event_tag types");

    template <typename Config>
    using registry_type_for = callbacks::delegate_registry<
        config_traits<Config>::max_callback,
        typename config_traits<Config>::signature>;

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
    delegate_dispatcher()
    {
    }

    /**
     * @brief Destroys the dispatcher without destroying any referenced callback objects.
     */
    ~delegate_dispatcher() CASTLE_DEFAULT;

    /**
     * @brief Copy construction is disabled because subscriptions reference embedded registries.
     */
    delegate_dispatcher(CASTLE_CONST delegate_dispatcher&) CASTLE_DELETE;
    /**
     * @brief Copy assignment is disabled because subscriptions reference embedded registries.
     */
    delegate_dispatcher& operator=(CASTLE_CONST delegate_dispatcher&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled because subscriptions reference embedded registries.
     */
    delegate_dispatcher(delegate_dispatcher&&) CASTLE_DELETE;
    /**
     * @brief Move assignment is disabled because subscriptions reference embedded registries.
     */
    delegate_dispatcher& operator=(delegate_dispatcher&&) CASTLE_DELETE;

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
     * @brief Registers a non-owning delegate pointer for the event identified by `Tag`.
     * @tparam Tag Event tag to subscribe.
     * @tparam CallbackPtr Pointer type convertible to the configured delegate base pointer.
     * @param callback Delegate pointer to store.
     * @param out_error Optional output pointer that receives the registration status.
     * @return Valid subscription on success; invalid subscription on failure.
     * @note Subscription order determines dispatch order.
     * @warning The pointed-to delegate object must remain alive until it is unsubscribed or cleared.
     */
    template <typename Tag, typename CallbackPtr>
    subscription register_callback(CallbackPtr callback, error* out_error = nullptr) CASTLE_NOEXCEPT
    {
        status inner_error = status::ok;
        subscription sub = registry<Tag>().subscribe(callback, &inner_error);

        if (out_error != nullptr)
        {
            *out_error = inner_error;
        }

        return sub;
    }

    /**
     * @brief Dispatches one event to every subscriber registered for `Tag`.
     * @tparam Tag Event tag to dispatch.
     * @tparam CallArgs Argument types forwarded to the registered delegates.
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
     * @tparam I The current index in the scan.
     * @tparam First The first event_tag_config type in the current scan.
     * @tparam Rest The remaining event_tag_config types in the current scan.
     * @return The index of the target event tag within the list of event_tag_config types.
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
                          "delegate_dispatcher: Tag is not present in the EventConfigs pack");
            return 0;
        }
    }

    /**
     * @brief Computes the index of a target event tag within the list of event_tag_config types at compile time.
     * @tparam Tag The event tag type whose index is being computed.
     * @return The index of the target event tag within the list of event_tag_config types.
     */
    template <typename Tag>
    static CASTLE_CONSTEXPR size_type index_of() CASTLE_NOEXCEPT
    {
        return index_of_scan<Tag, 0, EventConfigs...>();
    }

    /// @brief Retrieves the configuration type associated with a specific event tag.
    template <typename Tag>
    using config_for = castle::tuple_element_t<index_of<Tag>(), castle::tuple<EventConfigs...>>;
    
    /**
     * @brief Retrieves the registry associated with a specific event tag.
     * @tparam Tag The event tag type whose associated registry is being retrieved.
     * @return A reference to the registry associated with the specified event tag.
     */
    template <typename Tag>
    registry_type_for<config_for<Tag>>& registry() CASTLE_NOEXCEPT
    {
        return castle::get<index_of<Tag>()>(registries_);
    }

    /**
     * @brief Retrieves the registry associated with a specific event tag in a const context.
     * @tparam Tag The event tag type whose associated registry is being retrieved.
     * @return A const reference to the registry associated with the specified event tag.
     */
    template <typename Tag>
    CASTLE_CONST registry_type_for<config_for<Tag>>& registry() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::get<index_of<Tag>()>(registries_);
    }

    /**
     * @brief Clears all registries by invoking their clear method.
     * @tparam Is The indices of the registries to be cleared.
     */
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
