// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file server.hpp
 * @brief Fixed-capacity CoAP-over-UDP server with resource dispatch and duplicate suppression.
 */
#ifndef CASTLE_EXT_PROTOCOLS_COAP_SERVER_HPP
#define CASTLE_EXT_PROTOCOLS_COAP_SERVER_HPP

#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle_ext/net/server.hpp"
#include "castle_ext/protocols/coap/codec.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace coap
{

static CASTLE_CONSTEXPR uint32_t exchange_lifetime_ms = 247000U;
static CASTLE_CONSTEXPR uint32_t default_ack_timeout_ms = 2000U;

/** @brief Bit mask for the four core resource methods. */
enum method_mask : uint8_t
{
    allow_get = 0x01U,
    allow_post = 0x02U,
    allow_put = 0x04U,
    allow_delete = 0x08U,
    allow_all_methods = 0x0FU
};

/**
 * @brief Allocation-free resource server built on `castle::net::udp_server`.
 *
 * The server is polling based: call `process_once(now_ms, timeout_ms)` regularly. Resource
 * callbacks execute synchronously while the UDP server dispatches its receive callback and
 * must not recursively call `process_once()`. The resource and duplicate tables are fixed
 * at compile time. The server is not internally synchronized.
 */
template <size_type ReceiveBufferSize = 1152U,
          size_type MaxClients = 8U,
          size_type MaxResources = 12U,
          size_type MaxRecentMessages = 16U,
          size_type MaxServerExchanges = 4U,
          size_type MaxOptions = 16U,
          size_type OptionValueCapacity = 64U,
          size_type PayloadCapacity = 1024U,
          size_type MaxPathLength = 96U,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
class basic_server
{
    static_assert(ReceiveBufferSize >= 4U, "CoAP UDP buffer must hold the four-byte header");
    static_assert(MaxClients > 0U && MaxResources > 0U, "CoAP server capacities must be non-zero");
    static_assert(MaxRecentMessages > 0U, "CoAP duplicate cache must have at least one slot");
    static_assert(MaxPathLength > 1U, "CoAP resource path storage must hold a path");

public:
    using message_type_t = message<MaxOptions, OptionValueCapacity, PayloadCapacity>;
    using transport_type = castle::net::udp_server<MaxClients, ReceiveBufferSize>;
    using client_handle = typename transport_type::client_handle;
    using resource_callback = castle::callbacks::function<void(message_type_t CASTLE_CONST&, message_type_t&), CallbackStorageSize>;

    basic_server() CASTLE_DEFAULT;
    basic_server(basic_server CASTLE_CONST&) CASTLE_DELETE;
    basic_server& operator=(basic_server CASTLE_CONST&) CASTLE_DELETE;

    /** @brief Binds the UDP endpoint and installs the internal datagram dispatcher. */
    castle::status open(castle::net::endpoint CASTLE_CONST& local) CASTLE_NOEXCEPT
    {
        if (transport_.is_open())
        {
            return castle::status::invalid_config;
        }
        castle::status result = transport_.set_default_callback(
            [this](client_handle peer, castle::net::endpoint CASTLE_CONST& source,
                   uint8_t CASTLE_CONST* bytes, size_type size) { receive(peer, source, bytes, size); });
        if (!castle::succeeded(result))
        {
            return result;
        }
        return transport_.open(local);
    }

    castle::status close() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxServerExchanges; ++i)
        {
            pending_[i].active = false;
        }
        for (size_type i = 0U; i < MaxRecentMessages; ++i)
        {
            recent_[i].valid = false;
        }
        return transport_.close();
    }

    bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return transport_.is_open(); }
    size_type client_count() CASTLE_CONST CASTLE_NOEXCEPT { return transport_.client_count(); }
    size_type resource_count() CASTLE_CONST CASTLE_NOEXCEPT { return resource_count_; }
    transport_type& transport() CASTLE_NOEXCEPT { return transport_; }

    /** @brief Registers/replaces an exact hierarchical Uri-Path. */
    template <typename Callback>
    castle::status register_resource(char CASTLE_CONST* path, uint8_t methods, Callback&& callback)
    {
        if (path == nullptr || path[0] == '\0' || methods == 0U || (methods & ~allow_all_methods) != 0U)
        {
            return castle::status::invalid_argument;
        }
        resource_callback next(CASTLE_FORWARD<Callback>(callback));
        if (!next)
        {
            return castle::status::invalid_callback;
        }
        size_type length = 0U;
        while (path[length] != '\0' && length < MaxPathLength)
        {
            ++length;
        }
        if (length == 0U || length >= MaxPathLength)
        {
            return castle::status::out_of_range;
        }
        while (path[0] == '/')
        {
            ++path;
            --length;
        }
        if (length == 0U)
        {
            return castle::status::invalid_argument;
        }
        size_type slot = find_resource(path);
        if (slot == MaxResources)
        {
            for (slot = 0U; slot < MaxResources && resources_[slot].used; ++slot)
            {
            }
            if (slot == MaxResources)
            {
                return castle::status::full;
            }
            ++resource_count_;
        }
        resource& entry = resources_[slot];
        for (size_type i = 0U; i <= length; ++i)
        {
            entry.path[i] = i == length ? '\0' : path[i];
        }
        entry.path_length = length;
        entry.methods = methods;
        entry.callback = CASTLE_MOVE(next);
        entry.used = true;
        return castle::status::ok;
    }

    template <typename Callback>
    castle::status register_resource(char CASTLE_CONST* path, Callback&& callback)
    { return register_resource(path, allow_all_methods, CASTLE_FORWARD<Callback>(callback)); }

    castle::status unregister_resource(char CASTLE_CONST* path) CASTLE_NOEXCEPT
    {
        if (path == nullptr)
        {
            return castle::status::invalid_argument;
        }
        while (*path == '/')
        {
            ++path;
        }
        CASTLE_CONST size_type slot = find_resource(path);
        if (slot == MaxResources)
        {
            return castle::status::not_found;
        }
        resources_[slot].used = false;
        resources_[slot].callback = nullptr;
        if (resource_count_ > 0U)
        {
            --resource_count_;
        }
        return castle::status::ok;
    }

    /** @brief Sets a resource's response freshness lifetime (RFC 7252 Max-Age seconds). */
    castle::status set_resource_max_age(char CASTLE_CONST* path, uint32_t seconds) CASTLE_NOEXCEPT
    {
        if (path == nullptr)
        {
            return castle::status::invalid_argument;
        }
        while (*path == '/')
        {
            ++path;
        }
        CASTLE_CONST size_type slot = find_resource(path);
        if (slot == MaxResources)
        {
            return castle::status::not_found;
        }
        resources_[slot].max_age_seconds = seconds;
        return castle::status::ok;
    }

    /** @brief Enables a per-source fixed-window request limiter; rejected responses carry Max-Age. */
    castle::status configure_rate_limit(uint16_t maximum_requests, uint32_t window_ms) CASTLE_NOEXCEPT
    {
        if (maximum_requests == 0U || window_ms == 0U || window_ms > 0x7FFFFFFFU)
        {
            return castle::status::invalid_argument;
        }
        maximum_requests_per_window_ = maximum_requests;
        rate_limit_window_ms_ = window_ms;
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            rate_states_[i].valid = false;
        }
        return castle::status::ok;
    }

    /** @brief Disables request limiting without affecting registered resources. */
    void clear_rate_limit() CASTLE_NOEXCEPT
    {
        maximum_requests_per_window_ = 0U;
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            rate_states_[i].valid = false;
        }
    }

    /** @brief Receives one UDP datagram, expires duplicate entries, and drives CON retransmission. */
    castle::status process_once(uint32_t now_ms, int timeout_ms = 0) CASTLE_NOEXCEPT
    {
        now_ms_ = now_ms;
        CASTLE_CONST castle::status timer_result = tick(now_ms);
        if (!castle::succeeded(timer_result))
        {
            return timer_result;
        }
        expire_recent(now_ms);
        client_handle source;
        castle::status result = transport_.process_datagram(source, timeout_ms);
        return result;
    }

    /** @brief Drives outgoing separate CON response retransmissions without receiving. */
    castle::status tick(uint32_t now_ms) CASTLE_NOEXCEPT
    {
        now_ms_ = now_ms;
        for (size_type i = 0U; i < MaxServerExchanges; ++i)
        {
            pending_exchange& item = pending_[i];
            if (!item.active || !due(now_ms, item.deadline))
            {
                continue;
            }
            if (item.retries >= max_retransmissions_)
            {
                item.active = false;
                continue;
            }
            size_type sent = 0U;
            castle::status result = transport_.socket().send_to(item.wire, item.size, item.peer, sent);
            if (!castle::succeeded(result))
            {
                return result;
            }
            if (sent != item.size)
            {
                return castle::status::data_loss;
            }
            ++item.retries;
            item.timeout_ms = item.timeout_ms > 0x7FFFFFFFU / 2U ? 0x7FFFFFFFU : item.timeout_ms * 2U;
            item.deadline = now_ms + item.timeout_ms;
        }
        return castle::status::ok;
    }

    /** @brief Sends an empty ACK immediately before a manually generated separate response. */
    castle::status send_empty_ack(castle::net::endpoint CASTLE_CONST& peer, uint16_t message_id) CASTLE_NOEXCEPT
    {
        message_type_t ack;
        ack.type = message_type::acknowledgement;
        ack.code_value = 0U;
        ack.message_id = message_id;
        return send_to(peer, ack, nullptr, 0U);
    }

    /**
     * @brief Sends a separate response; CON responses are retained for retransmission until ACK/RST.
     * `response.code_value`, options and payload are supplied by the application; token is copied
     * from the request and the message ID is allocated by this server.
     */
    castle::status send_separate_response(castle::net::endpoint CASTLE_CONST& peer,
                                          message_type_t CASTLE_CONST& request,
                                          message_type_t response,
                                          uint32_t now_ms,
                                          bool confirmable = true) CASTLE_NOEXCEPT
    {
        if (!transport_.is_open())
        {
            return castle::status::not_configured;
        }
        if (!is_request_code(request.code_value) ||
            (request.type != message_type::confirmable && request.type != message_type::non_confirmable) ||
            request.token_length > maximum_token_length || !is_response_code(response.code_value))
        {
            return castle::status::invalid_argument;
        }

        size_type pending_slot = MaxServerExchanges;
        if (confirmable)
        {
            for (size_type i = 0U; i < MaxServerExchanges; ++i)
            {
                if (!pending_[i].active)
                {
                    pending_slot = i;
                    break;
                }
            }
            if (pending_slot == MaxServerExchanges)
            {
                return castle::status::full;
            }
        }

        // Do not reuse a Message ID that is still in an outstanding server CON exchange.
        uint16_t response_id = 0U;
        bool unique_id = false;
        for (size_type attempt = 0U; attempt <= MaxServerExchanges; ++attempt)
        {
            CASTLE_CONST uint16_t candidate = allocate_message_id();
            bool in_use = false;
            for (size_type i = 0U; i < MaxServerExchanges; ++i)
            {
                if (pending_[i].active && pending_[i].peer == peer && pending_[i].message_id == candidate)
                {
                    in_use = true;
                    break;
                }
            }
            if (!in_use)
            {
                response_id = candidate;
                unique_id = true;
                break;
            }
        }
        if (!unique_id)
        {
            return castle::status::full;
        }

        response.type = confirmable ? message_type::confirmable : message_type::non_confirmable;
        response.message_id = response_id;
        castle::status result = response.set_token(request.token, request.token_length);
        if (!castle::succeeded(result))
        {
            return result;
        }
        uint8_t wire[ReceiveBufferSize]{}; size_type size = 0U;
        result = encode_message(response, wire, sizeof(wire), size);
        if (!castle::succeeded(result))
        {
            return result;
        }

        // A response to a CON request is separate only after an empty ACK. A NON
        // request has no ACK exchange, so send the separate response directly.
        if (request.type == message_type::confirmable)
        {
            result = send_empty_ack(peer, request.message_id);
            if (!castle::succeeded(result))
            {
                return result;
            }
        }
        size_type sent = 0U;
        result = transport_.socket().send_to(wire, size, peer, sent);
        if (!castle::succeeded(result))
        {
            return result;
        }
        if (sent != size)
        {
            return castle::status::data_loss;
        }
        if (confirmable)
        {
            pending_exchange& item = pending_[pending_slot];
            item.peer = peer; item.message_id = response.message_id; item.size = size;
            for (size_type i = 0U; i < size; ++i)
            {
                item.wire[i] = wire[i];
            }
            item.retries = 0U; item.timeout_ms = ack_timeout_ms_; item.deadline = now_ms + ack_timeout_ms_;
            item.active = true;
        }
        return castle::status::ok;
    }

    void set_retransmission(uint32_t ack_timeout_ms, uint8_t max_retransmissions) CASTLE_NOEXCEPT
    {
        if (ack_timeout_ms != 0U && max_retransmissions <= 8U)
        {
            ack_timeout_ms_ = ack_timeout_ms;
            max_retransmissions_ = max_retransmissions;
        }
    }

    uint16_t allocate_message_id() CASTLE_NOEXCEPT { return next_message_id_++; }

