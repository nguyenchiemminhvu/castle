#ifndef CASTLE_EVENTS_INPLACE_SIGSLOT_H
#define CASTLE_EVENTS_INPLACE_SIGSLOT_H

#include "castle/core/compiler.h"
#include "castle/core/config.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/utility/move.h"
#include "castle/utility/forward.h"

#include "castle/container/array.h"

#include "castle/callbacks/inplace_function.h"

#include <stdint.h>

namespace castle
{
namespace sigslot
{

// -----------------------------------------------------------------------------
// Error codes returned by signal operations.
// -----------------------------------------------------------------------------

enum class signal_error : uint8_t
{
    ok = 0,
    full,
    invalid_connection
};

// -----------------------------------------------------------------------------
// Fixed-capacity signal.
//
// A signal owns a bounded number of slots.
//
// Each slot owns its callback through inplace_function, therefore no dynamic
// allocation is required.
//
// Invocation order:
//
//     slot 0 -> slot 1 -> ... -> slot MaxSlot - 1
//
// Only void-returning signals are supported.
//
// Template parameters:
//
// MaxSlot
//     Maximum number of simultaneous connections.
//
// Signature
//     Signal signature, e.g. void(std::uint16_t)
//
// StorageSize
//     Inline storage size of each callback.
//
// StorageAlignment
//     Inline storage alignment of each callback.
// -----------------------------------------------------------------------------

template <
    size_type MaxSlot,
    typename Signature,
    size_type StorageSize = castle::inplace_storage_reserved,
    size_type StorageAlignment = castle::inplace_alignment_default>
class signal;

// -----------------------------------------------------------------------------
// signal_connection
//
// A connection represents the lifetime of one subscription.
//
// Ownership model:
//
//     signal
//       |
//       +--> slot --> callback
//       |
//       +--> connection pointer
//
//     signal_connection
//       |
//       +--> identifies one slot
//
// The connection does NOT own the callback.
//
// The connection is move-only because there must be exactly one RAII owner
// responsible for disconnecting the subscription.
//
// No dynamic allocation.
// No virtual dispatch.
// -----------------------------------------------------------------------------

template <
    typename Signal,
    size_type MaxSlot>
class signal_connection
{
public:

    signal_connection() noexcept CASTLE_DEFAULT;

    ~signal_connection()
    {
        disconnect();
    }

    signal_connection(CASTLE_CONST signal_connection&) CASTLE_DELETE;
    signal_connection& operator=(CASTLE_CONST signal_connection&) CASTLE_DELETE;

    signal_connection(signal_connection&& other) noexcept
        : owner_(other.owner_)
        , index_(other.index_)
        , generation_(other.generation_)
        , valid_(other.valid_)
    {
        if (valid_ && owner_ != nullptr)
        {
            owner_->rebind_connection(
                index_,
                generation_,
                this
            );
        }

        other.reset();
    }

    signal_connection& operator=(signal_connection&& other) noexcept
    {
        if (this != &other)
        {
            disconnect();

            owner_ = other.owner_;
            index_ = other.index_;
            generation_ = other.generation_;
            valid_ = other.valid_;

            if (valid_ && owner_ != nullptr)
            {
                owner_->rebind_connection(
                    index_,
                    generation_,
                    this
                );
            }

            other.reset();
        }

        return *this;
    }

    // -------------------------------------------------------------------------
    // Explicitly disconnect this connection.
    //
    // Calling disconnect() more than once is safe.
    // -------------------------------------------------------------------------

    signal_error disconnect() noexcept
    {
        if (!valid_ || owner_ == nullptr)
        {
            return signal_error::invalid_connection;
        }

        Signal* owner = owner_;

        CASTLE_CONST signal_error result =
            owner->disconnect_slot(
                index_,
                generation_,
                this
            );

        if (result == signal_error::ok)
        {
            reset();
        }

        return result;
    }

    // -------------------------------------------------------------------------
    // Query whether this connection still represents a live subscription.
    // -------------------------------------------------------------------------

    bool connected() CASTLE_CONST noexcept
    {
        return valid_ && owner_ != nullptr;
    }

    explicit operator bool() CASTLE_CONST noexcept
    {
        return connected();
    }

private:

    using signal_type = Signal;

    friend Signal;

    signal_connection(Signal* owner, size_type index, uint32_t generation) noexcept
        : owner_(owner)
        , index_(index)
        , generation_(generation)
        , valid_(true)
    {
        if (owner_ != nullptr)
        {
            owner_->rebind_connection(
                index_,
                generation_,
                this
            );
        }
    }

    void reset() noexcept
    {
        owner_ = nullptr;
        index_ = 0U;
        generation_ = 0U;
        valid_ = false;
    }

