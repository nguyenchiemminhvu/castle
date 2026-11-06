// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file update_dispatcher.hpp
 * @brief Observer that invokes one callback per updated type, directly in the notifier's context.
 *
 * A dispatcher is the push-style counterpart of `update_inbox`. It is a
 * `castle::design_patterns::observer` of `update_set<Ts...>`, so it can be attached to a
 * `data_pool` or to any `castle::design_patterns::observable` that forwards update sets. It
 * records nothing: `notify()` calls, for every type of the incoming set, the callback bound to
 * that type. The callback knows its type from the binding and reads the latest value from the
 * pool itself, e.g. with `pool.get_snapshot<T>()`. Types without a callback are ignored.
 *
 * @code
 * struct imu {};
 * struct gnss {};
 *
 * castle_ext::data::update_dispatcher<imu, gnss> dispatcher;
 * dispatcher.bind<imu>([&pool]() noexcept
 * {
 *     castle_ext::data::snapshot<imu> sample = pool.get_snapshot<imu>();
 * });
 * pool.add_observer(&dispatcher);
 *
 * pool.publish(imu{});   // runs the imu callback in this thread, before publish() returns
 * @endcode
 */
#ifndef CASTLE_EXT_DATA_UPDATE_DISPATCHER_HPP
#define CASTLE_EXT_DATA_UPDATE_DISPATCHER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/design_patterns/observer.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/tuple.hpp"
#include "castle_ext/data/update_set.hpp"

namespace castle_ext
{
namespace data
{

/**
 * @brief Synchronous per-type callback table for `update_set<Ts...>` notifications.
 *
 * @tparam Ts Data types of the update set, in the same order as the producing pool.
 *
 * Every notification is delivered: there is no coalescing and no pending state. A callback
 * runs once per notification that contains its type, in declaration order of `Ts...` when one
 * set names several types.
 *
 * @note `notify()` runs the callbacks in the notifier's context (HAL thread, ISR-deferred task).
 *       Callbacks of different types may run concurrently when different threads publish them;
 *       with one writer per type, a given callback is never re-entered by the pool.
 * @note `bind()` is not synchronized: bind every callback before the dispatcher is attached.
 */
template <typename... Ts>
class update_dispatcher CASTLE_FINAL : public castle::design_patterns::observer<update_set<Ts...>>
{
public:
    using set_type = update_set<Ts...>;
    using callback_type = castle::callbacks::function<void()>;

    /** @brief Constructs a dispatcher with no callback bound; notifications are ignored. */
    update_dispatcher() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    update_dispatcher(CASTLE_CONST update_dispatcher&) CASTLE_DELETE;
    update_dispatcher& operator=(CASTLE_CONST update_dispatcher&) CASTLE_DELETE;

    /**
     * @brief Binds, replaces, or clears (`nullptr`) the callback of `T`.
     * @tparam T One of `Ts...`; cv-qualifiers are ignored.
     * @param callback Called by `notify()` whenever the incoming set contains `T`. It runs in
     *                 the notifier's context and must be short and thread-safe.
     * @note Not synchronized: call it before the dispatcher is attached to a pool or observable.
     */
    template <typename T>
    void bind(callback_type callback) CASTLE_NOEXCEPT
    {
        callbacks_[index_of<T>()] = CASTLE_MOVE(callback);
    }

    /** @brief Returns whether a callback is bound to `T`. */
    template <typename T>
    CASTLE_NODISCARD bool is_bound() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<bool>(callbacks_[index_of<T>()]);
    }

    /**
     * @brief Calls the bound callback of every type in `updates`.
     * @param updates Types that carry new data.
     * @note Complexity: O(sizeof...(Ts)) bit tests plus one call per bound type in `updates`.
     */
    void notify(CASTLE_CONST set_type& updates) CASTLE_NOEXCEPT CASTLE_OVERRIDE
    {
        updates.for_each([this](auto type) noexcept
        {
            CASTLE_CONST callback_type& callback = callbacks_[index_of<typename decltype(type)::type>()];
            if (callback)
            {
                callback();
            }
        });
    }

private:
    template <typename T>
    static CASTLE_CONSTEXPR castle::size_type index_of() CASTLE_NOEXCEPT
    {
        using value_type = castle::meta::remove_cv_t<T>;
        static_assert(castle::meta::pack_contains<value_type, Ts...>::value,
                      "type is not part of this update_dispatcher");
        return castle::meta::tuple_type_index<value_type, Ts...>::value;
    }

    callback_type callbacks_[sizeof...(Ts)]{};
};

} // namespace data
} // namespace castle_ext

#endif // CASTLE_EXT_DATA_UPDATE_DISPATCHER_HPP
