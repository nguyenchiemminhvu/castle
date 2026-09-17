// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Fixed-capacity signal/slot primitive with RAII connection handles.
 *
 * Use this header when one component needs to notify a bounded set of
 * callbacks through a void-returning signature without heap allocation. Each
 * connected slot is stored inline in the signal object, connections are
 * represented by move-only handles, and callbacks fire in slot index order.
 *
 * @warning This component provides no internal synchronization. Do not race
 * connect(), disconnect(), disconnect_all(), destruction, and emit() across
 * threads or interrupts without external coordination.
 *
 * @code
 * #include "castle/events/sigslot.hpp"
 *
 * struct receiver
 * {
 *     std::uint32_t total = 0U;
 *
 *     void on_value(std::uint32_t value)
 *     {
 *         total += value;
 *     }
 * };
 *
 * int main()
 * {
 *     castle::sigslot::signal<2U, void(std::uint32_t)> sig;
 *     receiver rx{};
 *     auto connection = sig.connect(rx, &receiver::on_value);
 *
 *     sig.emit(5U);
 *
 *     return rx.total == 5U ? 0 : 1;
 * }
 * @endcode
 */
#ifndef CASTLE_EVENTS_SIGSLOT_HPP
#define CASTLE_EVENTS_SIGSLOT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/forward.hpp"

#include "castle/container/array.hpp"

#include "castle/callbacks/function.hpp"

#include <stdint.h>

namespace castle
{
namespace sigslot
{

/**
 * @brief Result codes returned by signal and connection operations.
 */
enum class signal_error : uint8_t
{
    ok = 0,                 /**< @brief Operation succeeded. */
    full,                   /**< @brief No inactive slot was available for a new connection. */
    invalid_connection      /**< @brief The handle or slot identity was no longer valid. */
};

/**
 * @brief Fixed-capacity signal storing callbacks by value.
 *
 * @tparam MaxSlot Maximum number of simultaneously active connections.
 * @tparam Signature Signal signature. Only void-returning signatures are supported.
 * @tparam StorageSize Inline storage reserved for each callback object.
 * @tparam StorageAlignment Alignment of each callback's inline storage.
 */
template <
    size_type MaxSlot,
    typename Signature,
    size_type StorageSize = castle::inplace_storage_reserved,
    size_type StorageAlignment = castle::inplace_alignment_default>
class signal;

/**
 * @brief Move-only handle that disconnects one signal slot.
 *
 * @tparam Signal Owning signal type.
 * @tparam MaxSlot Slot capacity of the owning signal.
 *
 * @note The handle does not own the callback object; it only identifies a slot
 * inside the owning signal.
 */
template <
    typename Signal,
    size_type MaxSlot>
class signal_connection
{
public:

    /** @brief Construct an empty, disconnected handle. */
    signal_connection() noexcept CASTLE_DEFAULT;

    /**
     * @brief Destroy the handle and disconnect if it still refers to a live slot.
     *
     * @note Destruction is idempotent because disconnect() validates the slot
     * identity before removing it.
     */
    ~signal_connection()
    {
        disconnect();
    }

    /** @brief Copying is disabled so one live slot has at most one RAII owner. */
    signal_connection(CASTLE_CONST signal_connection&) CASTLE_DELETE;
    signal_connection& operator=(CASTLE_CONST signal_connection&) CASTLE_DELETE;

    /**
     * @brief Move-construct a connection handle.
     *
     * @param other Source handle.
     *
     * @note The owning signal is rebound to this object's address so a later
     * disconnect() still matches the slot's current connection pointer.
     */
    signal_connection(signal_connection&& other) noexcept
        : owner_(other.owner_)
        , index_(other.index_)
        , generation_(other.generation_)
        , valid_(other.valid_)
    {
        if (valid_ && owner_ != nullptr) // LCOV_EXCL_BR_LINE
        {
            owner_->rebind_connection(
                index_,
                generation_,
                this
            );
        }

        other.reset();
    }