    Signal* owner_ = nullptr;
    size_type index_ = 0U;
    uint32_t generation_ = 0U;
    bool valid_ = false;
};

// -----------------------------------------------------------------------------
// signal specialization for function signatures.
// -----------------------------------------------------------------------------

template <
    size_type MaxSlot,
    typename ReturnType,
    typename... Args,
    size_type StorageSize,
    size_type StorageAlignment>
class signal<
    MaxSlot,
    ReturnType(Args...),
    StorageSize,
    StorageAlignment>
{
    static_assert(MaxSlot > 0,
                  "signal requires MaxSlot > 0");

    static_assert(StorageSize > 0,
                  "signal requires StorageSize > 0");

    static_assert(StorageAlignment > 0,
                  "signal requires StorageAlignment > 0");

    static_assert(meta::is_void<ReturnType>::value,
                  "signal currently supports void return type only");

public:

    using callback_type = callbacks::inplace_function<ReturnType(Args...), StorageSize, StorageAlignment>;
    using connection_type = signal_connection<signal, MaxSlot>;

    signal() CASTLE_DEFAULT;

    ~signal()
    {
        invalidate_connections();
    }

    signal(CASTLE_CONST signal&) CASTLE_DELETE;
    signal& operator=(CASTLE_CONST signal&) CASTLE_DELETE;

    signal(signal&&) CASTLE_DELETE;
    signal& operator=(signal&&) CASTLE_DELETE;

    // -------------------------------------------------------------------------
    // Connect a ready-made inplace_function.
    // -------------------------------------------------------------------------

    connection_type connect(callback_type&& callback, signal_error* out_error = nullptr)
    {
        if (!callback)
        {
            set_error(out_error, signal_error::invalid_connection);
            return connection_type{};
        }

        for (size_type i = 0U; i < MaxSlot; ++i)
        {
            slot& current_slot = slots_[i];

            if (!current_slot.active)
            {
                current_slot.callback = CASTLE_MOVE(callback);
                current_slot.active = true;

                ++active_count_;

                set_error(out_error, signal_error::ok);

                return connection_type{
                    this,
                    i,
                    current_slot.generation
                };
            }
        }

        set_error(out_error, signal_error::full);

        return connection_type{};
    }

    // -------------------------------------------------------------------------
    // Connect any callable supported by inplace_function.
    //
    // Supported examples:
    //
    //     lambda
    //     lambda with capture
    //     free function
    //     static member function
    //     functor
    // -------------------------------------------------------------------------

    template <
        typename Callable,
        typename meta::enable_if_t<
            !meta::is_same<typename meta::decay<Callable>::type, callback_type>::value &&
            !meta::is_same<typename meta::decay<Callable>::type, connection_type>::value,
            int> = 0>
    connection_type connect(Callable&& callback, signal_error* out_error = nullptr)
    {
        callback_type callback_wrapper{CASTLE_FORWARD<Callable>(callback)};
        return connect(CASTLE_MOVE(callback_wrapper), out_error);
    }

    // -------------------------------------------------------------------------
    // Connect a non-CASTLE_CONST member function.
    //
    // The object is NOT owned by the signal.
    //
    // The caller remains responsible for the object's lifetime.
    // -------------------------------------------------------------------------

    template <
        typename Object,
        typename Class,
        typename... MethodArgs>
    connection_type connect(
        Object& object,
        ReturnType (Class::*method)(MethodArgs...),
        signal_error* out_error = nullptr)
    {
        static_assert(meta::is_same<typename meta::decay<Object>::type, Class>::value,
                      "Member function class does not match object type");

        static_assert(meta::is_same<void(Args...), void(MethodArgs...)>::value,
                      "Member function signature does not match signal signature");

        Class* object_ptr = &object;

        return connect(
            [object_ptr, method](Args... args)
            {
                (object_ptr->*method)(CASTLE_FORWARD<Args>(args)...);
            },
            out_error
        );
    }

    // -------------------------------------------------------------------------
    // Connect a CASTLE_CONST member function.
    // -------------------------------------------------------------------------

    template <
        typename Object,
        typename Class,
        typename... MethodArgs>
    connection_type connect(
        CASTLE_CONST Object& object,
        ReturnType (Class::*method)(MethodArgs...) CASTLE_CONST,
        signal_error* out_error = nullptr)
    {
        static_assert(meta::is_same<typename meta::decay<Object>::type, Class>::value,
                      "Member function class does not match object type");

        static_assert(meta::is_same<void(Args...), void(MethodArgs...)>::value,
                      "Member function signature does not match signal signature");

        CASTLE_CONST Class* object_ptr = &object;

        return connect(
            [object_ptr, method](Args... args)
            {
                (object_ptr->*method)(CASTLE_FORWARD<Args>(args)...);
            },
            out_error
        );
    }

    // -------------------------------------------------------------------------
    // Emit the signal.
    //
    // No allocation.
    // No virtual dispatch.
    //
    // Runtime complexity:
    //
    //     O(MaxSlot)
    //
    // Callbacks are invoked in slot order.
    // -------------------------------------------------------------------------

    void emit(Args... args)
    {
        for (size_type i = 0U; i < MaxSlot; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.active && current_slot.callback)
            {
                current_slot.callback(CASTLE_FORWARD<Args>(args)...);
            }
        }
    }

