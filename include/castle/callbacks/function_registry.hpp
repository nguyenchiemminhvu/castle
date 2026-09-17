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

// -----------------------------------------------------------------------------
// Fixed-capacity callback registry.
// Stores up to max_callback callbacks by value using function
// (no heap). Invocation order = subscription order (first-registered fires
// first).
//
// Unlike the non-owning delegate_registry, this registry OWNS each callback
// via function's internal buffer — so lambdas with captures and other
// stateful callables are fully supported without external lifetime management.
//
// Template parameters:
//   max_callback               - maximum number of concurrent subscriptions
//   signature                  - callback signature, e.g. void(int, float)
//   callback_storage_size      - function internal buffer size
//   callback_storage_alignment - function internal buffer alignment
//
// Usage:
//   function_registry<4, void(int)> registry;
//   auto sub = registry.subscribe([](int v) { /* ... */ });
//   registry.invoke(42);
//   sub.unsubscribe();
// -----------------------------------------------------------------------------
template <
    size_type max_callback,
    typename signature,
    size_type callback_storage_size = castle::inplace_storage_reserved,
    size_type callback_storage_alignment = castle::inplace_alignment_default>
class function_registry;

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

    function_registry() CASTLE_DEFAULT;
    ~function_registry() override CASTLE_DEFAULT;

    // Non-copyable, non-movable. Registry identity is tied to slot storage
    // AND to the back-pointer embedded in outstanding subscriptions.
    function_registry(CASTLE_CONST function_registry&) CASTLE_DELETE;
    function_registry& operator=(CASTLE_CONST function_registry&) CASTLE_DELETE;

    function_registry(function_registry&&) CASTLE_DELETE;
    function_registry& operator=(function_registry&&) CASTLE_DELETE;

    // -------------------------------------------------------------------------
    // Subscribe a ready-made function callback.
    // Returns a subscription handle. On failure the returned handle is
    // !valid() and out_error (if provided) is set.
    // -------------------------------------------------------------------------
    subscription subscribe(callback_type&& callback, error* out_error = nullptr)
    {
        if (!callback)
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

            if (!current_slot.active)
            {
                current_slot.callback = CASTLE_MOVE(callback);
                current_slot.active = true;
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
    // Subscribe any callable that is convertible to function.
    // Wraps the callable into callback_type and delegates to the primary
    // subscribe(callback_type&&) above.
    // -------------------------------------------------------------------------
    template <typename callback_t,
              typename = meta::enable_if_t<
                !meta::is_same<meta::decay_t<callback_t>, callback_type>::value
             && !meta::is_same<meta::decay_t<callback_t>, subscription>::value, void>>
    subscription subscribe(callback_t&& callback, error* out_error = nullptr)
    {
        callback_type callback_wrapper{CASTLE_FORWARD<callback_t>(callback)};
        return subscribe(CASTLE_MOVE(callback_wrapper), out_error);
    }

    // -------------------------------------------------------------------------
    // Type-erased unsubscribe entry point used by subscription.
    // Not intended for direct client use — prefer subscription::unsubscribe().
    // -------------------------------------------------------------------------
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

    // -------------------------------------------------------------------------
    // Invoke all active callbacks in subscription order (slot 0 .. N-1).
    // -------------------------------------------------------------------------
    void invoke(Args... args)
    {
        for (size_type i = 0; i < max_callback; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.active && current_slot.callback)
            {
                current_slot.callback(CASTLE_FORWARD<Args>(args)...);
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
    // outstanding subscription handles become stale.
    // -------------------------------------------------------------------------
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

    CASTLE_CONSTEXPR size_type size() CASTLE_CONST noexcept
    {
        return active_count_;
    }

    CASTLE_CONSTEXPR bool empty() CASTLE_CONST noexcept
    {
        return active_count_ == 0;
    }

    static CASTLE_CONSTEXPR size_type capacity() noexcept
    {
        return max_callback;
    }

private:
    // Each slot holds one callback, a generation counter for identity, and an
    // active flag. Generation increments on each deactivation so that stale
    // subscription handles are detected.
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
