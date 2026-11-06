// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file data_pool.hpp
 * @brief Shared HAL-to-application data store: latest value per type plus "which types changed".
 *
 * The pool is the single source of truth between producers (HAL drivers) and consumers (service,
 * application adapters). Its type set is fixed at compile time, so storage is static, lookups are
 * resolved by the compiler, and using an unknown type does not compile.
 *
 * - `publish(value)` stores the value in that type's `data_slot` and notifies the pool observers
 *   with an `update_set` naming only that type.
 * - `get_snapshot<T>()` returns a zero-copy `snapshot<T>` of the latest value.
 *
 * Typical flow: HAL publishes; an `update_inbox` registered as observer collects the changed
 * types and wakes the service thread; the service forwards the update set to the application
 * inboxes (`castle::design_patterns::observable`); each application reads only the types that
 * changed. Large structures are copied once, by the producer, never per consumer. When a
 * consumer must react in the publisher's context instead, register an `update_dispatcher`,
 * which calls one callback per changed type.
 *
 * @code
 * struct imu { float accel[3]{}; };
 * struct gnss_fix { double lat = 0.0; double lon = 0.0; };
 *
 * // gnss_fix snapshots may be held by up to 2 readers while newer fixes arrive.
 * using pool_type = castle_ext::data::data_pool<2U, imu, castle_ext::data::data_slot<gnss_fix, 2U>>;
 *
 * pool_type pool;
 * pool_type::inbox_type service_inbox;
 * pool.add_observer(&service_inbox);
 *
 * pool.publish(imu{{0.0F, 0.0F, 9.81F}});              // HAL thread
 *
 * pool_type::update_set_type changed = service_inbox.take();   // service thread
 * if (changed.contains<imu>())
 * {
 *     castle_ext::data::snapshot<imu> sample = pool.get_snapshot<imu>();
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_DATA_DATA_POOL_HPP
#define CASTLE_EXT_DATA_DATA_POOL_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/design_patterns/observer.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/tuple.hpp"
#include "castle_ext/data/data_slot.hpp"
#include "castle_ext/data/update_dispatcher.hpp"
#include "castle_ext/data/update_inbox.hpp"
#include "castle_ext/data/update_set.hpp"

namespace castle_ext
{
namespace data
{

namespace detail
{

/** @brief Maps a pool entry to its slot: `T` becomes `data_slot<T>`, `data_slot<T, N>` is kept. */
template <typename Entry>
struct pool_entry
{
    using slot_type = data_slot<Entry>;
};

template <typename T, castle::size_type MaxReaders>
struct pool_entry<data_slot<T, MaxReaders>>
{
    using slot_type = data_slot<T, MaxReaders>;
};

template <typename Entry>
using pool_slot_t = typename pool_entry<Entry>::slot_type;

template <typename Entry>
using pool_value_t = typename pool_slot_t<Entry>::value_type;

} // namespace detail

/**
 * @brief Fixed set of latest-value slots with change notification.
 *
 * @tparam MaxObservers Maximum number of observers notified by `publish()`.
 * @tparam Entries Data types, each given as `T` (one held snapshot, see `data_slot`) or as
 *         `data_slot<T, MaxReaders>` to size the snapshot buffers of that type. Value types must
 *         be unique.
 *
 * @note Register observers before producers start publishing; the observer list itself is not
 *       synchronized. Each type has one writer at a time; any number of threads may read.
 *       Readers that must not write can be handed a `const data_pool&`.
 */
template <castle::size_type MaxObservers, typename... Entries>
class data_pool CASTLE_FINAL
{
    static_assert(MaxObservers > 0U, "data_pool requires at least one observer slot");
    static_assert(castle::meta::has_unique_types<detail::pool_value_t<Entries>...>::value,
                  "data_pool value types must be unique");

public:
    /** @brief Notification payload: the set of changed types. */
    using update_set_type = update_set<detail::pool_value_t<Entries>...>;
    /** @brief Observer interface notified by `publish()`. */
    using observer_type = castle::design_patterns::observer<update_set_type>;
    /** @brief Ready-made thread-safe observer that collects updates for one consumer thread. */
    using inbox_type = update_inbox<detail::pool_value_t<Entries>...>;
    /** @brief Ready-made observer that calls one callback per changed type, in the publisher's context. */
    using dispatcher_type = update_dispatcher<detail::pool_value_t<Entries>...>;

    /** @brief Constructs every slot with a default-constructed value (generation 0). */
    data_pool() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    data_pool(CASTLE_CONST data_pool&) CASTLE_DELETE;
    data_pool& operator=(CASTLE_CONST data_pool&) CASTLE_DELETE;

    /**
     * @brief Registers an observer notified after every successful `publish()`.
     * @return `false` for `nullptr`, a duplicate, or when `MaxObservers` are registered.
     * @note The observer runs in the publisher's context and must be short and thread-safe;
     *       `inbox_type` meets both requirements, `dispatcher_type` passes them on to its callbacks.
     */
    bool add_observer(observer_type* observer) CASTLE_NOEXCEPT
    {
        return observers_.add_observer(observer);
    }

    /** @brief Unregisters an observer; returns `false` when it was not registered. */
    bool remove_observer(observer_type* observer) CASTLE_NOEXCEPT
    {
        return observers_.remove_observer(observer);
    }

    /**
     * @brief Stores `value` as the latest `T` and notifies observers with `{T}`.
     * @tparam T Deduced value type; must be one of the pool's types.
     * @return Result of `data_slot::publish()`. Observers are notified only on `status::ok`.
     */
    template <typename T>
    castle::status publish(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST castle::status result = slot<T>().publish(value);
        if (result == castle::status::ok)
        {
            observers_.notify_observers(update_set_type::template of<T>());
        }
        return result;
    }

    /**
     * @brief Returns a zero-copy snapshot of the latest `T`.
     * @tparam T One of the pool's value types.
     * @note Wait-free. Before the first publication the snapshot holds a default `T` with
     *       generation 0.
     */
    template <typename T>
    CASTLE_NODISCARD snapshot<T> get_snapshot() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return slot<T>().get_snapshot();
    }

private:
    template <typename T>
    static CASTLE_CONSTEXPR castle::size_type index_of() CASTLE_NOEXCEPT
    {
        static_assert(castle::meta::pack_contains<T, detail::pool_value_t<Entries>...>::value,
                      "type is not part of this data_pool");
        return castle::meta::tuple_type_index<T, detail::pool_value_t<Entries>...>::value;
    }

    template <typename T>
    auto& slot() CASTLE_NOEXCEPT
    {
        return castle::get<index_of<T>()>(slots_);
    }

    template <typename T>
    CASTLE_CONST auto& slot() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::get<index_of<T>()>(slots_);
    }

    castle::tuple<detail::pool_slot_t<Entries>...> slots_{};
    castle::design_patterns::observable<observer_type, MaxObservers> observers_{};
};

} // namespace data
} // namespace castle_ext

#endif // CASTLE_EXT_DATA_DATA_POOL_HPP
