// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file client.hpp
 * @brief Poll-driven, fixed-capacity MQTT client over CASTLE's TCP client socket.
 */
#ifndef CASTLE_EXT_PROTOCOLS_MQTT_CLIENT_HPP
#define CASTLE_EXT_PROTOCOLS_MQTT_CLIENT_HPP

#include "castle/callbacks/callback_dispatcher.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string_view.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/bytes.hpp"
#include "castle_ext/net/client.hpp"
#include "castle_ext/protocols/mqtt/codec.hpp"
#include "castle_ext/protocols/mqtt/topic.hpp"
#include "castle_ext/protocols/mqtt/types.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace mqtt
{

/**
 * @brief Allocation-free MQTT client. All network progress is driven by `poll()`.
 *
 * @tparam ReceiveBufferSize CASTLE TCP receive chunk size.
 * @tparam MaxPacketSize Maximum encoded MQTT control packet size, including the fixed header.
 * @tparam MaxOutstanding Outstanding outgoing and incoming QoS exchanges.
 * @tparam MaxTopicLength Maximum topic/filter/client-ID/username/password bytes plus terminator.
 * @tparam MaxPayloadSize Maximum copied payload size retained for QoS 1/2 retransmission.
 * @tparam MaxProperties Maximum MQTT 5 properties in one property list.
 *
 * The application callback runs synchronously from `poll()`. Message views and their backing bytes are
 * valid only for the duration of that callback. The client is not internally synchronized.
 */
template <size_type ReceiveBufferSize = 512U,
          size_type MaxPacketSize = 1024U,
          size_type MaxOutstanding = 8U,
          size_type MaxTopicLength = 128U,
          size_type MaxPayloadSize = 512U,
          size_type MaxProperties = 8U,
          size_type PropertyStringCapacity = 64U,
          size_type PropertyBinaryCapacity = 32U>
class basic_client
{
    static_assert(ReceiveBufferSize > 0U && MaxPacketSize >= 8U, "MQTT client buffers are too small");
    static_assert(MaxOutstanding > 0U && MaxTopicLength > 1U, "MQTT capacities must be non-zero");

public:
    using transport_type = castle::net::tcp_client<ReceiveBufferSize>;
    using message_callback = castle::callbacks::function<void(message_view CASTLE_CONST&), castle::inplace_storage_reserved>;
    using properties_type = property_list<MaxProperties, PropertyStringCapacity, PropertyBinaryCapacity>;

    /** @brief Constructs an unopened client and installs its fixed receive trampoline. */
    basic_client() CASTLE_NOEXCEPT
    {
        (void)transport_.set_receive_callback([this](castle::net::endpoint CASTLE_CONST&, uint8_t CASTLE_CONST* data, size_type size)
        {
            receive_bytes(data, size);
        });
    }

    basic_client(basic_client CASTLE_CONST&) CASTLE_DELETE;
    basic_client& operator=(basic_client CASTLE_CONST&) CASTLE_DELETE;

    /** @brief Replaces the synchronous message-arrived callback. */
    template <typename Callback>
    castle::status set_message_callback(Callback&& callback)
    {
        message_callback next(CASTLE_FORWARD<Callback>(callback));
        if (!next)
        {
            return castle::status::invalid_callback;
        }
        dispatcher_.replace(message_callback_, CASTLE_MOVE(next));
        return castle::status::ok;
    }

    /** @brief Clears the message-arrived callback. */
    void clear_message_callback() CASTLE_NOEXCEPT { dispatcher_.clear(message_callback_); }

    /**
     * @brief Opens TCP, sends CONNECT and waits for CONNACK.
     * @param remote Numeric broker endpoint (usually port 1883).
     * @param options CONNECT options. Borrowed input fields are copied into bounded client storage.
     * @param timeout_ms Blocking TCP connect and CONNACK wait limit.
     * @return `ok` after successful CONNACK, or a CASTLE status on failure/timeout.
     */
    castle::status connect(castle::net::endpoint CASTLE_CONST& remote,
                           connect_options<MaxProperties, PropertyStringCapacity, PropertyBinaryCapacity> CASTLE_CONST& options,
                           int timeout_ms = 1000) CASTLE_NOEXCEPT
    {
        if (transport_.is_open())
        {
            return castle::status::invalid_config;
        }
        castle::status result = validate_connect_options(options);
        if (!castle::succeeded(result))
        {
            return last_error_ = result;
        }
        options_ = options;
        if (options.clean_session)
        {
            clear_outgoing();
            clear_incoming();
            clear_requests();
        }
        rx_size_ = 0U;
        fatal_ = false;
        connecting_ = true;
        connected_ = false;
        clock_started_ = false;
        last_error_ = castle::status::ok;
        result = transport_.connect(remote, timeout_ms, false);
        if (!castle::succeeded(result))
        {
            connecting_ = false;
            (void)transport_.close(nullptr);
            return last_error_ = result;
        }
        result = send_connect();
        if (!castle::succeeded(result))
        {
            force_close();
            return last_error_ = result;
        }

        // TCP is configured blocking for predictable send semantics. A CONNACK may span receive chunks.
        for (size_type attempts = 0U; attempts < MaxPacketSize && connecting_; ++attempts) // LCOV_EXCL_BR_LINE
        {
            result = transport_.process_once(timeout_ms);
            if (fatal_)
            {
                force_close();
                return last_error_;
            }
            if (result == castle::status::time_out || result == castle::status::empty) // LCOV_EXCL_BR_LINE
            {
                force_close();
                return last_error_ = castle::status::time_out;
            }
            if (!castle::succeeded(result))
            {
                force_close();
                return last_error_ = result;
            }
        }
        // LCOV_EXCL_START
        // can not cover branches code
        if (!connected_)
        {
            force_close();
            return last_error_ = castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        if (session_present_ && !options.clean_session)
        {
            resend_outgoing();
        }
        else
        {
            clear_outgoing();
            clear_incoming();
        }
        return castle::status::ok;
    }

    /** @brief Sends a PUBLISH packet at the selected QoS. QoS 1/2 reserve an identifier and bounded retry slot. */
    castle::status publish(castle::container::string_view topic, uint8_t CASTLE_CONST* payload,
                           size_type payload_size, qos quality = qos::at_most_once,
                           bool retain = false) CASTLE_NOEXCEPT
    {
        if (!connected_)
        {
            return castle::status::not_configured;
        }
        if (!castle::succeeded(validate_topic_name(topic)) || (payload == nullptr && payload_size != 0U))
        {
            return castle::status::invalid_argument;
        }
        if (topic.size() >= MaxTopicLength || payload_size > MaxPayloadSize ||
            static_cast<uint8_t>(quality) > 2U)
        {
            return castle::status::out_of_range;
        }
        uint16_t identifier = 0U;
        outgoing_entry* entry = nullptr;
        if (quality != qos::at_most_once)
        {
            entry = allocate_outgoing(identifier);
            if (entry == nullptr)
            {
                return castle::status::full;
            }
            entry->used = true;
            entry->identifier = identifier;
            entry->quality = quality;
            entry->state = quality == qos::at_least_once ? outgoing_state::await_puback : outgoing_state::await_pubrec;
            copy_text(entry->topic, topic);
            entry->payload_size = payload_size;
            for (size_type i = 0U; i < payload_size; ++i)
            {
                entry->payload[i] = payload[i];
            }
            entry->retain = retain;
        }
        castle::status result = send_publish(topic, payload, payload_size, quality, retain, identifier, false);
        if (!castle::succeeded(result) && entry != nullptr)
        {
            entry->used = false;
        }
        return result;
    }

    /** @brief Convenience overload for a string-literal payload. */
    castle::status publish(castle::container::string_view topic, castle::container::string_view payload,
                           qos quality = qos::at_most_once, bool retain = false) CASTLE_NOEXCEPT
    {
        return publish(topic, reinterpret_cast<uint8_t CASTLE_CONST*>(payload.data()), payload.size(), quality, retain);
    }

    /** @brief Subscribes to one Topic Filter and returns its Packet Identifier. */
    castle::status subscribe(castle::container::string_view filter, qos maximum_quality,
                             uint16_t& packet_identifier) CASTLE_NOEXCEPT
    {
        castle::container::string_view filters[1U] = {filter};
        qos qualities[1U] = {maximum_quality};
        return subscribe(filters, qualities, 1U, packet_identifier);
    }

    /** @brief Subscribes to one or more filters in a single MQTT SUBSCRIBE packet. */
    castle::status subscribe(castle::container::string_view CASTLE_CONST* filters, qos CASTLE_CONST* qualities,
                             size_type count, uint16_t& packet_identifier) CASTLE_NOEXCEPT
    {
        packet_identifier = 0U;
        if (!connected_)
        {
            return castle::status::not_configured;
        }
        if (filters == nullptr || qualities == nullptr || count == 0U)
        {
            return castle::status::invalid_argument;
        }
        uint16_t id = allocate_identifier();
        if (id == 0U) // LCOV_EXCL_BR_LINE
        {
            return castle::status::full;
        }
        packet_writer body(body_, MaxPacketSize);
        (void)body.put_u16(id);
        if (options_.protocol_version == version::v5_0)
        {
            castle::status status = append_empty_properties(body);
            if (!castle::succeeded(status)) // LCOV_EXCL_BR_LINE
            {
                return status;
            }
        }
        for (size_type i = 0U; i < count; ++i)
        {
            if (!castle::succeeded(validate_topic_filter(filters[i])) || static_cast<uint8_t>(qualities[i]) > 2U)
            {
                return castle::status::invalid_argument;
            }
            (void)body.put_utf8(filters[i]);
            (void)body.put_u8(static_cast<uint8_t>(qualities[i]));
        }
        if (!castle::succeeded(body.status()))
        {
            return body.status();
        }
        if (!reserve_request(id, packet_type::subscribe))
        {
            return castle::status::full;
        }
        castle::status result = send_body(packet_type::subscribe, 0x02U, body.data(), body.size());
        if (castle::succeeded(result))
        {
            packet_identifier = id;
        }
        else
        {
            pending_request* request = find_request(id, packet_type::subscribe);
            if (request != nullptr) // LCOV_EXCL_BR_LINE
            {
                request->used = false;
            }
        }
        return result;
    }

    /** @brief Unsubscribes from one Topic Filter. */
    castle::status unsubscribe(castle::container::string_view filter, uint16_t& packet_identifier) CASTLE_NOEXCEPT
    {
        castle::container::string_view filters[1U] = {filter};
        return unsubscribe(filters, 1U, packet_identifier);
    }

    /** @brief Unsubscribes from one or more filters in a single MQTT UNSUBSCRIBE packet. */
    castle::status unsubscribe(castle::container::string_view CASTLE_CONST* filters, size_type count,
                               uint16_t& packet_identifier) CASTLE_NOEXCEPT
    {
        packet_identifier = 0U;
        if (!connected_)
        {
            return castle::status::not_configured;
        }
        if (filters == nullptr || count == 0U)
        {
            return castle::status::invalid_argument;
        }
        uint16_t id = allocate_identifier();
        if (id == 0U) // LCOV_EXCL_BR_LINE
        {
            return castle::status::full;
        }
        packet_writer body(body_, MaxPacketSize);
        (void)body.put_u16(id);
        if (options_.protocol_version == version::v5_0)
        {
            castle::status status = append_empty_properties(body);
            if (!castle::succeeded(status)) // LCOV_EXCL_BR_LINE
            {
                return status;
            }
        }
        for (size_type i = 0U; i < count; ++i)
        {
            if (!castle::succeeded(validate_topic_filter(filters[i])))
            {
                return castle::status::invalid_argument;
            }
            (void)body.put_utf8(filters[i]);
        }
        if (!castle::succeeded(body.status()))
        {
            return body.status();
        }
        if (!reserve_request(id, packet_type::unsubscribe))
        {
            return castle::status::full;
        }
        castle::status result = send_body(packet_type::unsubscribe, 0x02U, body.data(), body.size());
        if (castle::succeeded(result))
        {
            packet_identifier = id;
        }
        else
        {
            pending_request* request = find_request(id, packet_type::unsubscribe);
            if (request != nullptr) // LCOV_EXCL_BR_LINE
            {
                request->used = false;
            }
        }
        return result;
    }

    /**
     * @brief Processes at most one TCP receive event and services keep-alive deadlines.
     * @param now_ms Monotonic milliseconds supplied by the application.
     * @param timeout_ms Socket readiness wait. Zero performs a non-blocking poll.
     */
    castle::status poll(uint32_t now_ms, int timeout_ms = 0) CASTLE_NOEXCEPT
    {
        if (!transport_.is_open())
        {
            return castle::status::not_configured;
        }
        now_ms_ = now_ms;
        if (!clock_started_)
        {
            last_tx_ms_ = now_ms_;
            last_rx_ms_ = now_ms_;
            clock_started_ = true;
        }
        castle::status result = transport_.process_once(timeout_ms);
        if (fatal_)
        {
            castle::status error = last_error_;
            force_close();
            return error;
        }
        if (result == castle::status::not_configured)
        {
            connected_ = false;
            return result;
        }
        if (result != castle::status::ok && result != castle::status::empty && result != castle::status::time_out) // LCOV_EXCL_BR_LINE
        {
            last_error_ = result;
            return result;
        }
        if (connected_ && options_.keep_alive_seconds != 0U) // LCOV_EXCL_BR_LINE
        {
            uint32_t keep_alive_ms = static_cast<uint32_t>(options_.keep_alive_seconds) * 1000U;
            if (!ping_outstanding_ && elapsed(now_ms_, last_tx_ms_) >= keep_alive_ms)
            {
                uint8_t empty[1U]{};
                castle::status ping = send_body(packet_type::pingreq, 0U, empty, 0U);
                if (!castle::succeeded(ping)) // LCOV_EXCL_BR_LINE
                {
                    return ping;
                }
                ping_outstanding_ = true;
                ping_sent_ms_ = now_ms_;
            }
            else if (ping_outstanding_ && elapsed(now_ms_, ping_sent_ms_) > keep_alive_ms)
            {
                force_close();
                return last_error_ = castle::status::time_out;
            }
        }
        return result == castle::status::time_out ? castle::status::empty : result;
    }

    /** @brief Sends DISCONNECT and closes the TCP connection. */
    castle::status disconnect(reason_code reason = reason_code::normal_disconnection) CASTLE_NOEXCEPT
    {
        if (!transport_.is_open())
        {
            return castle::status::not_configured;
        }
        castle::status result = castle::status::ok;
        if (connected_) // LCOV_EXCL_BR_LINE
        {
            packet_writer body(body_, MaxPacketSize);
            if (options_.protocol_version == version::v5_0 && reason != reason_code::normal_disconnection)
            {
                (void)body.put_u8(static_cast<uint8_t>(reason));
                (void)append_empty_properties(body);
            }
            result = send_body(packet_type::disconnect, 0U, body.data(), body.size());
        }
        force_close();
        return result;
    }

    /** @brief Immediately closes TCP without sending MQTT DISCONNECT. */
    castle::status force_close() CASTLE_NOEXCEPT
    {
        connected_ = false;
        connecting_ = false;
        clock_started_ = false;
        ping_outstanding_ = false;
        rx_size_ = 0U;
        return transport_.close(nullptr);
    }

    bool is_connected() CASTLE_CONST CASTLE_NOEXCEPT { return connected_; }
    bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return transport_.is_open(); }
    bool session_present() CASTLE_CONST CASTLE_NOEXCEPT { return session_present_; }
    castle::status last_error() CASTLE_CONST CASTLE_NOEXCEPT { return last_error_; }
    uint16_t last_acknowledged_packet_id() CASTLE_CONST CASTLE_NOEXCEPT { return last_acknowledged_id_; }
    transport_type& transport() CASTLE_NOEXCEPT { return transport_; }

private:
    enum class outgoing_state : uint8_t { await_puback, await_pubrec, await_pubcomp };
    struct outgoing_entry
    {
        bool used = false;
        uint16_t identifier = 0U;
        qos quality = qos::at_least_once;
        outgoing_state state = outgoing_state::await_puback;
        char topic[MaxTopicLength]{};
        uint8_t payload[MaxPayloadSize]{};
        size_type payload_size = 0U;
        bool retain = false;
    };
    struct pending_request
    {
        bool used = false;
        uint16_t identifier = 0U;
        packet_type type = packet_type::reserved;
    };
    struct incoming_entry
    {
        bool used = false;
        uint16_t identifier = 0U;
        char topic[MaxTopicLength]{};
        uint8_t payload[MaxPayloadSize]{};
        size_type payload_size = 0U;
        bool retain = false;
    };

    static uint32_t elapsed(uint32_t now, uint32_t then) CASTLE_NOEXCEPT { return now - then; }
    static void copy_text(char* target, castle::container::string_view text) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < text.size(); ++i)
        {
            target[i] = text[i];
        }
        target[text.size()] = '\0';
    }

    castle::status validate_connect_options(connect_options<MaxProperties, PropertyStringCapacity, PropertyBinaryCapacity> CASTLE_CONST& options) CASTLE_NOEXCEPT
    {
        if (options.protocol_version != version::v3_1_0 && options.protocol_version != version::v3_1_1 && options.protocol_version != version::v5_0)
        {
            return castle::status::invalid_argument;
        }
        if (options.client_id.empty() || options.client_id.size() >= MaxTopicLength || !detail::valid_utf8(options.client_id))
        {
            return castle::status::invalid_argument;
        }
        if ((options.has_password && !options.has_username) ||
            (options.has_username && (options.username.size() >= MaxTopicLength || !detail::valid_utf8(options.username))) ||
            (options.has_password && (options.password.size() >= MaxTopicLength || !detail::valid_utf8(options.password))))
        {
            return castle::status::invalid_argument;
        }
        if (options.has_will && (!castle::succeeded(validate_topic_name(options.will.topic)) ||
            static_cast<uint8_t>(options.will.quality) > 2U || options.will.payload_size > MaxPayloadSize ||
            (options.will.payload == nullptr && options.will.payload_size != 0U)))
        {
            return castle::status::invalid_argument;
        }
        return castle::status::ok;
    }

    castle::status send_connect() CASTLE_NOEXCEPT
    {
        packet_writer body(body_, MaxPacketSize);
        castle::container::string_view protocol_name = options_.protocol_version == version::v3_1_0
            ? castle::container::string_view("MQIsdp") : castle::container::string_view("MQTT");
        (void)body.put_utf8(protocol_name);
        (void)body.put_u8(static_cast<uint8_t>(options_.protocol_version));
        uint8_t flags = options_.clean_session ? 0x02U : 0U;
        if (options_.has_will)
        {
            flags |= 0x04U;
            flags |= static_cast<uint8_t>(static_cast<uint8_t>(options_.will.quality) << 3U);
            if (options_.will.retain)
            {
                flags |= 0x20U;
            }
        }
        if (options_.has_username)
        {
            flags |= 0x80U;
        }
        if (options_.has_password)
        {
            flags |= 0x40U;
        }
        (void)body.put_u8(flags);
        (void)body.put_u16(options_.keep_alive_seconds);
        if (options_.protocol_version == version::v5_0)
        {
            castle::status status = append_properties(body, options_.properties);
            if (!castle::succeeded(status))
            {
                return status;
            }
        }
        (void)body.put_utf8(options_.client_id);
        if (options_.has_will)
        {
            if (options_.protocol_version == version::v5_0)
            {
                castle::status status = append_properties(body, options_.will_properties);
                if (!castle::succeeded(status))
                {
                    return status;
                }
            }
            (void)body.put_utf8(options_.will.topic);
            (void)body.put_binary(options_.will.payload, options_.will.payload_size);
        }
        if (options_.has_username)
        {
            (void)body.put_utf8(options_.username);
        }
        if (options_.has_password)
        {
            (void)body.put_binary(reinterpret_cast<uint8_t CASTLE_CONST*>(options_.password.data()), options_.password.size());
        }
        if (!castle::succeeded(body.status()))
        {
            return body.status();
        }
        return send_body(packet_type::connect, 0U, body.data(), body.size());
    }

    template <size_type Count, size_type StrCap, size_type BinCap>
    castle::status append_properties(packet_writer& body, property_list<Count, StrCap, BinCap> CASTLE_CONST& properties) CASTLE_NOEXCEPT
    {
        uint8_t encoded[MaxPacketSize]{}; size_type used = 0U;
        castle::status result = encode_properties(properties, encoded, sizeof(encoded), used);
        return castle::succeeded(result) ? body.put_bytes(encoded, used) : result;
    }

    castle::status append_empty_properties(packet_writer& body) CASTLE_NOEXCEPT { return body.put_u8(0U); }

    castle::status send_body(packet_type type, uint8_t flags, uint8_t CASTLE_CONST* body, size_type body_size) CASTLE_NOEXCEPT
    {
        size_type written = 0U;
        castle::status result = encode_packet(type, flags, body, body_size, tx_, sizeof(tx_), written);
        if (!castle::succeeded(result))
        {
            return result;
        }
        size_type sent = 0U;
        result = transport_.send_all(tx_, written, sent, nullptr);
        if (castle::succeeded(result) && sent == written) // LCOV_EXCL_BR_LINE
        {
            last_tx_ms_ = now_ms_;
        }
        else if (castle::succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            result = castle::status::data_loss;
        }
        if (!castle::succeeded(result))
        {
            last_error_ = result;
        }
        return result;
    }

    castle::status send_publish(castle::container::string_view topic, uint8_t CASTLE_CONST* payload, size_type payload_size,
                                qos quality, bool retain, uint16_t identifier, bool duplicate) CASTLE_NOEXCEPT
    {
        packet_writer body(body_, MaxPacketSize);
        (void)body.put_utf8(topic);
        if (quality != qos::at_most_once)
        {
            (void)body.put_u16(identifier);
        }
        if (options_.protocol_version == version::v5_0)
        {
            (void)append_empty_properties(body);
        }
        (void)body.put_bytes(payload, payload_size);
        if (!castle::succeeded(body.status()))
        {
            return body.status();
        }
        uint8_t flags = static_cast<uint8_t>(static_cast<uint8_t>(quality) << 1U);
        if (retain)
        {
            flags |= 0x01U;
        }
        if (duplicate)
        {
            flags |= 0x08U;
        }
        return send_body(packet_type::publish, flags, body.data(), body.size());
    }

    outgoing_entry* allocate_outgoing(uint16_t& identifier) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            if (!outgoing_[i].used)
            {
                identifier = allocate_identifier();
                return identifier == 0U ? nullptr : &outgoing_[i]; // LCOV_EXCL_BR_LINE
            }
        }
        return nullptr;
    }

    bool reserve_request(uint16_t identifier, packet_type type) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            if (!requests_[i].used)
            {
                requests_[i].used = true;
                requests_[i].identifier = identifier;
                requests_[i].type = type;
                return true;
            }
        }
        return false;
    }
    pending_request* find_request(uint16_t identifier, packet_type type) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            if (requests_[i].used && requests_[i].identifier == identifier && requests_[i].type == type)
            {
                return &requests_[i];
            }
        }
        return nullptr;
    }
    void clear_requests() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            requests_[i].used = false;
        }
    }

    uint16_t allocate_identifier() CASTLE_NOEXCEPT
    {
        for (size_type attempt = 0U; attempt < 65535U; ++attempt) // LCOV_EXCL_BR_LINE
        {
            ++next_identifier_;
            if (next_identifier_ == 0U)
            {
                ++next_identifier_;
            }
            bool used = false;
            for (size_type i = 0U; i < MaxOutstanding; ++i)
            {
                if ((outgoing_[i].used && outgoing_[i].identifier == next_identifier_) ||
                    (requests_[i].used && requests_[i].identifier == next_identifier_))
                {
                    used = true;
                    break;
                }
            }
            if (!used)
            {
                return next_identifier_;
            }
        }
        // LCOV_EXCL_START
        // can not cover branches code
        return 0U;
        // LCOV_EXCL_STOP
    }

    outgoing_entry* find_outgoing(uint16_t id) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            if (outgoing_[i].used && outgoing_[i].identifier == id)
            {
                return &outgoing_[i];
            }
        }
        return nullptr;
    }
    incoming_entry* find_incoming(uint16_t id) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            if (incoming_[i].used && incoming_[i].identifier == id)
            {
                return &incoming_[i];
            }
        }
        return nullptr;
    }
    incoming_entry* allocate_incoming() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            if (!incoming_[i].used)
            {
                return &incoming_[i];
            }
        }
        return nullptr;
    }

    void clear_outgoing() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            outgoing_[i].used = false;
        }
    }
    void clear_incoming() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            incoming_[i].used = false;
        }
    }

    void resend_outgoing() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            outgoing_entry& entry = outgoing_[i];
            if (!entry.used)
            {
                continue;
            }
            if (entry.state == outgoing_state::await_pubcomp)
            {
                uint8_t body[2U]{};
                write_be16(body, entry.identifier);
                if (options_.protocol_version == version::v5_0)
                {
                    uint8_t extended[3U] = {body[0U], body[1U], 0U};
                    (void)send_body(packet_type::pubrel, 0x0AU, extended, 3U);
                }
                else
                {
                    (void)send_body(packet_type::pubrel, 0x0AU, body, 2U);
                }
            }
            else
            {
                (void)send_publish(castle::container::string_view(entry.topic), entry.payload, entry.payload_size,
                                   entry.quality, entry.retain, entry.identifier, true);
            }
        }
    }

    void receive_bytes(uint8_t CASTLE_CONST* data, size_type size) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        // can not cover branches code
        if (fatal_ || data == nullptr || size > (MaxPacketSize + ReceiveBufferSize) - rx_size_)
        {
            fatal_ = true;
            last_error_ = castle::status::out_of_range;
            return;
        }
        // LCOV_EXCL_STOP
        for (size_type i = 0U; i < size; ++i)
        {
            rx_[rx_size_++] = data[i];
        }
        size_type consumed = 0U;
        while (consumed < rx_size_)
        {
            packet_view packet{};
            castle::status status = inspect_packet(rx_ + consumed, rx_size_ - consumed, MaxPacketSize, packet);
            if (status == castle::status::empty)
            {
                break;
            }
            if (!castle::succeeded(status))
            {
                fatal_ = true;
                last_error_ = status;
                return;
            }
            status = handle_packet(packet);
            if (castle::succeeded(status))
            {
                last_rx_ms_ = now_ms_;
            }
            if (!castle::succeeded(status))
            {
                fatal_ = true;
                last_error_ = status;
                return;
            }
            consumed += packet.packet_size;
        }
        if (consumed != 0U)
        {
            size_type remaining = rx_size_ - consumed;
            for (size_type i = 0U; i < remaining; ++i)
            {
                rx_[i] = rx_[consumed + i];
            }
            rx_size_ = remaining;
        }
    }

    castle::status handle_packet(packet_view CASTLE_CONST& packet) CASTLE_NOEXCEPT
    {
        packet_reader reader(packet.body, packet.remaining_length);
        switch (packet.type)
        {
            case packet_type::connack: return handle_connack(reader);
            case packet_type::publish: return handle_publish(packet, reader);
            case packet_type::puback: return handle_ack(reader, outgoing_state::await_puback, packet_type::puback);
            case packet_type::pubrec: return handle_ack(reader, outgoing_state::await_pubrec, packet_type::pubrec);
            case packet_type::pubcomp: return handle_ack(reader, outgoing_state::await_pubcomp, packet_type::pubcomp);
            case packet_type::pubrel: return handle_pubrel(reader);
            case packet_type::suback: return handle_suback(reader);
            case packet_type::unsuback: return handle_unsuback(reader);
            case packet_type::pingresp:
                if (packet.remaining_length != 0U || !ping_outstanding_)
                {
                    return castle::status::invalid_argument;
                }
                ping_outstanding_ = false;
                return castle::status::ok;
            case packet_type::disconnect:
                if (!connected_)
                {
                    return castle::status::invalid_argument;
                }
                if (options_.protocol_version == version::v5_0 && reader.remaining() != 0U)
                {
                    uint8_t reason = 0U;
                    if (!castle::succeeded(reader.get_u8(reason))) // LCOV_EXCL_BR_LINE
                    {
                        return reader.status();
                    }
                    properties_type properties;
                    if (reader.remaining() != 0U && !castle::succeeded(decode_properties(reader, properties, true))) // LCOV_EXCL_BR_LINE
                    {
                        return reader.status();
                    }
                }
                connected_ = false;
                (void)transport_.close(nullptr);
                return castle::status::not_configured;
            case packet_type::auth: return castle::status::invalid_config;
            default: return castle::status::invalid_argument;
        }
    }

    castle::status handle_connack(packet_reader& reader) CASTLE_NOEXCEPT
    {
        if (!connecting_ || reader.remaining() < 2U)
        {
            return castle::status::invalid_argument;
        }
        uint8_t flags = 0U; uint8_t code = 0U;
        (void)reader.get_u8(flags); (void)reader.get_u8(code);
        if ((flags & 0xFEU) != 0U)
        {
            return castle::status::invalid_argument;
        }
        session_present_ = (flags & 0x01U) != 0U;
        if (options_.protocol_version == version::v5_0)
        {
            if (reader.remaining() == 0U)
            {
                return castle::status::invalid_argument;
            }
            properties_type properties;
            castle::status status = decode_properties(reader, properties, true);
            if (!castle::succeeded(status))
            {
                return status;
            }
        }
        else if (reader.remaining() != 0U || code > 5U)
        {
            return castle::status::invalid_argument;
        }
        connecting_ = false;
        if (code != 0U)
        {
            if (options_.protocol_version == version::v5_0)
            {
                return to_status(static_cast<reason_code>(code));
            }
            switch (code) // LCOV_EXCL_BR_LINE
            {
                case 1U: return castle::status::invalid_config;
                case 2U: return castle::status::invalid_argument;
                case 3U: return castle::status::time_out;
                case 4U: case 5U: return castle::status::not_configured;
                // LCOV_EXCL_START
                // can not cover branches code
                default: return castle::status::invalid_argument;
                // LCOV_EXCL_STOP
            }
        }
        connected_ = true;
        last_rx_ms_ = now_ms_;
        return castle::status::ok;
    }

    castle::status handle_publish(packet_view CASTLE_CONST& packet, packet_reader& reader) CASTLE_NOEXCEPT
    {
        uint8_t flags = packet.flags;
        qos quality = static_cast<qos>((flags >> 1U) & 0x03U);
        if (static_cast<uint8_t>(quality) > 2U) // LCOV_EXCL_BR_LINE
        {
            return castle::status::invalid_argument;
        }
        char topic[MaxTopicLength]{}; size_type topic_size = 0U;
        castle::status status = reader.get_utf8(topic, sizeof(topic), topic_size);
        if (!castle::succeeded(status) || !castle::succeeded(validate_topic_name(castle::container::string_view(topic, topic_size))))
        {
            return castle::status::invalid_argument;
        }
        uint16_t identifier = 0U;
        if (quality != qos::at_most_once)
        {
            if (!castle::succeeded(reader.get_u16(identifier)) || identifier == 0U)
            {
                return castle::status::invalid_argument;
            }
        }
        if (options_.protocol_version == version::v5_0)
        {
            properties_type properties;
            status = decode_properties(reader, properties, true);
            if (!castle::succeeded(status))
            {
                return status;
            }
        }
        size_type payload_size = reader.remaining();
        if (payload_size > MaxPayloadSize)
        {
            return castle::status::out_of_range;
        }
        message_view message;
        message.topic = castle::container::string_view(topic, topic_size);
        message.payload = reader.current();
        message.payload_size = payload_size;
        message.quality = quality;
        message.duplicate = (flags & 0x08U) != 0U;
        message.retain = (flags & 0x01U) != 0U;
        message.packet_identifier = identifier;
        if (quality == qos::exactly_once)
        {
            incoming_entry* entry = find_incoming(identifier);
            if (entry == nullptr)
            {
                entry = allocate_incoming();
                if (entry == nullptr)
                {
                    return castle::status::full;
                }
                entry->used = true;
                entry->identifier = identifier;
                copy_text(entry->topic, message.topic);
                entry->payload_size = payload_size;
                for (size_type i = 0U; i < payload_size; ++i)
                {
                    entry->payload[i] = message.payload[i];
                }
                entry->retain = message.retain;
            }
            return send_identifier(packet_type::pubrec, identifier, false);
        }
        if (message_callback_)
        {
            dispatcher_.invoke(message_callback_, message);
        }
        if (quality == qos::at_least_once)
        {
            return send_identifier(packet_type::puback, identifier, false);
        }
        return castle::status::ok;
    }

    castle::status handle_pubrel(packet_reader& reader) CASTLE_NOEXCEPT
    {
        uint16_t identifier = 0U;
        if (!castle::succeeded(reader.get_u16(identifier)) || identifier == 0U)
        {
            return castle::status::invalid_argument;
        }
        if (options_.protocol_version == version::v5_0 && reader.remaining() != 0U)
        {
            uint8_t reason = 0U;
            (void)reader.get_u8(reason);
            properties_type properties;
            if (reader.remaining() != 0U && !castle::succeeded(decode_properties(reader, properties, true))) // LCOV_EXCL_BR_LINE
            {
                return reader.status();
            }
        }
        else if (options_.protocol_version != version::v5_0 && reader.remaining() != 0U)
        {
            return castle::status::invalid_argument;
        }
        incoming_entry* entry = find_incoming(identifier);
        if (entry != nullptr)
        {
            message_view message;
            message.topic = castle::container::string_view(entry->topic);
            message.payload = entry->payload;
            message.payload_size = entry->payload_size;
            message.quality = qos::exactly_once;
            message.retain = entry->retain;
            message.packet_identifier = identifier;
            if (message_callback_)
            {
                dispatcher_.invoke(message_callback_, message);
            }
            entry->used = false;
        }
        return send_identifier(packet_type::pubcomp, identifier, false);
    }

    castle::status handle_ack(packet_reader& reader, outgoing_state expected, packet_type type) CASTLE_NOEXCEPT
    {
        uint16_t identifier = 0U;
        if (!castle::succeeded(reader.get_u16(identifier)) || identifier == 0U)
        {
            return castle::status::invalid_argument;
        }
        uint8_t reason = 0U;
        if (options_.protocol_version == version::v5_0 && reader.remaining() != 0U)
        {
            (void)reader.get_u8(reason);
            properties_type properties;
            if (reader.remaining() != 0U && !castle::succeeded(decode_properties(reader, properties, true))) // LCOV_EXCL_BR_LINE
            {
                return reader.status();
            }
        }
        else if (options_.protocol_version != version::v5_0 && reader.remaining() != 0U)
        {
            return castle::status::invalid_argument;
        }
        outgoing_entry* entry = find_outgoing(identifier);
        if (entry == nullptr || entry->state != expected)
        {
            return castle::status::invalid_argument;
        }
        last_acknowledged_id_ = identifier;
        if (type == packet_type::pubrec)
        {
            if (reason >= 0x80U)
            {
                entry->used = false;
                return to_status(static_cast<reason_code>(reason));
            }
            entry->state = outgoing_state::await_pubcomp;
            return send_identifier(packet_type::pubrel, identifier, false);
        }
        if (reason >= 0x80U)
        {
            entry->used = false;
            return to_status(static_cast<reason_code>(reason));
        }
        entry->used = false;
        return castle::status::ok;
    }

    castle::status handle_suback(packet_reader& reader) CASTLE_NOEXCEPT
    {
        uint16_t identifier = 0U;
        if (!castle::succeeded(reader.get_u16(identifier)) || identifier == 0U)
        {
            return castle::status::invalid_argument;
        }
        if (options_.protocol_version == version::v5_0)
        {
            properties_type properties;
            castle::status status = decode_properties(reader, properties, true);
            if (!castle::succeeded(status))
            {
                return status;
            }
        }
        if (reader.remaining() == 0U)
        {
            return castle::status::invalid_argument;
        }
        while (reader.remaining() != 0U)
        {
            uint8_t code = 0U;
            (void)reader.get_u8(code);
            if (code > 2U && code < 0x80U)
            {
                return castle::status::invalid_argument;
            }
        }
        pending_request* request = find_request(identifier, packet_type::subscribe);
        if (request == nullptr)
        {
            return castle::status::invalid_argument;
        }
        request->used = false;
        last_acknowledged_id_ = identifier;
        return castle::status::ok;
    }

    castle::status handle_unsuback(packet_reader& reader) CASTLE_NOEXCEPT
    {
        uint16_t identifier = 0U;
        if (!castle::succeeded(reader.get_u16(identifier)) || identifier == 0U)
        {
            return castle::status::invalid_argument;
        }
        if (options_.protocol_version == version::v5_0)
        {
            properties_type properties;
            castle::status status = decode_properties(reader, properties, true);
            if (!castle::succeeded(status))
            {
                return status;
            }
            while (reader.remaining() != 0U)
            {
                uint8_t reason = 0U;
                (void)reader.get_u8(reason);
            }
        }
        else if (reader.remaining() != 0U)
        {
            return castle::status::invalid_argument;
        }
        pending_request* request = find_request(identifier, packet_type::unsubscribe);
        if (request == nullptr)
        {
            return castle::status::invalid_argument;
        }
        request->used = false;
        last_acknowledged_id_ = identifier;
        return castle::status::ok;
    }

    castle::status send_identifier(packet_type type, uint16_t identifier, bool duplicate) CASTLE_NOEXCEPT
    {
        packet_writer body(body_, MaxPacketSize);
        (void)body.put_u16(identifier);
        if (options_.protocol_version == version::v5_0 && type == packet_type::pubrel)
        {
            (void)body.put_u8(0U); // Success reason code
            (void)body.put_u8(0U); // Property Length
        }
        uint8_t flags = type == packet_type::pubrel ? static_cast<uint8_t>(duplicate ? 0x0AU : 0x02U) : 0U; // LCOV_EXCL_BR_LINE
        return send_body(type, flags, body.data(), body.size());
    }

    transport_type transport_{};
    message_callback message_callback_{};
    castle::callbacks::callback_dispatcher<message_callback> dispatcher_{};
    connect_options<MaxProperties, PropertyStringCapacity, PropertyBinaryCapacity> options_{};
    outgoing_entry outgoing_[MaxOutstanding]{};
    incoming_entry incoming_[MaxOutstanding]{};
    pending_request requests_[MaxOutstanding]{};
    uint8_t rx_[MaxPacketSize + ReceiveBufferSize]{};
    uint8_t body_[MaxPacketSize]{};
    uint8_t tx_[MaxPacketSize]{};
    size_type rx_size_ = 0U;
    uint16_t next_identifier_ = 0U;
    uint16_t last_acknowledged_id_ = 0U;
    uint32_t now_ms_ = 0U;
    uint32_t last_tx_ms_ = 0U;
    uint32_t last_rx_ms_ = 0U;
    uint32_t ping_sent_ms_ = 0U;
    bool connecting_ = false;
    bool connected_ = false;
    bool session_present_ = false;
    bool ping_outstanding_ = false;
    bool clock_started_ = false;
    bool fatal_ = false;
    castle::status last_error_ = castle::status::ok;
};

/** @brief Default-capacity MQTT client alias. */
using client = basic_client<>;

} // namespace mqtt
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_MQTT_CLIENT_HPP
