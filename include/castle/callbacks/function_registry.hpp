// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file function_registry.hpp
 * @brief Defines a fixed-capacity registry that owns callback wrappers in-place and returns generation-checked subscriptions.
 *
 * Use this header when a component needs a small callback list that stores stateful lambdas or other callables by value
 * without heap allocation. Each occupied slot owns one castle::callbacks::function object, so callers do not need to
 * keep the original callable alive after subscribe() succeeds.
 *
 * Key constraints:
 * - Fixed compile-time capacity and per-slot inline storage.
 * - No heap allocation.
 * - Only `void(Args...)` callback signatures are accepted.
 * - Subscription handles become stale after unsubscribe(), clear(), slot reuse, or registry destruction.
 * - No internal synchronization; concurrent access requires external coordination.
 *
 * @code
 * #include "castle/callbacks/function_registry.hpp"
 *
 * int main()
 * {
 *     castle::callbacks::function_registry<4U, void(int)> registry;
 *     int total = 0;
 *
 *     castle::callbacks::subscription sub =
 *         registry.subscribe([&total](int value) noexcept
 *         {
 *             total += value;
 *         });
 *
 *     registry(3);
 *     sub.unsubscribe();
 *     return total == 3 ? 0 : 1;
 * }
 * @endcode
 */
#ifndef CASTLE_CALLBACKS_FUNCTION_REGISTRY_HPP
#define CASTLE_CALLBACKS_FUNCTION_REGISTRY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/forward.hpp"
#include "castle/container/array.hpp"

#include "castle/callbacks/function.hpp"
#include "castle/callbacks/subscription.hpp"

#include <stdint.h>

