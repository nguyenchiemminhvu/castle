// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file delegate_registry.hpp
 * @brief Defines a fixed-capacity registry of non-owning delegate pointers with generation-checked subscriptions.
 *
 * Use this header when callbacks already exist as delegate objects elsewhere and a component only needs to subscribe,
 * invoke, and later remove them without taking ownership. The registry stores raw pointers to delegate_base instances,
 * never allocates, and detects stale subscription handles with per-slot generation counters.
 *
 * Key constraints:
 * - Fixed compile-time capacity.
 * - No heap allocation.
 * - Callbacks are borrowed, not owned; each registered delegate object must outlive its active registration.
 * - Subscription handles become stale after unsubscribe(), clear(), slot reuse, or registry destruction.
 * - No internal synchronization; concurrent access requires external coordination.
 *
 * @code
 * #include "castle/callbacks/delegate_registry.hpp"
 * #include "castle/callbacks/delegate.hpp"
 *
 * void on_event(int value) noexcept
 * {
 *     (void)value;
 * }
 *
 * int main()
 * {
 *     castle::callbacks::delegate_ptr<void(int)> callback{&on_event};
 *     castle::callbacks::delegate_registry<4U, void(int)> registry;
 *
 *     castle::status error = castle::status::ok;
 *     castle::callbacks::subscription sub = registry.subscribe(&callback, &error);
 *
 *     if (sub && error == castle::status::ok)
 *     {
 *         registry.invoke(42);
 *         sub.unsubscribe();
 *     }
 *
 *     return 0;
 * }
 * @endcode
 */
#ifndef CASTLE_CALLBACKS_DELEGATE_REGISTRY_HPP
#define CASTLE_CALLBACKS_DELEGATE_REGISTRY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/forward.hpp"

#include "castle/callbacks/delegate.hpp"
#include "castle/callbacks/subscription.hpp"

#include "castle/container/array.hpp"

#include <stdint.h>

