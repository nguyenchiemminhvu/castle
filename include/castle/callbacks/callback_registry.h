#pragma once

#include "castle/callbacks/function.h"
#include "castle/callbacks/callback_subscription.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace castle
{
namespace callbacks
{

// -----------------------------------------------------------------------------
// Fixed-capacity callback registry (non-owning).
// Stores up to max_callback pointers to i_function<Args...> instances.
// The caller owns the lifetime of each callback object; the registry only
// borrows non-owning pointers. This mirrors the design philosophy of
// function.h — callback variants (function, function_m, function_ct,
// function_ct_m, function_ct_im) are user-defined objects with potentially
// zero storage cost, and forcing ownership here would defeat that goal.
//
// Invocation order = registration order (first-registered fires first).
//
// Template parameters:
//   max_callback - maximum number of concurrent subscriptions
//   signature    - callback signature, e.g. void(int, float)
//
// Usage:
//   void handler(int v) { /* ... */ }
//   castle::events::function<int> cb(&handler);
//
//   callback_registry<4, void(int)> registry;
//   auto sub = registry.subscribe(&cb);   // subscription is self-contained
//   registry.invoke(42);
//   sub.unsubscribe();                    // self-unsubscribe
// -----------------------------------------------------------------------------
template <
    std::size_t max_callback,
    typename signature>
class callback_registry;

template <
    std::size_t max_callback,
    typename return_type,
    typename... Args>
class callback_registry<max_callback, return_type(Args...)> final
    : public i_unsubscribable
{
    static_assert(std::is_void_v<return_type>,
                  "callback_registry requires void callback return type");

public:
    // callback_registry enforces return_type == void above, so the stored
    // base pointer is i_function<void(Args...)>.
    using callback_type = i_function<return_type(Args...)>;
    using subscription = callback_subscription;
    using error = callback_subscription_error;

    callback_registry() = default;
    ~callback_registry() override = default;

    // Non-copyable, non-movable. Registry identity is tied to slot storage
    // AND to the back-pointer embedded in outstanding subscriptions.
    callback_registry(const callback_registry&) = delete;
    callback_registry& operator=(const callback_registry&) = delete;

    callback_registry(callback_registry&&) = delete;
    callback_registry& operator=(callback_registry&&) = delete;

    // -------------------------------------------------------------------------
    // Subscribe a non-owning pointer to an i_function<Args...> instance.
    // The caller must keep the callback object alive until unsubscribe() or
    // clear() is called (or until this registry is destroyed).
    //
    // Returns a subscription handle. On failure the returned handle is
    // !valid() and out_error (if provided) is set.
    // -------------------------------------------------------------------------
    subscription subscribe(callback_type* callback, error* out_error = nullptr) noexcept
    {
        if (callback == nullptr)
        {
            if (out_error != nullptr)
            {
                *out_error = error::invalid_callback;
            }
            return subscription{};
        }

        for (std::size_t i = 0; i < max_callback; ++i)
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

    // -------------------------------------------------------------------------
    // Type-erased unsubscribe entry point used by callback_subscription.
    // Not intended for direct client use — prefer subscription::unsubscribe().
    // -------------------------------------------------------------------------
    error unsubscribe_slot(std::size_t index, std::uint32_t generation) noexcept override
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

    // -------------------------------------------------------------------------
    // Invoke all active callbacks in registration order (slot 0 .. N-1).
    // Dispatch goes through i_function<Args...>::operator() — this is a single
    // virtual call. Concrete variants like function_ct / function_ct_im often
    // devirtualise/inline the underlying target.
    // -------------------------------------------------------------------------
    void invoke(Args... args)
    {
        for (std::size_t i = 0; i < max_callback; ++i)
        {
            callback_type* callback = slots_[i].callback;

            if (callback != nullptr)
            {
                (*callback)(std::forward<Args>(args)...);
            }
        }
    }

    // -------------------------------------------------------------------------
    // Convenience operator() overload to allow registry to be called like a
    // function. This is equivalent to invoke() but may be more natural in some
    // contexts.
    // -------------------------------------------------------------------------
    void operator()(Args... args)
    {
        this->invoke(std::forward<Args>(args)...);
    }

    // -------------------------------------------------------------------------
    // Remove all active callbacks. Bumps generation on each cleared slot so
    // outstanding subscription handles become stale. Callback objects
    // themselves are untouched (caller owns their lifetime).
    // -------------------------------------------------------------------------
    void clear() noexcept
    {
        for (std::size_t i = 0; i < max_callback; ++i)
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

    constexpr std::size_t size() const noexcept
    {
        return active_count_;
    }

    constexpr bool empty() const noexcept
    {
        return active_count_ == 0;
    }

    static constexpr std::size_t capacity() noexcept
    {
        return max_callback;
    }

private:
    // Each slot holds a non-owning callback pointer and a generation counter
    // for identity. Generation increments on each deactivation so that stale
    // subscription handles are detected.
    struct slot
    {
        callback_type* callback = nullptr;
        std::uint32_t generation = 0;
    };

    std::array<slot, max_callback> slots_{};
    std::size_t active_count_ = 0;
};

} // namespace callbacks
} // namespace castle