private:
    struct resource
    {
        char path[MaxPathLength]{};
        size_type path_length = 0U;
        uint8_t methods = 0U;
        uint32_t max_age_seconds = 60U;
        resource_callback callback;
        bool used = false;
    };

    struct recent_entry
    {
        castle::net::endpoint peer;
        uint16_t message_id = 0U;
        uint32_t timestamp = 0U;
        uint8_t wire[ReceiveBufferSize]{};
        size_type size = 0U;
        bool valid = false;
    };

    struct pending_exchange
    {
        castle::net::endpoint peer;
        uint16_t message_id = 0U;
        uint8_t wire[ReceiveBufferSize]{};
        size_type size = 0U;
        uint32_t deadline = 0U;
        uint32_t timeout_ms = 0U;
        uint8_t retries = 0U;
        bool active = false;
    };

    struct rate_state
    {
        castle::net::endpoint peer;
        uint32_t window_start_ms = 0U;
        uint16_t request_count = 0U;
        bool valid = false;
    };

    static uint8_t method_bit(uint8_t request_code) CASTLE_NOEXCEPT
    {
        return request_code >= 1U && request_code <= 4U ? static_cast<uint8_t>(1U << (request_code - 1U)) : 0U;
    }

    static bool due(uint32_t now, uint32_t deadline) CASTLE_NOEXCEPT
    { return static_cast<int32_t>(now - deadline) >= 0; }

    size_type find_resource(char CASTLE_CONST* path) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (path == nullptr)
        {
            return MaxResources;
        }
        size_type length = 0U;
        while (path[length] != '\0')
        {
            ++length;
        }
        for (size_type i = 0U; i < MaxResources; ++i)
        {
            CASTLE_CONST resource& item = resources_[i];
            if (!item.used || item.path_length != length)
            {
                continue;
            }
            bool equal = true;
            for (size_type j = 0U; j < length; ++j)
            {
                if (item.path[j] != path[j])
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
        return MaxResources;
    }

    size_type find_resource(message_type_t CASTLE_CONST& request) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxResources; ++i)
        {
            if (resources_[i].used && uri_path_matches(request, resources_[i].path))
            {
                return i;
            }
        }
        return MaxResources;
    }

    size_type find_recent(castle::net::endpoint CASTLE_CONST& peer, uint16_t id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxRecentMessages; ++i)
        {
            if (recent_[i].valid && recent_[i].peer == peer && recent_[i].message_id == id)
            {
                return i;
            }
        }
        return MaxRecentMessages;
    }

    void expire_recent(uint32_t now_ms) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxRecentMessages; ++i)
        {
            if (recent_[i].valid && due(now_ms, recent_[i].timestamp + exchange_lifetime_ms))
            {
                recent_[i].valid = false;
            }
        }
    }

    bool is_rate_limited(castle::net::endpoint CASTLE_CONST& peer, uint32_t now_ms,
                         uint32_t& retry_after_ms) CASTLE_NOEXCEPT
    {
        retry_after_ms = 0U;
        if (maximum_requests_per_window_ == 0U)
        {
            return false;
        }
        size_type slot = MaxClients;
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            if (rate_states_[i].valid && rate_states_[i].peer == peer)
            {
                slot = i;
                break;
            }
            if (!rate_states_[i].valid && slot == MaxClients)
            {
                slot = i;
            }
        }
        if (slot == MaxClients)
        {
            // The peer table is also bounded. Reuse the oldest rate record if an old
            // peer has already been displaced by the underlying UDP server.
            slot = 0U;
            for (size_type i = 1U; i < MaxClients; ++i)
            {
                if (due(rate_states_[slot].window_start_ms, rate_states_[i].window_start_ms))
                {
                    slot = i;
                }
            }
        }
        rate_state& state = rate_states_[slot];
        if (!state.valid || state.peer != peer || due(now_ms, state.window_start_ms + rate_limit_window_ms_))
        {
            state.peer = peer;
            state.window_start_ms = now_ms;
            state.request_count = 0U;
            state.valid = true;
        }
        if (state.request_count >= maximum_requests_per_window_)
        {
            retry_after_ms = rate_limit_window_ms_ - (now_ms - state.window_start_ms);
            return true;
        }
        ++state.request_count;
        return false;
    }

    void cache_reply(castle::net::endpoint CASTLE_CONST& peer, uint16_t id, uint32_t now_ms,
                     uint8_t CASTLE_CONST* wire, size_type size) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type slot = recent_cursor_++ % MaxRecentMessages;
        recent_entry& item = recent_[slot];
        item.peer = peer; item.message_id = id; item.timestamp = now_ms; item.size = size;
        for (size_type i = 0U; i < size; ++i)
        {
            item.wire[i] = wire[i];
        }
        item.valid = true;
    }

    castle::status send_to(castle::net::endpoint CASTLE_CONST& peer, message_type_t CASTLE_CONST& value,
                           uint8_t* saved, size_type saved_capacity) CASTLE_NOEXCEPT
    {
        uint8_t wire[ReceiveBufferSize]{}; size_type size = 0U;
        castle::status result = encode_message(value, wire, sizeof(wire), size);
        if (!castle::succeeded(result))
        {
            return result;
        }
        size_type sent = 0U;
        result = transport_.socket().send_to(wire, size, peer, sent);
        if (!castle::succeeded(result))
        {
            return result;
        }
        if (sent != size)
        {
            return castle::status::data_loss;
        }
        if (saved != nullptr)
        {
            if (size > saved_capacity)
            {
                return castle::status::full;
            }
            for (size_type i = 0U; i < size; ++i)
            {
                saved[i] = wire[i];
            }
        }
        return castle::status::ok;
    }

    void receive(client_handle peer_handle, castle::net::endpoint CASTLE_CONST& source,
                 uint8_t CASTLE_CONST* bytes, size_type size) CASTLE_NOEXCEPT
    {
        if (bytes == nullptr || size < 4U)
        {
            return;
        }
        message_type_t request;
        decode_error reason = decode_error::none;
        castle::status result = decode_message(bytes, size, request, &reason);
        if (!castle::succeeded(result))
        {
            if (reason == decode_error::unknown_critical_option &&
                request.type == message_type::confirmable && is_request_code(request.code_value))
            {
                message_type_t bad_option;
                bad_option.type = message_type::acknowledgement;
                bad_option.message_id = request.message_id;
                bad_option.code_value = static_cast<uint8_t>(code::bad_option);
                (void)bad_option.set_token(request.token, request.token_length);
                (void)send_to(source, bad_option, nullptr, 0U);
            }
            else if (((bytes[0U] >> 4U) & 0x03U) == static_cast<uint8_t>(message_type::confirmable))
            {
                message_type_t reset;
                reset.type = message_type::reset;
                reset.message_id = read_u16(bytes + 2U);
                (void)send_to(source, reset, nullptr, 0U);
            }
            return;
        }
        if (request.type == message_type::acknowledgement || request.type == message_type::reset)
        {
            for (size_type i = 0U; i < MaxServerExchanges; ++i)
            {
                if (pending_[i].active && pending_[i].peer == source && pending_[i].message_id == request.message_id)
                {
                    pending_[i].active = false;
                }
            }
            return;
        }
        if (!is_request_code(request.code_value))
        {
            if (request.code_value == 0U)
            {
                return;
            }
            message_type_t reset;
            reset.type = message_type::reset;
            reset.message_id = request.message_id;
            (void)send_to(source, reset, nullptr, 0U);
            return;
        }
        CASTLE_CONST size_type duplicate = find_recent(source, request.message_id);
        if (duplicate != MaxRecentMessages)
        {
            CASTLE_CONST recent_entry& cached = recent_[duplicate];
            if (request.type == message_type::confirmable && cached.size != 0U)
            {
                size_type sent = 0U;
                (void)transport_.send_some(peer_handle, cached.wire, cached.size, sent);
            }
            return;
        }

        uint32_t retry_after_ms = 0U;
        if (is_rate_limited(source, now_ms_, retry_after_ms))
        {
            message_type_t limited;
            limited.type = request.type == message_type::confirmable ? message_type::acknowledgement : message_type::non_confirmable;
            limited.message_id = request.message_id;
            limited.code_value = static_cast<uint8_t>(code::service_unavailable);
            (void)limited.set_token(request.token, request.token_length);
            CASTLE_CONST uint32_t retry_after_seconds = (retry_after_ms + 999U) / 1000U;
            (void)limited.append_uint_option(static_cast<uint16_t>(option_number::max_age), retry_after_seconds);
            uint8_t saved[ReceiveBufferSize]{};
            CASTLE_CONST castle::status limited_result = send_to(source, limited, saved, sizeof(saved));
            if (castle::succeeded(limited_result))
            {
                cache_reply(source, request.message_id, now_ms_, saved, size_of_encoded(saved, limited));
            }
            return;
        }

        message_type_t response;
        response.type = request.type == message_type::confirmable ? message_type::acknowledgement : message_type::non_confirmable;
        response.message_id = request.message_id;
        response.code_value = static_cast<uint8_t>(code::content);
        (void)response.set_token(request.token, request.token_length);
        CASTLE_CONST size_type resource_index = find_resource(request);
        if (resource_index == MaxResources)
        {
            response.code_value = static_cast<uint8_t>(code::not_found);
        }
        else if ((resources_[resource_index].methods & method_bit(request.code_value)) == 0U)
        {
            response.code_value = static_cast<uint8_t>(code::method_not_allowed);
        }
        else
        {
            resources_[resource_index].callback(request, response);
            if (response.code_value == static_cast<uint8_t>(code::empty))
            {
                response.code_value = static_cast<uint8_t>(code::internal_server_error);
            }
            if (response.find_option(static_cast<uint16_t>(option_number::max_age)) == nullptr)
            {
                if (!castle::succeeded(response.append_uint_option(static_cast<uint16_t>(option_number::max_age),
                                                                     resources_[resource_index].max_age_seconds)))
                {
                    response.code_value = static_cast<uint8_t>(code::internal_server_error);
                }
            }
            response.type = request.type == message_type::confirmable ? message_type::acknowledgement : message_type::non_confirmable;
            response.message_id = request.message_id;
            (void)response.set_token(request.token, request.token_length);
        }
        uint8_t saved[ReceiveBufferSize]{};
        result = send_to(source, response, saved, sizeof(saved));
        if (castle::succeeded(result))
        {
            cache_reply(source, request.message_id, now_ms_, saved, size_of_encoded(saved, response));
        }
    }

    static size_type size_of_encoded(uint8_t CASTLE_CONST* wire, message_type_t CASTLE_CONST& response) CASTLE_NOEXCEPT
    {
        // Re-encode only to obtain the exact datagram length; the bounded response is deterministic.
        uint8_t scratch[ReceiveBufferSize]{}; size_type size = 0U;
        (void)encode_message(response, scratch, sizeof(scratch), size);
        (void)wire;
        return size;
    }

    transport_type transport_;
    resource resources_[MaxResources]{};
    recent_entry recent_[MaxRecentMessages]{};
    pending_exchange pending_[MaxServerExchanges]{};
    rate_state rate_states_[MaxClients]{};
    size_type resource_count_ = 0U;
    size_type recent_cursor_ = 0U;
    uint32_t now_ms_ = 0U;
    uint32_t ack_timeout_ms_ = default_ack_timeout_ms;
    uint8_t max_retransmissions_ = 4U;
    uint16_t next_message_id_ = 1U;
    uint16_t maximum_requests_per_window_ = 0U;
    uint32_t rate_limit_window_ms_ = 0U;
};

using server = basic_server<>;

} // namespace coap
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_COAP_SERVER_HPP
