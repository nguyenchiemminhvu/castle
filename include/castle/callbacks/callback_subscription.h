#ifndef CASTLE_CALLBACKS_CALLBACK_SUBSCRIPTION_H
#define CASTLE_CALLBACKS_CALLBACK_SUBSCRIPTION_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/error/status.h"

#include <stdint.h>

namespace castle
{
namespace callbacks
{

// -----------------------------------------------------------------------------
// Type-erased unsubscribe interface.
// Every callback_registry instantiation implements this so a callback_
// subscription can call back into its owning registry without knowing the
// concrete signature/max_callback template parameters.
// -----------------------------------------------------------------------------
class i_unsubscribable
{
public:
    virtual ~i_unsubscribable() CASTLE_DEFAULT;

    // Remove the slot identified by (index, generation). Implementations must
    // treat mismatched generation / inactive slot as invalid_subscription.
    virtual status unsubscribe_slot(
        size_type index,
        uint32_t generation) CASTLE_NOEXCEPT = 0;
};

// -----------------------------------------------------------------------------
// Subscription handle returned by callback_registry::subscribe().
// Holds slot index + generation to prevent stale unsubscribe from removing a
// reused slot. Also holds a non-owning back-pointer to its owning registry so
// that callers can call subscription.unsubscribe() directly (self-unsubscribe).
//
// Lightweight value type — safe to copy/store. Copies share the same identity;
// the first successful unsubscribe wins, subsequent ones return
// invalid_subscription.
// -----------------------------------------------------------------------------
class callback_subscription
{
public:
    CASTLE_CONSTEXPR callback_subscription() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    CASTLE_CONSTEXPR callback_subscription(
        i_unsubscribable* owner,
        size_type index,
        uint32_t generation) CASTLE_NOEXCEPT
        : owner_(owner),
          index_(index),
          generation_(generation),
          valid_(true)
    {
    }

    CASTLE_CONSTEXPR bool valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid_ && owner_ != nullptr;
    }

    explicit operator bool() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid();
    }

    CASTLE_CONSTEXPR size_type index() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index_;
    }

    CASTLE_CONSTEXPR uint32_t generation() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return generation_;
    }

    // Non-CASTLE_CONST because a successful unsubscribe invalidates *this in place.
    status unsubscribe() CASTLE_NOEXCEPT
    {
        if (!valid())
        {
            return status::invalid_subscription;
        }

        CASTLE_CONST status result =
            owner_->unsubscribe_slot(index_, generation_);

        // Regardless of success/failure, this handle no longer refers to a
        // live slot. Reset so subsequent calls are no-ops.
        reset();

        return result;
    }

    void reset() CASTLE_NOEXCEPT
    {
        owner_ = nullptr;
        index_ = 0;
        generation_ = 0;
        valid_ = false;
    }

private:
    i_unsubscribable* owner_ = nullptr;
    size_type index_ = 0;
    uint32_t generation_ = 0;
    bool valid_ = false;
};

} // namespace callbacks
} // namespace castle

#endif // CASTLE_CALLBACKS_CALLBACK_SUBSCRIPTION_H
