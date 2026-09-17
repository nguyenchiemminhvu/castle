// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file subscription.hpp
 * @brief Defines the callback unsubscription interface and the lightweight handle returned by callback registries.
 *
 * Use this header when a component needs to hold, copy, or pass around a stable identifier for a callback registration
 * without knowing the concrete registry type. A subscription stores only an owner pointer plus slot identity metadata;
 * it never allocates and performs no RTTI-based lookup.
 *
 * Key constraints:
 * - No dynamic allocation.
 * - No automatic unsubscription on destruction.
 * - Copying duplicates the same slot identity; only the first successful unsubscribe can remove the slot.
 * - Thread-safety is not provided by this handle itself. Any cross-thread coordination depends on the owning registry.
 * - The owner pointer is non-owning; calling unsubscribe() after the registry has been destroyed is invalid.
 *
 * @code
 * #include "castle/callbacks/delegate_registry.hpp"
 * #include "castle/callbacks/delegate.hpp"
 *
 * void on_event(int) noexcept {}
 *
 * int main()
 * {
 *     castle::callbacks::delegate_ptr<void(int)> callback{&on_event};
 *     castle::callbacks::delegate_registry<2U, void(int)> registry;
 *
 *     castle::callbacks::subscription sub = registry.subscribe(&callback);
 *     if (sub.valid())
 *     {
 *         registry(1);
 *         sub.unsubscribe();
 *     }
 *
 *     return 0;
 * }
 * @endcode
 */
#ifndef CASTLE_CALLBACKS_SUBSCRIPTION_HPP
#define CASTLE_CALLBACKS_SUBSCRIPTION_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace callbacks
{

/**
 * @brief Type-erased owner interface used by subscription to remove a registered callback slot.
 *
 * Concrete registries implement this interface so that a subscription can request unsubscription without encoding the
 * registry's template arguments in the handle type.
 *
 * @note Implementations are expected to reject stale or inactive slot identities with status::invalid_subscription.
 * @warning This interface does not manage lifetime. A subscription that outlives its owner contains a dangling pointer.
 */
class i_unsubscribable
{
public:
    /**
     * @brief Destroys the unsubscribe interface.
     *
     * @note The destructor is virtual so a subscription can reference different registry specializations through this
     * interface.
     */
    virtual ~i_unsubscribable() CASTLE_DEFAULT;

    /**
     * @brief Removes the slot identified by an index and generation pair.
     * @param index Slot index inside the owning registry.
     * @param generation Generation captured when the subscription was created.
     * @return status::ok when the slot is removed; otherwise status::invalid_subscription.
     * @note Implementations use the generation to detect stale handles after slot reuse or clear().
     */
    virtual status unsubscribe_slot(
        size_type index,
        uint32_t generation) CASTLE_NOEXCEPT = 0;
};

/**
 * @brief Value-type handle that identifies one registry slot and can request unsubscription.
 *
 * A default-constructed handle is invalid. A valid handle remains usable until unsubscribe() or reset() is called, or
 * until the owning registry invalidates the slot by unsubscribing, clearing, or destroying itself.
 *
 * @note Destroying a subscription object does not unsubscribe.
 * @warning If the owning registry is destroyed first, this handle must not call unsubscribe() because owner_ becomes
 * dangling.
 */
class subscription
{
public:
    /**
     * @brief Constructs an invalid subscription handle.
     *
     * @note The resulting handle does not refer to any registry slot.
     */
    CASTLE_CONSTEXPR subscription() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Constructs a handle for a specific registry slot.
     * @param owner Non-owning pointer to the registry interface that can remove the slot.
     * @param index Slot index inside the owning registry.
     * @param generation Slot generation captured at subscription time.
     * @note This constructor marks the handle valid when @p owner is non-null and the supplied identity is intended to
     * name a live slot.
     */
    CASTLE_CONSTEXPR subscription(
        i_unsubscribable* owner,
        size_type index,
        uint32_t generation) CASTLE_NOEXCEPT
        : owner_(owner),
          index_(index),
          generation_(generation),
          valid_(true)
    {
    }

    /**
     * @brief Reports whether this handle still contains an owner pointer and has not been locally invalidated.
     * @return True when the handle is locally marked valid and still stores a non-null owner pointer.
     * @note This does not probe the registry. A handle may report true even if the slot has already been cleared or
     * the owner has been destroyed elsewhere.
     */
    CASTLE_CONSTEXPR bool valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid_ && owner_ != nullptr;
    }

    /**
     * @brief Tests whether the handle is locally valid.
     * @return The same value as valid().
     */
    explicit operator bool() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid();
    }

    /**
     * @brief Returns the stored slot index.
     * @return The slot index captured when the subscription was created, or zero after reset().
     * @note This is an inspection helper only; it does not prove that the slot is still active.
     */
    CASTLE_CONSTEXPR size_type index() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index_;
    }

    /**
     * @brief Returns the stored slot generation.
     * @return The generation captured when the subscription was created, or zero after reset().
     * @note Registries increment the generation whenever a slot is deactivated so stale handles can be rejected.
     */
    CASTLE_CONSTEXPR uint32_t generation() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return generation_;
    }

    /**
     * @brief Requests removal of the referenced slot from the owning registry.
     * @return status::ok when the slot is removed; status::invalid_subscription when the handle is invalid, stale, or
     * the slot is already inactive.
     * @note This handle is reset regardless of the result, so repeated calls become invalid_subscription no-ops.
     * @warning Do not call this after the owning registry has been destroyed.
     */
    status unsubscribe() CASTLE_NOEXCEPT
    {
        if (!valid())
        {
            return status::invalid_subscription;
        }

        CASTLE_CONST status result =
            owner_->unsubscribe_slot(index_, generation_);

        reset();

        return result;
    }

    /**
     * @brief Invalidates this handle without touching any registry slot.
     * @note Use this to discard a handle explicitly. Destroying the handle has the same effect on the object itself but
     * does not change the registry.
     */
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

#endif // CASTLE_CALLBACKS_SUBSCRIPTION_HPP
