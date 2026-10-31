// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file client.hpp
 * @brief Fixed-capacity CoAP-over-UDP client with token matching and CON retransmission.
 */
#ifndef CASTLE_EXT_PROTOCOLS_COAP_CLIENT_HPP
#define CASTLE_EXT_PROTOCOLS_COAP_CLIENT_HPP

#include "castle/callbacks/callback_dispatcher.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/bytes.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle_ext/net/client.hpp"
#include "castle_ext/protocols/coap/codec.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace coap
{

/**
 * @brief Allocation-free CoAP client using the CASTLE UDP client.
 *
 * Time is supplied by the application in monotonic milliseconds. Call `process_once()`
 * regularly to receive datagrams and drive retransmission deadlines. Retransmission
 * intervals are deterministic (2, 4, 8, 16 seconds by default); no random source is used.
 */
template <size_type ReceiveBufferSize = 1152U,
          size_type MaxExchanges = 4U,
          size_type MaxOptions = 16U,
          size_type OptionValueCapacity = 64U,
          size_type PayloadCapacity = 1024U,
          size_type MaxRecentMessages = 8U,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
class basic_client
{
    static_assert(ReceiveBufferSize >= 4U, "CoAP UDP buffer must hold the four-byte header");
    static_assert(MaxExchanges > 0U, "CoAP client needs at least one exchange slot");
    static_assert(MaxRecentMessages > 0U, "CoAP client needs a duplicate cache slot");

public:
    using message_type_t = message<MaxOptions, OptionValueCapacity, PayloadCapacity>;
    using transport_type = castle::net::udp_client<ReceiveBufferSize>;

    struct exchange_handle
    {
        size_type index = invalid_index();
        uint32_t generation = 0U;
        static CASTLE_CONSTEXPR size_type invalid_index() CASTLE_NOEXCEPT { return static_cast<size_type>(-1); }
        bool is_valid() CASTLE_CONST CASTLE_NOEXCEPT { return index != invalid_index() && generation != 0U; }
        bool operator==(exchange_handle CASTLE_CONST& other) CASTLE_CONST CASTLE_NOEXCEPT
        { return index == other.index && generation == other.generation; }
    };

    using response_callback = castle::callbacks::function<void(exchange_handle, castle::status,
        message_type_t CASTLE_CONST*, castle::net::endpoint CASTLE_CONST&), CallbackStorageSize>;

    basic_client() CASTLE_DEFAULT;
    basic_client(basic_client CASTLE_CONST&) CASTLE_DELETE;
    basic_client& operator=(basic_client CASTLE_CONST&) CASTLE_DELETE;

    castle::status open(castle::net::address_family family = castle::net::address_family::ipv4) CASTLE_NOEXCEPT
    {
        return transport_.open(family, true);
    }

    castle::status close() CASTLE_NOEXCEPT
    {
        clear_exchanges();
        return transport_.close();
    }

    bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return transport_.is_open(); }
    transport_type& transport() CASTLE_NOEXCEPT { return transport_; }

    template <typename Callback>
    castle::status set_response_callback(Callback&& callback)
    {
        response_callback next(CASTLE_FORWARD<Callback>(callback));
        if (!next)
        {
            return castle::status::invalid_callback;
        }
        dispatcher_.replace(callback_, CASTLE_MOVE(next));
        return castle::status::ok;
    }

    void clear_response_callback() CASTLE_NOEXCEPT { dispatcher_.clear(callback_); }

    /** @brief Sets the first value returned by `allocate_message_id()` (zero is valid). */
    void set_next_message_id(uint16_t value) CASTLE_NOEXCEPT { next_message_id_ = value; }

    uint16_t allocate_message_id() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint16_t result = next_message_id_++;
        return result;
    }

    /** @brief Sends a CON/NON request and retains a bounded exchange entry for matching/cancellation. */
    castle::status send_request(castle::net::endpoint CASTLE_CONST& remote, message_type_t CASTLE_CONST& request,
                                exchange_handle& out, uint32_t now_ms) CASTLE_NOEXCEPT
    {
        out = exchange_handle{};
        if (!transport_.is_open())
        {
            return castle::status::not_configured;
        }
        if (!is_request_code(request.code_value) ||
            (request.type != message_type::confirmable && request.type != message_type::non_confirmable) ||
            request.token_length > maximum_token_length)
        {
            return castle::status::invalid_argument;
        }
        if (find_mid(remote, request.message_id) != MaxExchanges || find_token(remote, request) != MaxExchanges)
        {
            return castle::status::already_exists;
        }
        CASTLE_CONST size_type slot = find_free();
        if (slot == MaxExchanges)
        {
            return castle::status::full;
        }
        exchange& entry = exchanges_[slot];
        size_type encoded_size = 0U;
        castle::status result = encode_message(request, entry.wire, ReceiveBufferSize, encoded_size);
        if (!castle::succeeded(result))
        {
            return result;
        }
        size_type sent = 0U;
        result = transport_.send_to(entry.wire, encoded_size, remote, sent);
        if (!castle::succeeded(result))
        {
            return result;
        }
        if (sent != encoded_size)
        {
            return castle::status::data_loss;
        }
        entry.remote = remote;
        entry.message_id = request.message_id;
        entry.token_length = request.token_length;
        for (size_type i = 0U; i < request.token_length; ++i)
        {
            entry.token[i] = request.token[i];
        }
        entry.wire_size = encoded_size;
        entry.next_deadline = now_ms + ack_timeout_ms_;
        entry.timeout_ms = ack_timeout_ms_;
        entry.retransmissions = 0U;
        entry.acknowledged = false;
        entry.confirmable = request.type == message_type::confirmable;
        entry.observe = has_observe_registration(request);
        entry.active = true;
        entry.generation = next_generation(entry.generation);
        out = exchange_handle{slot, entry.generation};
        return castle::status::ok;
    }

    /** @brief Cancels one outstanding request or Observe registration. */
    castle::status cancel(exchange_handle handle) CASTLE_NOEXCEPT
    {
        exchange* entry = find(handle);
        if (entry == nullptr)
        {
            return castle::status::not_found;
        }
        entry->active = false;
        entry->wire_size = 0U;
        return castle::status::ok;
    }

    /** @brief Changes deterministic ACK timeout/retry policy. Retransmissions are bounded to eight. */
    castle::status set_retransmission(uint32_t ack_timeout_ms, uint8_t max_retransmissions) CASTLE_NOEXCEPT
    {
        if (ack_timeout_ms == 0U || max_retransmissions > 8U)
        {
            return castle::status::invalid_argument;
        }
        ack_timeout_ms_ = ack_timeout_ms;
        max_retransmissions_ = max_retransmissions;
        return castle::status::ok;
    }

    /** @brief Processes expired timers and at most one received UDP datagram. */
    castle::status process_once(uint32_t now_ms, int wait_ms = 0) CASTLE_NOEXCEPT
    {
        castle::status result = service_timers(now_ms);
        if (!castle::succeeded(result))
        {
            return result;
        }
        result = transport_.socket().wait_readable(wait_ms);
        if (result == castle::status::time_out)
        {
            return castle::status::empty;
        }
        if (!castle::succeeded(result))
        {
            return result;
        }
        uint8_t buffer[ReceiveBufferSize]{};
        size_type received = 0U;
        castle::net::endpoint source;
        result = transport_.receive_from(buffer, sizeof(buffer), received, source);
        if (result == castle::status::data_loss)
        {
            return result;
        }
        if (!castle::succeeded(result))
        {
            return result;
        }

        message_type_t incoming;
        decode_error reason = decode_error::none;
        result = decode_message(buffer, received, incoming, &reason);
        if (!castle::succeeded(result))
        {
            if (received >= 4U && ((buffer[0U] >> 4U) & 0x03U) == static_cast<uint8_t>(message_type::confirmable))
            {
                (void)send_reset(source, read_be16(buffer + 2U));
            }
            return result;
        }
        if (incoming.code_value == 0U && incoming.type == message_type::acknowledgement)
        {
            CASTLE_CONST size_type index = find_mid(source, incoming.message_id);
            if (index != MaxExchanges)
            {
                exchanges_[index].acknowledged = true;
                exchanges_[index].next_deadline = now_ms + exchange_lifetime_ms;
            }
            return castle::status::ok;
        }
        if (incoming.type == message_type::reset)
        {
            CASTLE_CONST size_type index = find_mid(source, incoming.message_id);
            if (index != MaxExchanges)
            {
                complete(index, castle::status::invalid_config, nullptr, source);
            }
            return castle::status::ok;
        }
        if (incoming.type == message_type::confirmable || incoming.type == message_type::non_confirmable)
        {
            if (is_recent_duplicate(source, incoming.message_id, now_ms))
            {
                if (incoming.type == message_type::confirmable)
                {
                    (void)send_control(source, message_type::acknowledgement, incoming.message_id);
                }
                return castle::status::ok;
            }
            remember_incoming(source, incoming.message_id, now_ms);
        }
        CASTLE_CONST size_type index = find_token(source, incoming);
        if (index == MaxExchanges)
        {
            if (incoming.type == message_type::confirmable)
            {
                (void)send_reset(source, incoming.message_id);
            }
            return castle::status::not_found;
        }
        exchange& entry = exchanges_[index];
        if (incoming.type == message_type::acknowledgement && entry.message_id != incoming.message_id)
        {
            return castle::status::not_found;
        }
        if (incoming.type == message_type::confirmable)
        {
            (void)send_control(source, message_type::acknowledgement, incoming.message_id);
        }
        CASTLE_CONST bool keep_observe = entry.observe && incoming.find_option(static_cast<uint16_t>(option_number::observe)) != nullptr;
        CASTLE_CONST exchange_handle handle{index, entry.generation};
        if (callback_)
        {
            dispatcher_.invoke(callback_, handle, castle::status::ok, &incoming, source);
        }
        if (!keep_observe)
        {
            entry.active = false;
            entry.wire_size = 0U;
        }
        return castle::status::ok;
    }

    /** @brief Drives retransmission deadlines without performing socket I/O. */
    castle::status tick(uint32_t now_ms) CASTLE_NOEXCEPT
    {
        return service_timers(now_ms);
    }

    size_type outstanding() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type count = 0U;
        for (size_type i = 0U; i < MaxExchanges; ++i)
        {
            if (exchanges_[i].active)
            {
                ++count;
            }
        }
        return count;
    }

