#pragma once

#include "castle/callbacks/inplace_function.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace castle
{
namespace sigslot
{

// -----------------------------------------------------------------------------
// Error codes returned by signal operations.
// -----------------------------------------------------------------------------

enum class signal_error : std::uint8_t
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
    std::size_t MaxSlot,
    typename Signature,
    std::size_t StorageSize = 64,
    std::size_t StorageAlignment = alignof(std::max_align_t)>
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
    std::size_t MaxSlot>
class signal_connection
{
public:

    signal_connection() noexcept = default;

    ~signal_connection()
    {
        disconnect();
    }

    signal_connection(const signal_connection&) = delete;
    signal_connection& operator=(const signal_connection&) = delete;

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

        const signal_error result =
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

    bool connected() const noexcept
    {
        return valid_ && owner_ != nullptr;
    }

    explicit operator bool() const noexcept
    {
        return connected();
    }

private:

    using signal_type = Signal;

    friend Signal;

    signal_connection(Signal* owner, std::size_t index, std::uint32_t generation) noexcept
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
    std::size_t index_ = 0U;
    std::uint32_t generation_ = 0U;
    bool valid_ = false;
};

// -----------------------------------------------------------------------------
// signal specialization for function signatures.
// -----------------------------------------------------------------------------

template <
    std::size_t MaxSlot,
    typename ReturnType,
    typename... Args,
    std::size_t StorageSize,
    std::size_t StorageAlignment>
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

    static_assert(std::is_void_v<ReturnType>,
                  "signal currently supports void return type only");

public:

    using callback_type = callbacks::inplace_function<ReturnType(Args...), StorageSize, StorageAlignment>;

    using connection_type = signal_connection<signal, MaxSlot>;

public:

    signal() = default;

    ~signal()
    {
        invalidate_connections();
    }

    signal(const signal&) = delete;
    signal& operator=(const signal&) = delete;

    signal(signal&&) = delete;
    signal& operator=(signal&&) = delete;

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

        for (std::size_t i = 0U; i < MaxSlot; ++i)
        {
            slot& current_slot = slots_[i];

            if (!current_slot.active)
            {
                current_slot.callback = std::move(callback);
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
        typename std::enable_if_t<
            !std::is_same_v<typename std::decay<Callable>::type, callback_type> &&
            !std::is_same_v<typename std::decay<Callable>::type, connection_type>,
            int> = 0>
    connection_type connect(Callable&& callback, signal_error* out_error = nullptr)
    {
        callback_type callback_wrapper{std::forward<Callable>(callback)};
        return connect(std::move(callback_wrapper), out_error);
    }

    // -------------------------------------------------------------------------
    // Connect a non-const member function.
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
        static_assert(std::is_same_v<typename std::decay<Object>::type, Class>,
                      "Member function class does not match object type");

        static_assert(std::is_same_v<void(Args...), void(MethodArgs...)>,
                      "Member function signature does not match signal signature");

        Class* object_ptr = &object;

        return connect(
            [object_ptr, method](Args... args)
            {
                (object_ptr->*method)(std::forward<Args>(args)...);
            },
            out_error
        );
    }

    // -------------------------------------------------------------------------
    // Connect a const member function.
    // -------------------------------------------------------------------------

    template <
        typename Object,
        typename Class,
        typename... MethodArgs>
    connection_type connect(
        const Object& object,
        ReturnType (Class::*method)(MethodArgs...) const,
        signal_error* out_error = nullptr)
    {
        static_assert(std::is_same_v<typename std::decay<Object>::type, Class>,
                      "Member function class does not match object type");

        static_assert(std::is_same_v<void(Args...), void(MethodArgs...)>,
                      "Member function signature does not match signal signature");

        const Class* object_ptr = &object;

        return connect(
            [object_ptr, method](Args... args)
            {
                (object_ptr->*method)(std::forward<Args>(args)...);
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
        for (std::size_t i = 0U; i < MaxSlot; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.active && current_slot.callback)
            {
                current_slot.callback(std::forward<Args>(args)...);
            }
        }
    }

    // -------------------------------------------------------------------------
    // Function-call syntax.
    // -------------------------------------------------------------------------

    void operator()(Args... args)
    {
        emit(std::forward<Args>(args)...);
    }

    // -------------------------------------------------------------------------
    // Remove all connections.
    //
    // Outstanding connection objects become invalid.
    // -------------------------------------------------------------------------

    void disconnect_all() noexcept
    {
        for (std::size_t i = 0U; i < MaxSlot; ++i)
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

    std::size_t size() const noexcept
    {
        return active_count_;
    }

    bool empty() const noexcept
    {
        return active_count_ == 0U;
    }

    static constexpr std::size_t capacity() noexcept
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
        std::uint32_t generation = 0U;
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
        std::size_t index,
        std::uint32_t generation,
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
        std::size_t index,
        std::uint32_t generation,
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
        for (std::size_t i = 0U; i < MaxSlot; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.connection_ != nullptr)
            {
                current_slot.connection_->reset();
                current_slot.connection_ = nullptr;
            }
        }
    }

    std::array<slot, MaxSlot> slots_{};
    std::size_t active_count_ = 0U;
};

} // namespace sigslot
} // namespace castle
