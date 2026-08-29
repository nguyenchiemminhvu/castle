#pragma once

#include <cstddef>
#include <cstdint>

namespace castle
{
namespace callbacks
{

// -----------------------------------------------------------------------------
// Error codes returned by callback_registry operations.
// -----------------------------------------------------------------------------
enum class callback_subscription_error : std::uint8_t
{
    ok = 0,
    full,
    invalid_callback,
    invalid_subscription
};

// -----------------------------------------------------------------------------
// Type-erased unsubscribe interface.
// Every callback_registry instantiation implements this so a callback_
// subscription can call back into its owning registry without knowing the
// concrete signature/max_callback template parameters.
// -----------------------------------------------------------------------------
class i_unsubscribable
{
public:
    virtual ~i_unsubscribable() = default;

    // Remove the slot identified by (index, generation). Implementations must
    // treat mismatched generation / inactive slot as invalid_subscription.
    virtual callback_subscription_error unsubscribe_slot(
        std::size_t index,
        std::uint32_t generation) noexcept = 0;
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
    constexpr callback_subscription() noexcept = default;

    constexpr callback_subscription(
        i_unsubscribable* owner,
        std::size_t index,
        std::uint32_t generation) noexcept
        : owner_(owner),
          index_(index),
          generation_(generation),
          valid_(true)
    {
    }

    constexpr bool valid() const noexcept
    {
        return valid_ && owner_ != nullptr;
    }

    explicit operator bool() const noexcept
    {
        return valid();
    }

    constexpr std::size_t index() const noexcept
    {
        return index_;
    }

    constexpr std::uint32_t generation() const noexcept
    {
        return generation_;
    }

    // Non-const because a successful unsubscribe invalidates *this in place.
    callback_subscription_error unsubscribe() noexcept
    {
        if (!valid())
        {
            return callback_subscription_error::invalid_subscription;
        }

        const callback_subscription_error result =
            owner_->unsubscribe_slot(index_, generation_);

        // Regardless of success/failure, this handle no longer refers to a
        // live slot. Reset so subsequent calls are no-ops.
        reset();

        return result;
    }

    void reset() noexcept
    {
        owner_ = nullptr;
        index_ = 0;
        generation_ = 0;
        valid_ = false;
    }

private:
    i_unsubscribable* owner_ = nullptr;
    std::size_t index_ = 0;
    std::uint32_t generation_ = 0;
    bool valid_ = false;
};

} // namespace callbacks
} // namespace castle