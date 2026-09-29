// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file node.hpp
 * @brief Hardware-independent, allocation-free sender and receiver node adapters.
 *
 * Nodes store their handlers in `castle::callbacks::function` with compile-time
 * inline storage, so no heap allocation or function-pointer context is needed.
 * Platform-specific CAN/CAN-FD controllers, interrupts, queues, and pin drivers
 * remain outside this header and can be integrated without virtual dispatch.
 */
#ifndef CASTLE_EXT_PROTOCOLS_CAN_NODE_HPP
#define CASTLE_EXT_PROTOCOLS_CAN_NODE_HPP

#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle_ext/protocols/can/frame.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace can
{

/**
 * @brief Allocation-free sender node facade.
 *
 * @tparam CallbackStorageSize Inline storage reserved for the transmit callback.
 * @tparam CallbackStorageAlignment Alignment used for the transmit callback storage.
 *
 * The user-provided callback owns the actual controller/driver operation.
 * Application sends are limited to data and remote frames: error and overload
 * frames are bus-level controller signaling and are not normal software TX frames.
 */
template <
    castle::size_type CallbackStorageSize = castle::inplace_storage_reserved,
    castle::size_type CallbackStorageAlignment = castle::inplace_alignment_default>
class sender_node CASTLE_FINAL
{
    static_assert(CallbackStorageSize > 0U,
                  "sender_node callback storage must be non-zero");
    static_assert(CallbackStorageAlignment > 0U
                  && (CallbackStorageAlignment & (CallbackStorageAlignment - 1U)) == 0U,
                  "sender_node callback storage alignment must be a power of two");

public:
    using transmit_callback_type = castle::callbacks::function<
        castle::status(CASTLE_CONST frame&),
        CallbackStorageSize,
        CallbackStorageAlignment>;

    sender_node() CASTLE_NOEXCEPT : handler_{}, sent_count_(0U) {}

    explicit sender_node(transmit_callback_type handler) CASTLE_NOEXCEPT
        : handler_(CASTLE_MOVE(handler)), sent_count_(0U)
    {
    }

    /** @brief Replaces the hardware adapter callback. */
    void configure(transmit_callback_type&& handler) CASTLE_NOEXCEPT
    {
        handler_ = CASTLE_MOVE(handler);
    }

    /** @brief Stores any callable compatible with the transmit callback signature. */
    template <
        typename Callback,
        typename = castle::meta::enable_if_t<
            !castle::meta::is_same<
                castle::meta::decay_t<Callback>,
                transmit_callback_type>::value>>
    void configure(Callback&& handler)
    {
        transmit_callback_type wrapper(CASTLE_FORWARD<Callback>(handler));
        handler_ = CASTLE_MOVE(wrapper);
    }

    /** @brief Removes the hardware adapter callback. */
    void clear() CASTLE_NOEXCEPT
    {
        handler_ = transmit_callback_type{};
    }

    /** @brief Returns the number of successful send requests (wraps modulo 2^32). */
    CASTLE_NODISCARD uint32_t sent_count() CASTLE_CONST CASTLE_NOEXCEPT { return sent_count_; }

    /**
     * @brief Validates and transmits an application data or remote frame.
     * @return Adapter status, `not_configured` when no callback is installed,
     * or `invalid_argument` for malformed or controller-generated signal frames.
     */
    castle::status send(const frame& value) CASTLE_NOEXCEPT
    {
        if (!value.valid() || value.kind == frame_kind::error || value.kind == frame_kind::overload)
        {
            return castle::status::invalid_argument;
        }
        if (!handler_)
        {
            return castle::status::not_configured;
        }
        const castle::status result = handler_(value);
        if (castle::succeeded(result))
        {
            ++sent_count_;
        }
        return result;
    }

private:
    transmit_callback_type handler_;
    uint32_t sent_count_;
};

/**
 * @brief Allocation-free receiver node facade.
 *
 * @tparam CallbackStorageSize Inline storage reserved for the receive callback.
 * @tparam CallbackStorageAlignment Alignment used for the receive callback storage.
 *
 * Feed frames delivered by a platform adapter to `receive()`. Valid error and
 * overload signaling events are surfaced like other received frames. Counters
 * are ordinary integers; synchronization is the integration layer's job when
 * receive is called from an ISR and another execution context reads counters.
 */
template <
    castle::size_type CallbackStorageSize = castle::inplace_storage_reserved,
    castle::size_type CallbackStorageAlignment = castle::inplace_alignment_default>
class receiver_node CASTLE_FINAL
{
    static_assert(CallbackStorageSize > 0U,
                  "receiver_node callback storage must be non-zero");
    static_assert(CallbackStorageAlignment > 0U
                  && (CallbackStorageAlignment & (CallbackStorageAlignment - 1U)) == 0U,
                  "receiver_node callback storage alignment must be a power of two");

public:
    using receive_callback_type = castle::callbacks::function<
        void(CASTLE_CONST frame&),
        CallbackStorageSize,
        CallbackStorageAlignment>;

    receiver_node() CASTLE_NOEXCEPT : handler_{}, received_count_(0U), invalid_count_(0U) {}

    explicit receiver_node(receive_callback_type handler) CASTLE_NOEXCEPT
        : handler_(CASTLE_MOVE(handler)), received_count_(0U), invalid_count_(0U)
    {
    }

    /** @brief Replaces the application callback. */
    void configure(receive_callback_type&& handler) CASTLE_NOEXCEPT
    {
        handler_ = CASTLE_MOVE(handler);
    }

    /** @brief Stores any callable compatible with the receive callback signature. */
    template <
        typename Callback,
        typename = castle::meta::enable_if_t<
            !castle::meta::is_same<
                castle::meta::decay_t<Callback>,
                receive_callback_type>::value>>
    void configure(Callback&& handler)
    {
        receive_callback_type wrapper(CASTLE_FORWARD<Callback>(handler));
        handler_ = CASTLE_MOVE(wrapper);
    }

    /** @brief Removes the application callback. */
    void clear() CASTLE_NOEXCEPT
    {
        handler_ = receive_callback_type{};
    }

    /** @brief Number of valid frames accepted (wraps modulo 2^32). */
    CASTLE_NODISCARD uint32_t received_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return received_count_;
    }

    /** @brief Number of malformed frame values rejected (wraps modulo 2^32). */
    CASTLE_NODISCARD uint32_t invalid_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return invalid_count_;
    }

    /** @brief Validates and dispatches one received frame. */
    castle::status receive(const frame& value) CASTLE_NOEXCEPT
    {
        if (!value.valid())
        {
            ++invalid_count_;
            return castle::status::invalid_argument;
        }
        ++received_count_;
        if (!handler_)
        {
            return castle::status::not_configured;
        }
        handler_(value);
        return castle::status::ok;
    }

private:
    receive_callback_type handler_;
    uint32_t received_count_;
    uint32_t invalid_count_;
};

} // namespace can
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_CAN_NODE_HPP