    /**
     * @brief Move-assign a connection handle.
     *
     * @param other Source handle.
     * @return *this.
     *
     * @note Any currently owned connection is disconnected before taking the
     * new one.
     */
    signal_connection& operator=(signal_connection&& other) noexcept
    {
        if (this != &other)
        {
            disconnect();

            owner_ = other.owner_;
            index_ = other.index_;
            generation_ = other.generation_;
            valid_ = other.valid_;

            if (valid_ && owner_ != nullptr) // LCOV_EXCL_BR_LINE
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

    /**
     * @brief Disconnect the represented slot.
     *
     * @return signal_error::ok when the slot is removed, otherwise
     * signal_error::invalid_connection.
     *
     * @note Calling disconnect() more than once is safe.
     */
    signal_error disconnect() noexcept
    {
        if (!valid_ || owner_ == nullptr) // LCOV_EXCL_BR_LINE
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

        if (result == signal_error::ok) // LCOV_EXCL_BR_LINE
        {
            reset();
        }

        return result;
    }

    /**
     * @brief Report whether this handle still names a live slot.
     *
     * @return true when the handle has not been reset locally, otherwise false.
     *
     * @warning A slot invalidated by signal destruction or disconnect_all()
     * becomes disconnected even if this function has not been polled yet.
     */
    bool connected() CASTLE_CONST noexcept
    {
        return valid_ && owner_ != nullptr; // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Boolean test for connected().
     *
     * @return connected().
     */
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
        if (owner_ != nullptr) // LCOV_EXCL_BR_LINE
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

    /** @brief Callback wrapper stored in each signal slot. */
    using callback_type = callbacks::function<ReturnType(Args...), StorageSize, StorageAlignment>;

    /** @brief Connection handle type returned by connect(). */
    using connection_type = signal_connection<signal, MaxSlot>;

    /** @brief Construct an empty signal with no active slots. */
    signal() CASTLE_DEFAULT;

    /**
     * @brief Destroy the signal and invalidate any outstanding connections.
     */
    ~signal()
    {
        invalidate_connections();
    }

    /** @brief Copy and move are disabled because slot state owns callbacks and connection back-pointers. */
    signal(CASTLE_CONST signal&) CASTLE_DELETE;
    signal& operator=(CASTLE_CONST signal&) CASTLE_DELETE;

    signal(signal&&) CASTLE_DELETE;
    signal& operator=(signal&&) CASTLE_DELETE;

    /**
     * @brief Connect a pre-built callback object.
     *
     * @param callback Callback to store by value in the first inactive slot.
     * @param out_error Optional output for the connection result.
     * @return A move-only connection handle. The returned handle is empty when
     * the callback is invalid or the signal is full.
     *
     * @note Slot selection is first-fit from index 0 upward.
     */
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

    /**
     * @brief Connect any callable supported by callback_type.
     *
     * @tparam Callable Callable type to wrap into callback_type.
     * @param callback Callable to store by value.
     * @param out_error Optional output for the connection result.
     * @return A connection handle for the inserted slot, or an empty handle on
     * failure.
     */
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

    /**
     * @brief Connect a non-const member function.
     *
     * @tparam Object Object type supplied by the caller.
     * @tparam Class Member function class type.
     * @tparam MethodArgs Member function parameter types.
     * @param object Target object.
     * @param method Member function pointer.
     * @param out_error Optional output for the connection result.
     * @return A connection handle for the inserted slot, or an empty handle on
     * failure.
     *
     * @warning The signal does not own object. The caller must keep it alive
     * for the lifetime of the connection.
     */
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
                (object_ptr->*method)(CASTLE_FORWARD<Args>(args)...); // LCOV_EXCL_BR_LINE
            },
            out_error
        );
    }