private:
    struct exchange
    {
        castle::net::endpoint remote;
        uint16_t message_id = 0U;
        uint8_t token[maximum_token_length]{};
        size_type token_length = 0U;
        uint8_t wire[ReceiveBufferSize]{};
        size_type wire_size = 0U;
        uint32_t next_deadline = 0U;
        uint32_t timeout_ms = 0U;
        uint8_t retransmissions = 0U;
        uint32_t generation = 0U;
        bool acknowledged = false;
        bool confirmable = false;
        bool observe = false;
        bool active = false;
    };

    static uint32_t next_generation(uint32_t value) CASTLE_NOEXCEPT
    { ++value; return value == 0U ? 1U : value; }

    static bool due(uint32_t now, uint32_t deadline) CASTLE_NOEXCEPT
    { return static_cast<int32_t>(now - deadline) >= 0; }

    static bool has_observe_registration(message_type_t CASTLE_CONST& request) CASTLE_NOEXCEPT
    {
        CASTLE_CONST auto* value = request.find_option(static_cast<uint16_t>(option_number::observe));
        uint32_t decoded = 0U;
        return value != nullptr && castle::succeeded(decode_uint_option(value->value, value->length, decoded)) && decoded == 0U;
    }

    size_type find_free() CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxExchanges; ++i)
        {
            if (!exchanges_[i].active)
            {
                return i;
            }
        }
        return MaxExchanges;
    }

    size_type find_mid(castle::net::endpoint CASTLE_CONST& peer, uint16_t id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxExchanges; ++i)
        {
            if (exchanges_[i].active && exchanges_[i].remote == peer && exchanges_[i].message_id == id)
            {
                return i;
            }
        }
        return MaxExchanges;
    }

    size_type find_token(castle::net::endpoint CASTLE_CONST& peer, message_type_t CASTLE_CONST& incoming) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxExchanges; ++i)
        {
            CASTLE_CONST exchange& e = exchanges_[i];
            if (!e.active || e.remote != peer || e.token_length != incoming.token_length)
            {
                continue;
            }
            bool equal = true;
            for (size_type j = 0U; j < e.token_length; ++j)
            {
                if (e.token[j] != incoming.token[j])
                {
                    equal = false;
                    break;
                }
            }
            if (equal)
            {
                return i;
            }
        }
        return MaxExchanges;
    }

    exchange* find(exchange_handle handle) CASTLE_NOEXCEPT
    {
        if (!handle.is_valid() || handle.index >= MaxExchanges)
        {
            return nullptr;
        }
        exchange& entry = exchanges_[handle.index];
        return entry.active && entry.generation == handle.generation ? &entry : nullptr;
    }

    castle::status service_timers(uint32_t now_ms) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxExchanges; ++i)
        {
            exchange& entry = exchanges_[i];
            if (!entry.active || !due(now_ms, entry.next_deadline))
            {
                continue;
            }
            if (entry.acknowledged || !entry.confirmable || entry.retransmissions >= max_retransmissions_)
            {
                CASTLE_CONST exchange_handle handle{i, entry.generation};
                if (callback_)
                {
                    dispatcher_.invoke(callback_, handle, castle::status::time_out, nullptr, entry.remote);
                }
                entry.active = false;
                entry.wire_size = 0U;
                continue;
            }
            size_type sent = 0U;
            castle::status result = transport_.send_to(entry.wire, entry.wire_size, entry.remote, sent);
            if (!castle::succeeded(result))
            {
                return result;
            }
            if (sent != entry.wire_size)
            {
                return castle::status::data_loss;
            }
            ++entry.retransmissions;
            entry.timeout_ms = entry.timeout_ms > 0x7FFFFFFFU / 2U ? 0x7FFFFFFFU : entry.timeout_ms * 2U;
            entry.next_deadline = now_ms + entry.timeout_ms;
        }
        return castle::status::ok;
    }

    castle::status send_control(castle::net::endpoint CASTLE_CONST& peer, message_type control, uint16_t id) CASTLE_NOEXCEPT
    {
        message_type_t reset;
        reset.type = control;
        reset.code_value = 0U;
        reset.message_id = id;
        uint8_t wire[4U]{}; size_type size = 0U, sent = 0U;
        castle::status result = encode_message(reset, wire, sizeof(wire), size);
        if (castle::succeeded(result))
        {
            result = transport_.send_to(wire, size, peer, sent);
        }
        return result;
    }

    castle::status send_reset(castle::net::endpoint CASTLE_CONST& peer, uint16_t id) CASTLE_NOEXCEPT
    { return send_control(peer, message_type::reset, id); }

    void complete(size_type index, castle::status result, message_type_t CASTLE_CONST* value,
                  castle::net::endpoint CASTLE_CONST& peer) CASTLE_NOEXCEPT
    {
        exchange& entry = exchanges_[index];
        CASTLE_CONST exchange_handle handle{index, entry.generation};
        if (callback_)
        {
            dispatcher_.invoke(callback_, handle, result, value, peer);
        }
        entry.active = false;
        entry.wire_size = 0U;
    }

    void clear_exchanges() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxExchanges; ++i)
        {
            exchanges_[i].active = false;
            exchanges_[i].wire_size = 0U;
        }
        for (size_type i = 0U; i < MaxRecentMessages; ++i)
        {
            recent_[i].valid = false;
        }
    }

    struct recent_message
    {
        castle::net::endpoint peer;
        uint16_t message_id = 0U;
        uint32_t timestamp = 0U;
        bool valid = false;
    };

    bool is_recent_duplicate(castle::net::endpoint CASTLE_CONST& peer, uint16_t id, uint32_t now_ms) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxRecentMessages; ++i)
        {
            recent_message& item = recent_[i];
            if (item.valid && due(now_ms, item.timestamp + exchange_lifetime_ms))
            {
                item.valid = false;
            }
            if (item.valid && item.peer == peer && item.message_id == id)
            {
                return true;
            }
        }
        return false;
    }

    void remember_incoming(castle::net::endpoint CASTLE_CONST& peer, uint16_t id, uint32_t now_ms) CASTLE_NOEXCEPT
    {
        recent_message& item = recent_[recent_cursor_++ % MaxRecentMessages];
        item.peer = peer; item.message_id = id; item.timestamp = now_ms; item.valid = true;
    }

    transport_type transport_;
    exchange exchanges_[MaxExchanges]{};
    recent_message recent_[MaxRecentMessages]{};
    size_type recent_cursor_ = 0U;
    response_callback callback_;
    castle::callbacks::callback_dispatcher<response_callback> dispatcher_{};
    uint16_t next_message_id_ = 1U;
    uint32_t ack_timeout_ms_ = 2000U;
    uint8_t max_retransmissions_ = 4U;
    static CASTLE_CONSTEXPR uint32_t exchange_lifetime_ms = 247000U;
};

using client = basic_client<>;

} // namespace coap
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_COAP_CLIENT_HPP