namespace castle
{
namespace callbacks
{

/**
 * @brief Forward declaration for a fixed-capacity registry of borrowed delegates.
 * @tparam max_callback Maximum number of simultaneously active callback slots.
 * @tparam signature Callback signature in the form `void(Args...)`.
 * @note Only the `void(Args...)` specialization is defined.
 */
template <
    size_type max_callback,
    typename signature>
class delegate_registry;

/**
 * @brief Stores up to @p max_callback delegate pointers and invokes them in slot order.
 * @tparam max_callback Maximum number of active subscriptions.
 * @tparam return_type Callback return type; must be `void`.
 * @tparam Args Callback argument types.
 *
 * The registry is non-owning: it stores raw pointers to delegate_base objects supplied by the caller. subscribe()
 * scans for the first empty slot, invoke() walks every slot from index 0 to `max_callback - 1`, and clear() invalidates
 * outstanding subscriptions by bumping each cleared slot generation.
 *
 * @note Complexity: subscribe(), invoke(), and clear() are O(capacity()).
 * @warning A registered callback object must remain alive until it is unsubscribed, cleared, or the registry is
 * destroyed.
 * @warning Destroying the registry does not notify outstanding subscription handles; calling unsubscribe() on such a
 * handle afterward is invalid because the stored owner pointer dangles.
 */
template <
    size_type max_callback,
    typename return_type,
    typename... Args>
class delegate_registry<max_callback, return_type(Args...)> CASTLE_FINAL
    : public i_unsubscribable
{
    static_assert(meta::is_void<return_type>::value,
                  "delegate_registry requires void callback return type");

public:
    using callback_type = delegate_base<return_type(Args...)>;
    using subscription = castle::callbacks::subscription;
    using error = castle::status;

    /**
     * @brief Constructs an empty registry.
     *
     * @note All slots start inactive with generation zero.
     */
    delegate_registry() CASTLE_DEFAULT;

    /**
     * @brief Destroys the registry.
     *
     * @note Registered callbacks are not destroyed because the registry does not own them.
     * @warning Outstanding subscription handles are not invalidated in-place and must not be used after destruction.
     */
    ~delegate_registry() override CASTLE_DEFAULT;

    /**
     * @brief Copy construction is disabled.
     * @note Copying would invalidate the ownership identity stored in existing subscriptions.
     */
    delegate_registry(CASTLE_CONST delegate_registry&) CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @note Copying would invalidate the ownership identity stored in existing subscriptions.
     */
    delegate_registry& operator=(CASTLE_CONST delegate_registry&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @note Moving would invalidate the owner pointer stored in existing subscriptions.
     */
    delegate_registry(delegate_registry&&) CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @note Moving would invalidate the owner pointer stored in existing subscriptions.
     */
    delegate_registry& operator=(delegate_registry&&) CASTLE_DELETE;

    /**
     * @brief Subscribes a delegate pointer if capacity remains.
     * @param callback Non-owning pointer to a delegate object compatible with `void(Args...)`.
     * @param[out] out_error Optional status output that receives status::ok, status::invalid_callback, or status::full.
     * @return A valid subscription on success; otherwise a default invalid subscription.
     * @note The returned subscription captures the slot generation so stale handles are rejected after slot reuse.
     * @warning Passing nullptr fails with status::invalid_callback.
     * @warning The pointed-to delegate object must outlive the active registration.
     */
    subscription subscribe(callback_type* callback, error* out_error = nullptr) CASTLE_NOEXCEPT
    {
        if (callback == nullptr)
        {
            if (out_error != nullptr)
            {
                *out_error = error::invalid_callback;
            }
            return subscription{};
        }

        for (size_type i = 0; i < max_callback; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.callback == nullptr)
            {
                current_slot.callback = callback;
                ++active_count_;

                if (out_error != nullptr)
                {
                    *out_error = error::ok;
                }

                return subscription{
                    this,
                    i,
                    current_slot.generation
                };
            }
        }

        if (out_error != nullptr)
        {
            *out_error = error::full;
        }
        return subscription{};
    }

    /**
     * @brief Removes a slot through the type-erased subscription interface.
     * @param index Slot index to remove.
     * @param generation Expected slot generation captured by the subscription.
     * @return status::ok when the slot is active and generations match; otherwise status::invalid_subscription.
     * @note Successful removal clears the callback pointer, increments the slot generation, and decrements size().
     */
    error unsubscribe_slot(size_type index, uint32_t generation) CASTLE_NOEXCEPT override
    {
        if (index >= max_callback)
        {
            return error::invalid_subscription;
        }

        slot& current_slot = slots_[index];

        if (current_slot.callback == nullptr)
        {
            return error::invalid_subscription;
        }

        if (current_slot.generation != generation)
        {
            return error::invalid_subscription;
        }

        current_slot.callback = nullptr;
        ++current_slot.generation;
        --active_count_;

        return error::ok;
    }

    /**
     * @brief Invokes every active callback in increasing slot order.
     * @param args Arguments forwarded to each registered callback.
     * @note Invocation order is deterministic and matches successful subscription order until slots are reused.
     * @warning No synchronization is performed. Mutating the registry during concurrent invocation requires external
     * coordination.
     */
    void invoke(Args... args)
    {
        for (size_type i = 0; i < max_callback; ++i)
        {
            callback_type* callback = slots_[i].callback;

            if (callback != nullptr)
            {
                (*callback)(CASTLE_FORWARD<Args>(args)...);
            }
        }
    }

    /**
     * @brief Invokes every active callback in increasing slot order.
     * @param args Arguments forwarded to each registered callback.
     * @note Equivalent to invoke().
     */
    void operator()(Args... args)
    {
        this->invoke(CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Removes all active callbacks and invalidates their subscriptions.
     *
     * @note Each occupied slot has its generation incremented so stale subscription handles are rejected afterward.
     * @warning The callback objects themselves are untouched because ownership remains with the caller.
     */
    void clear() CASTLE_NOEXCEPT
    {
        for (size_type i = 0; i < max_callback; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.callback != nullptr)
            {
                current_slot.callback = nullptr;
                ++current_slot.generation;
            }
        }

        active_count_ = 0;
    }

    /**
     * @brief Returns the number of active callbacks.
     * @return Active subscription count.
     */
    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return active_count_;
    }

    /**
     * @brief Reports whether the registry contains no active callbacks.
     * @return True when size() is zero.
     */
    CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return active_count_ == 0;
    }

    /**
     * @brief Returns the maximum number of simultaneously active callbacks.
     * @return Compile-time capacity of the registry.
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return max_callback;
    }

private:
    /**
     * @brief Stores one callback pointer plus the generation used to validate subscriptions.
     *
     * The generation increments each time the slot is deactivated so an older subscription cannot unsubscribe a reused
     * slot.
     */
    struct slot
    {
        callback_type* callback = nullptr;
        uint32_t generation = 0;
    };

    container::array<slot, max_callback> slots_{};
    size_type active_count_ = 0;
};

} // namespace callbacks
} // namespace castle

#endif // CASTLE_CALLBACKS_DELEGATE_REGISTRY_HPP