namespace castle
{
namespace callbacks
{

/**
 * @brief Forward declaration for a fixed-capacity registry of in-place function wrappers.
 * @tparam max_callback Maximum number of simultaneously active callback slots.
 * @tparam signature Callback signature in the form `void(Args...)`.
 * @tparam callback_storage_size Inline storage reserved for each callback object.
 * @tparam callback_storage_alignment Alignment of each callback slot.
 * @note Only the `void(Args...)` specialization is defined.
 */
template <
    size_type max_callback,
    typename signature,
    size_type callback_storage_size = castle::inplace_storage_reserved,
    size_type callback_storage_alignment = castle::inplace_alignment_default>
class function_registry;

/**
 * @brief Stores up to @p max_callback callbacks by value and invokes them in slot order.
 * @tparam max_callback Maximum number of active subscriptions.
 * @tparam return_type Callback return type; must be `void`.
 * @tparam Args Callback argument types.
 * @tparam callback_storage_size Inline storage reserved inside each stored function wrapper.
 * @tparam callback_storage_alignment Alignment used by each stored function wrapper.
 *
 * subscribe() moves a callback into the first inactive slot and returns a subscription that can later remove that slot.
 * clear() resets every active callback wrapper and invalidates prior subscriptions by incrementing each cleared slot
 * generation.
 *
 * @note Complexity: subscribe(), invoke(), and clear() are O(capacity()).
 * @warning Outstanding subscription handles are not safe to use after registry destruction because they keep a
 * non-owning owner pointer.
 */
template <
    size_type max_callback,
    typename return_type,
    typename... Args,
    size_type callback_storage_size,
    size_type callback_storage_alignment>
class function_registry<max_callback, return_type(Args...), callback_storage_size, callback_storage_alignment> CASTLE_FINAL
    : public i_unsubscribable
{
    static_assert(meta::is_void<return_type>::value,
                  "function_registry requires void callback return type");

public:
    using callback_type = function<return_type(Args...), callback_storage_size, callback_storage_alignment>;

    using subscription = castle::callbacks::subscription;
    using error = castle::status;

    /**
     * @brief Constructs an empty registry.
     */
    function_registry() CASTLE_DEFAULT;

    /**
     * @brief Destroys the registry.
     *
     * @note Active callbacks are destroyed as part of the slot storage itself.
     * @warning Outstanding subscriptions become invalid externally and must not call unsubscribe() after destruction.
     */
    ~function_registry() override CASTLE_DEFAULT;

    /**
     * @brief Copy construction is disabled.
     * @note Copying would invalidate the owner identity stored inside subscriptions.
     */
    function_registry(CASTLE_CONST function_registry&) CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @note Copying would invalidate the owner identity stored inside subscriptions.
     */
    function_registry& operator=(CASTLE_CONST function_registry&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @note Moving would invalidate the owner identity stored inside subscriptions.
     */
    function_registry(function_registry&&) CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @note Moving would invalidate the owner identity stored inside subscriptions.
     */
    function_registry& operator=(function_registry&&) CASTLE_DELETE;

    /**
     * @brief Subscribes a ready-made function wrapper if capacity remains.
     * @param callback Function wrapper to move into registry storage.
     * @param[out] out_error Optional status output that receives status::ok, status::invalid_callback, or status::full.
     * @return A valid subscription on success; otherwise a default invalid subscription.
     * @note An empty callback wrapper is rejected with status::invalid_callback.
     */
    subscription subscribe(callback_type&& callback, error* out_error = nullptr)
    {
        if (!callback)
        {
            if (out_error != nullptr) // LCOV_EXCL_BR_LINE
            {
                *out_error = error::invalid_callback;
            }
            return subscription{};
        }

        for (size_type i = 0; i < max_callback; ++i)
        {
            slot& current_slot = slots_[i];

            if (!current_slot.active)
            {
                current_slot.callback = CASTLE_MOVE(callback);
                current_slot.active = true;
                ++active_count_;

                if (out_error != nullptr) // LCOV_EXCL_BR_LINE
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

        if (out_error != nullptr) // LCOV_EXCL_BR_LINE
        {
            *out_error = error::full;
        }
        return subscription{};
    }

    /**
     * @brief Subscribes any callable that can be wrapped by callback_type.
     * @tparam callback_t Callable type accepted by callback_type's constructor.
     * @param callback Callable object, lambda, or function pointer to store.
     * @param[out] out_error Optional status output that receives status::ok, status::invalid_callback, or status::full.
     * @return A valid subscription on success; otherwise a default invalid subscription.
     * @note This overload constructs a temporary callback_type and then forwards to subscribe(callback_type&&).
     */
    template <typename callback_t,
              typename = meta::enable_if_t<
                !meta::is_same<meta::decay_t<callback_t>, callback_type>::value
             && !meta::is_same<meta::decay_t<callback_t>, subscription>::value, void>>
    subscription subscribe(callback_t&& callback, error* out_error = nullptr)
    {
        callback_type callback_wrapper{CASTLE_FORWARD<callback_t>(callback)};
        return subscribe(CASTLE_MOVE(callback_wrapper), out_error);
    }

    /**
     * @brief Removes a slot through the type-erased subscription interface.
     * @param index Slot index to remove.
     * @param generation Expected slot generation captured by the subscription.
     * @return status::ok when the slot is active and generations match; otherwise status::invalid_subscription.
     */
    error unsubscribe_slot(size_type index, uint32_t generation) noexcept override
    {
        if (index >= max_callback)
        {
            return error::invalid_subscription;
        }

        slot& current_slot = slots_[index];

        if (!current_slot.active)
        {
            return error::invalid_subscription;
        }

        if (current_slot.generation != generation)
        {
            return error::invalid_subscription;
        }

        current_slot.callback = callback_type{};
        current_slot.active = false;

        ++current_slot.generation;
        --active_count_;

        return error::ok;
    }

    /**
     * @brief Invokes every active callback in increasing slot order.
     * @param args Arguments forwarded to each registered callback.
     * @note Invocation order is deterministic and follows slot order.
     */
    void invoke(Args... args)
    {
        for (size_type i = 0; i < max_callback; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.active && current_slot.callback) // LCOV_EXCL_BR_LINE
            {
                current_slot.callback(CASTLE_FORWARD<Args>(args)...);
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
     * @brief Removes every active callback and invalidates existing subscriptions.
     *
     * @note Each occupied slot is reset to an empty callback wrapper and its generation is incremented.
     */
    void clear() noexcept
    {
        for (size_type i = 0; i < max_callback; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.active)
            {
                current_slot.callback = callback_type{};
                current_slot.active = false;
                ++current_slot.generation;
            }
        }

        active_count_ = 0;
    }

    /**
     * @brief Returns the number of active callbacks.
     * @return Active subscription count.
     */
    CASTLE_CONSTEXPR size_type size() CASTLE_CONST noexcept
    {
        return active_count_;
    }

    /**
     * @brief Reports whether the registry contains no active callbacks.
     * @return True when size() is zero.
     */
    CASTLE_CONSTEXPR bool empty() CASTLE_CONST noexcept
    {
        return active_count_ == 0;
    }

    /**
     * @brief Returns the maximum number of simultaneously active callbacks.
     * @return Compile-time capacity of the registry.
     */
    static CASTLE_CONSTEXPR size_type capacity() noexcept
    {
        return max_callback;
    }

private:
    /**
     * @brief Stores one callback object together with its slot identity metadata.
     *
     * The generation increments whenever the slot is deactivated so old subscriptions cannot unsubscribe a reused slot.
     */
    struct slot
    {
        callback_type callback;
        uint32_t generation = 0;
        bool active = false;
    };

    container::array<slot, max_callback> slots_{};
    size_type active_count_ = 0;
};

} // namespace callbacks
} // namespace castle

#endif // CASTLE_CALLBACKS_FUNCTION_REGISTRY_HPP
