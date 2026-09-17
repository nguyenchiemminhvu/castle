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

// -----------------------------------------------------------------------------
// Fixed-capacity callback registry (non-owning).
// Stores up to max_callback pointers to delegate_base<Args...> instances.
// The caller owns the lifetime of each callback object; the registry only
// borrows non-owning pointers. This mirrors the design philosophy of
// delegate.h — callback variants are user-defined objects with potentially
// zero storage cost, and forcing ownership here would defeat that goal.
//
// Invocation order = subscription order (first-registered fires first).
//
// Template parameters:
//   max_callback - maximum number of concurrent subscriptions
//   signature    - callback signature, e.g. void(int, float)
//
// Usage:
//   void handler(int v) { /* ... */ }
//   castle::callbacks::delegate_ptr<void(int)> cb(&handler);
//
//   delegate_registry<4, void(int)> registry;
//   auto sub = registry.subscribe(&cb);   // subscription is self-contained
//   registry.invoke(42);
//   sub.unsubscribe();                    // self-unsubscribe
// -----------------------------------------------------------------------------
template <
    size_type max_callback,
    typename signature>
class delegate_registry;

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
    // delegate_registry enforces return_type == void above, so the stored
    // base pointer is delegate_base<void(Args...)>.
    using callback_type = delegate_base<return_type(Args...)>;
    using subscription = castle::callbacks::subscription;
    using error = castle::status;

    delegate_registry() CASTLE_DEFAULT;
    ~delegate_registry() override CASTLE_DEFAULT;

    // Non-copyable, non-movable. Registry identity is tied to slot storage
    // AND to the back-pointer embedded in outstanding subscriptions.
    delegate_registry(CASTLE_CONST delegate_registry&) CASTLE_DELETE;
    delegate_registry& operator=(CASTLE_CONST delegate_registry&) CASTLE_DELETE;

    delegate_registry(delegate_registry&&) CASTLE_DELETE;
    delegate_registry& operator=(delegate_registry&&) CASTLE_DELETE;

    // -------------------------------------------------------------------------
    // Subscribe a non-owning pointer to an delegate_base<Args...> instance.
    // The caller must keep the callback object alive until unsubscribe() or
    // clear() is called (or until this registry is destroyed).
    //
    // Returns a subscription handle. On failure the returned handle is
    // !valid() and out_error (if provided) is set.
    // -------------------------------------------------------------------------
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

    // -------------------------------------------------------------------------
    // Type-erased unsubscribe entry point used by subscription.
    // Not intended for direct client use — prefer subscription::unsubscribe().
    // -------------------------------------------------------------------------
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

    // -------------------------------------------------------------------------
    // Invoke all active callbacks in subscription order (slot 0 .. N-1).
    // Dispatch goes through delegate_base<Args...>::operator() — this is a single
    // virtual call. Concrete variants like function_ct / function_ct_im often
    // devirtualise/inline the underlying target.
    // -------------------------------------------------------------------------
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

    // -------------------------------------------------------------------------
    // Convenience operator() overload to allow registry to be called like a
    // function. This is equivalent to invoke() but may be more natural in some
    // contexts.
    // -------------------------------------------------------------------------
    void operator()(Args... args)
    {
        this->invoke(CASTLE_FORWARD<Args>(args)...);
    }

    // -------------------------------------------------------------------------
    // Remove all active callbacks. Bumps generation on each cleared slot so
    // outstanding subscription handles become stale. Callback objects
    // themselves are untouched (caller owns their lifetime).
    // -------------------------------------------------------------------------
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

    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return active_count_;
    }

    CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return active_count_ == 0;
    }

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
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
        uint32_t generation = 0;
    };

    container::array<slot, max_callback> slots_{};
    size_type active_count_ = 0;
};

} // namespace callbacks
} // namespace castle

#endif // CASTLE_CALLBACKS_DELEGATE_REGISTRY_HPP