    /**
     * @brief Connect a const member function.
     *
     * @tparam Object Object type supplied by the caller.
     * @tparam Class Member function class type.
     * @tparam MethodArgs Member function parameter types.
     * @param object Target object.
     * @param method Const member function pointer.
     * @param out_error Optional output for the connection result.
     * @return A connection handle for the inserted slot, or an empty handle on
     * failure.
     *
     * @warning The signal does not own object. The caller must keep it alive
     * for the lifetime of the connection.
     */
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
                (object_ptr->*method)(CASTLE_FORWARD<Args>(args)...); // LCOV_EXCL_BR_LINE
            },
            out_error
        );
    }

    /**
     * @brief Invoke every active slot in ascending slot order.
     *
     * @param args Runtime arguments forwarded to each callback.
     *
     * @note Emission order is deterministic: slot 0, slot 1, and so on.
     * @warning The implementation does not guard against callbacks that modify
     * the same signal during emission.
     */
    void emit(Args... args)
    {
        for (size_type i = 0U; i < MaxSlot; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.active && current_slot.callback) // LCOV_EXCL_BR_LINE
            {
                current_slot.callback(CASTLE_FORWARD<Args>(args)...);
            }
        }
    }

    /**
     * @brief Invoke emit() using function-call syntax.
     *
     * @param args Runtime arguments forwarded to each callback.
     */
    void operator()(Args... args)
    {
        emit(CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Disconnect all active slots.
     *
     * @note Outstanding connection handles are reset when their slot still
     * stores the current handle pointer.
     */
    void disconnect_all() noexcept
    {
        for (size_type i = 0U; i < MaxSlot; ++i)
        {
            slot& current_slot = slots_[i];

            if (current_slot.active) // LCOV_EXCL_BR_LINE
            {
                if (current_slot.connection_ != nullptr) // LCOV_EXCL_BR_LINE
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

    /**
     * @brief Count active connections.
     *
     * @return Number of active slots.
     */
    size_type size() CASTLE_CONST noexcept
    {
        return active_count_;
    }

    /**
     * @brief Report whether no slots are active.
     *
     * @return true when size() is zero, otherwise false.
     */
    bool empty() CASTLE_CONST noexcept
    {
        return active_count_ == 0U;
    }

    /**
     * @brief Query the compile-time slot capacity.
     *
     * @return MaxSlot.
     */
    static CASTLE_CONSTEXPR size_type capacity() noexcept
    {
        return MaxSlot;
    }

private:

    struct slot
    {
        callback_type callback;
        connection_type* connection_ = nullptr;
        uint32_t generation = 0U;
        bool active = false;
    };

    friend connection_type;

    /**
     * @brief Set the error code if the output pointer is not null.
     * @param out_error Pointer to the output error code.
     */
    static void set_error(signal_error* out_error, signal_error error) noexcept
    {
        if (out_error != nullptr)
        {
            *out_error = error;
        }
    }

    /**
     * @brief Disconnect a specific slot based on its index, generation, and connection pointer.
     * @param index Index of the slot to disconnect.
     * @param generation Generation number of the slot to disconnect.
     * @param connection_ptr Pointer to the connection object associated with the slot to disconnect.
     * @return signal_error indicating the result of the disconnection attempt.
     * @note This function is intended for internal use by the signal dispatcher and should not be called directly.
     */
    signal_error disconnect_slot(
        size_type index,
        uint32_t generation,
        connection_type* connection_ptr) noexcept
    {
        // LCOV_EXCL_START
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

        // The stored connection address is part of the slot identity so moved
        // handles cannot disconnect an older object by mistake.
        if (current_slot.connection_ != connection_ptr)
        {
            return signal_error::invalid_connection;
        }
        // LCOV_EXCL_STOP

        current_slot.callback = callback_type{};
        current_slot.active = false;
        current_slot.connection_ = nullptr;

        ++current_slot.generation;
        --active_count_;

        return signal_error::ok;
    }

    /**
     * @brief Rebind a connection to a specific slot based on its index and generation.
     * @param index Index of the slot to rebind the connection for.
     * @param generation Generation number of the slot to rebind the connection for.
     * @param new_connection Pointer to the new connection object to bind to the slot.
     * @note This function is intended for internal use by the signal dispatcher and should not be called directly.
     */
    void rebind_connection(
        size_type index,
        uint32_t generation,
        connection_type* new_connection) noexcept
    {
        // LCOV_EXCL_START
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
        // LCOV_EXCL_STOP

        current_slot.connection_ = new_connection;
    }

    /**
     * @brief Invalidate all active connections by resetting them and setting their pointers to nullptr.
     * @note This function is intended for internal use by the signal dispatcher and should not be called directly.
     */
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

#endif // CASTLE_EVENTS_SIGSLOT_HPP