    // -------------------------------------------------------------------------
    // Function-call syntax.
    // -------------------------------------------------------------------------

    void operator()(Args... args)
    {
        emit(CASTLE_FORWARD<Args>(args)...);
    }

    // -------------------------------------------------------------------------
    // Remove all connections.
    //
    // Outstanding connection objects become invalid.
    // -------------------------------------------------------------------------

    void disconnect_all() noexcept
    {
        for (size_type i = 0U; i < MaxSlot; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.active)
            {
                if (current_slot.connection_ != nullptr)
                {
                    current_slot.connection_->reset();
                }

                current_slot.connection_ = nullptr;
                current_slot.callback = callback_type{};
                current_slot.active = false;

                ++current_slot.generation;
            }
        }

        active_count_ = 0U;
    }

    // -------------------------------------------------------------------------
    // Number of currently connected slots.
    // -------------------------------------------------------------------------

    size_type size() CASTLE_CONST noexcept
    {
        return active_count_;
    }

    bool empty() CASTLE_CONST noexcept
    {
        return active_count_ == 0U;
    }

    static CASTLE_CONSTEXPR size_type capacity() noexcept
    {
        return MaxSlot;
    }

private:

    // -------------------------------------------------------------------------
    // One fixed slot.
    //
    // connection_ is non-owning.
    //
    // It exists so the signal can invalidate an outstanding connection when
    // the signal itself is destroyed or disconnect_all() is called.
    // -------------------------------------------------------------------------

    struct slot
    {
        callback_type callback;
        connection_type* connection_ = nullptr;
        uint32_t generation = 0U;
        bool active = false;
    };

    friend connection_type;

    // -------------------------------------------------------------------------
    // Set optional error output.
    // -------------------------------------------------------------------------

    static void set_error(signal_error* out_error, signal_error error) noexcept
    {
        if (out_error != nullptr)
        {
            *out_error = error;
        }
    }

    // -------------------------------------------------------------------------
    // Disconnect one slot.
    //
    // index + generation identify the logical connection.
    //
    // connection_ptr identifies the current RAII object.
    //
    // All three checks are intentional:
    //
    //     index
    //     generation
    //     connection object address
    //
    // This protects against stale connections and moved connections.
    // -------------------------------------------------------------------------

    signal_error disconnect_slot(
        size_type index,
        uint32_t generation,
        connection_type* connection_ptr) noexcept
    {
        if (index >= MaxSlot)
        {
            return signal_error::invalid_connection;
        }

        slot& current_slot = slots_[index];

        if (!current_slot.active)
        {
            return signal_error::invalid_connection;
        }

        if (current_slot.generation != generation)
        {
            return signal_error::invalid_connection;
        }

        if (current_slot.connection_ != connection_ptr)
        {
            return signal_error::invalid_connection;
        }

        current_slot.callback = callback_type{};
        current_slot.active = false;
        current_slot.connection_ = nullptr;

        ++current_slot.generation;
        --active_count_;

        return signal_error::ok;
    }

    // -------------------------------------------------------------------------
    // A connection can move.
    //
    // The slot must therefore point to the new connection object.
    // -------------------------------------------------------------------------

    void rebind_connection(
        size_type index,
        uint32_t generation,
        connection_type* new_connection) noexcept
    {
        if (index >= MaxSlot)
        {
            return;
        }

        slot& current_slot = slots_[index];

        if (!current_slot.active)
        {
            return;
        }

        if (current_slot.generation != generation)
        {
            return;
        }

        current_slot.connection_ = new_connection;
    }

    // -------------------------------------------------------------------------
    // Signal lifetime handling.
    //
    // When signal is destroyed, all outstanding connections are made inert
    // before the signal storage disappears.
    // -------------------------------------------------------------------------

    void invalidate_connections() noexcept
    {
        for (size_type i = 0U; i < MaxSlot; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.connection_ != nullptr)
            {
                current_slot.connection_->reset();
                current_slot.connection_ = nullptr;
            }
        }
    }

    container::array<slot, MaxSlot> slots_{};
    size_type active_count_ = 0U;
};

} // namespace sigslot
} // namespace castle

#endif // CASTLE_EVENTS_INPLACE_SIGSLOT_H
