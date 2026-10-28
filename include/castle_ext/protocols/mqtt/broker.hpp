// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file broker.hpp
 * @brief Single-process, fixed-capacity MQTT broker over CASTLE's TCP server socket.
 */
#ifndef CASTLE_EXT_PROTOCOLS_MQTT_BROKER_HPP
#define CASTLE_EXT_PROTOCOLS_MQTT_BROKER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string_view.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/net/server.hpp"
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
 * @brief Poll-driven MQTT broker with bounded clients, sessions, subscriptions, retained messages and QoS state.
 *
 * Session/subscription/retained state is held only in RAM. The broker is single-threaded and not synchronized;
 * all methods, including `poll()`, must be serialized by the owner. `MaxClients` bounds concurrent TCP peers,
 * while each client owns independent receive/QoS buffers. The server's TCP receive callbacks execute inline.
 */
template <size_type MaxClients = 4U,
          size_type ReceiveBufferSize = 512U,
          size_type MaxPacketSize = 1024U,
          size_type MaxSubscriptions = 32U,
          size_type MaxRetained = 16U,
          size_type MaxOutstanding = 8U,
          size_type MaxTopicLength = 128U,
          size_type MaxPayloadSize = 512U,
          size_type MaxProperties = 8U,
          size_type PropertyStringCapacity = 64U,
          size_type PropertyBinaryCapacity = 32U>
class basic_broker
{
    static_assert(MaxClients > 0U && ReceiveBufferSize > 0U, "MQTT broker requires client and receive capacity");
    static_assert(MaxPacketSize >= 8U && MaxSubscriptions > 0U && MaxTopicLength > 1U, "MQTT broker capacities are too small");
    static_assert(MaxOutstanding > 0U && MaxRetained > 0U, "MQTT broker QoS and retained capacities must be non-zero");

public:
    using server_type = castle::net::tcp_server<MaxClients, ReceiveBufferSize>;
    using client_handle = typename server_type::client_handle;
    using properties_type = property_list<MaxProperties, PropertyStringCapacity, PropertyBinaryCapacity>;

    /** @brief Constructs an unopened broker with empty client/session tables. */
    basic_broker() CASTLE_DEFAULT;
    basic_broker(basic_broker CASTLE_CONST&) CASTLE_DELETE;
    basic_broker& operator=(basic_broker CASTLE_CONST&) CASTLE_DELETE;

    /** @brief Opens the TCP listener (port 1883 is the MQTT default). */
    castle::status open(castle::net::endpoint CASTLE_CONST& local = castle::net::endpoint::any_ipv4(1883U),
                        int backlog = 8) CASTLE_NOEXCEPT
    {
        return server_.open(local, backlog, true, nullptr);
    }

    /** @brief Closes all clients and the listening socket. */
    castle::status close() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            clients_[i].used = false;
        }
        return server_.close(nullptr);
    }

    /** @brief Copies fixed-length username/password credentials; empty username disables authentication. */
    castle::status set_authentication(castle::container::string_view username,
                                      castle::container::string_view password) CASTLE_NOEXCEPT
    {
        if (username.size() >= MaxTopicLength || password.size() >= MaxTopicLength ||
            !detail::valid_utf8(username) || !detail::valid_utf8(password))
        {
            return castle::status::out_of_range;
        }
        copy_text(expected_username_, username);
        copy_text(expected_password_, password);
        authentication_enabled_ = !username.empty();
        return castle::status::ok;
    }

    /** @brief Processes one accept and at most one receive chunk for each connected client. */
    castle::status poll(uint32_t now_ms, int accept_timeout_ms = 0) CASTLE_NOEXCEPT
    {
        if (!server_.is_open())
        {
            return castle::status::not_configured;
        }
        now_ms_ = now_ms;
        prune_sessions();
        client_handle accepted;
        castle::status accepted_status = server_.accept_client(accepted, accept_timeout_ms, nullptr);
        if (castle::succeeded(accepted_status))
        {
            client_state& state = clients_[accepted.index];
            state = client_state{};
            state.used = true;
            state.handle = accepted;
            state.last_received_ms = now_ms_;
            castle::status callback_status = server_.set_client_callback(accepted,
                [this](client_handle handle, castle::net::endpoint CASTLE_CONST&, uint8_t CASTLE_CONST* bytes, size_type size)
                {
                    receive_bytes(handle, bytes, size);
                });
            if (!castle::succeeded(callback_status)) // LCOV_EXCL_BR_LINE
            {
                state.close_pending = true;
            }
        }
        else if (accepted_status != castle::status::empty && accepted_status != castle::status::full)
        {
            return accepted_status;
        }

        for (size_type i = 0U; i < MaxClients; ++i)
        {
            client_handle handle = server_.client_at(i);
            if (!handle.is_valid())
            {
                continue;
            }
            client_state& state = clients_[i];
            if (!state.used || state.handle != handle)
            {
                state = client_state{};
                state.used = true;
                state.handle = handle;
                state.last_received_ms = now_ms_;
            }
            if (state.keep_alive_seconds != 0U &&
                elapsed(now_ms_, state.last_received_ms) > (static_cast<uint32_t>(state.keep_alive_seconds) * 1500U))
            {
                state.close_pending = true;
                state.will_pending = true;
            }
            if (!state.close_pending)
            {
                castle::status result = server_.process_client(handle, 0, nullptr);
                if (result != castle::status::ok && result != castle::status::empty)
                {
                    state.close_pending = true;
                    if (result == castle::status::not_configured || result == castle::status::system_call_error) // LCOV_EXCL_BR_LINE
                    {
                        state.will_pending = true;
                    }
                }
            }
            if (state.close_pending)
            {
                if (state.will_pending)
                {
                    publish_will(state);
                }
                close_state(state, true);
                (void)server_.close_client(handle, nullptr);
            }
        }
        return castle::status::ok;
    }

    size_type client_count() CASTLE_CONST CASTLE_NOEXCEPT { return server_.client_count(); }
    size_type subscription_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type count = 0U;
        for (size_type i = 0U; i < MaxSubscriptions; ++i)
        {
            if (subscriptions_[i].used)
            {
                ++count;
            }
        }
        return count;
    }
    size_type retained_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type count = 0U;
        for (size_type i = 0U; i < MaxRetained; ++i)
        {
            if (retained_[i].used)
            {
                ++count;
            }
        }
        return count;
    }
    server_type& transport() CASTLE_NOEXCEPT { return server_; }

private:
    enum class outgoing_state : uint8_t { await_puback, await_pubrec, await_pubcomp };
    struct qos_entry
    {
        bool used = false;
        uint16_t identifier = 0U;
        outgoing_state state = outgoing_state::await_puback;
        uint8_t quality = 0U;
        char topic[MaxTopicLength]{};
        uint8_t payload[MaxPayloadSize]{};
        size_type payload_size = 0U;
        bool retain = false;
    };
    struct client_state
    {
        bool used = false;
        bool close_pending = false;
        bool will_pending = false;
        bool clean_session = true;
        bool will_present = false;
        bool will_retain = false;
        uint8_t protocol_level = 0U;
        uint16_t keep_alive_seconds = 0U;
        uint16_t next_packet_identifier = 0U;
        uint32_t last_received_ms = 0U;
        uint32_t session_expiry_seconds = 0U;
        client_handle handle{};
        char client_id[MaxTopicLength]{};
        char will_topic[MaxTopicLength]{};
        uint8_t will_payload[MaxPayloadSize]{};
        size_type will_payload_size = 0U;
        qos will_quality = qos::at_most_once;
        uint8_t rx[MaxPacketSize + ReceiveBufferSize]{};
        size_type rx_size = 0U;
        qos_entry incoming_qos2[MaxOutstanding]{};
        qos_entry outgoing[MaxOutstanding]{};
    };
    struct subscription_entry
    {
        bool used = false;
        char client_id[MaxTopicLength]{};
        char filter[MaxTopicLength]{};
        qos maximum_quality = qos::at_most_once;
    };
    struct retained_entry
    {
        bool used = false;
        char topic[MaxTopicLength]{};
        uint8_t payload[MaxPayloadSize]{};
        size_type payload_size = 0U;
        qos quality = qos::at_most_once;
    };
    struct session_entry
    {
        bool used = false;
        char client_id[MaxTopicLength]{};
        uint32_t expires_at_ms = 0U;
    };

    static uint32_t elapsed(uint32_t now, uint32_t then) CASTLE_NOEXCEPT { return now - then; }
    static void copy_text(char* target, castle::container::string_view value) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < value.size(); ++i)
        {
            target[i] = value[i];
        }
        target[value.size()] = '\0';
    }
    static bool text_equal(char CASTLE_CONST* left, castle::container::string_view right) CASTLE_NOEXCEPT
    {
        size_type i = 0U;
        while (left[i] != '\0' && i < right.size())
        {
            if (left[i] != right[i])
            {
                return false;
            }
            ++i;
        }
        return left[i] == '\0' && i == right.size();
    }
    static bool text_equal(char CASTLE_CONST* left, char CASTLE_CONST* right) CASTLE_NOEXCEPT
    {
        size_type i = 0U;
        while (left[i] != '\0' && right[i] != '\0')
        {
            if (left[i] != right[i])
            {
                return false;
            }
            ++i;
        }
        return left[i] == right[i];
    }
    client_state* find_client(client_handle handle) CASTLE_NOEXCEPT
    {
        if (!handle.is_valid() || handle.index >= MaxClients) // LCOV_EXCL_BR_LINE
        {
            return nullptr;
        }
        client_state& state = clients_[handle.index];
        return state.used && state.handle == handle ? &state : nullptr; // LCOV_EXCL_BR_LINE
    }

    void receive_bytes(client_handle handle, uint8_t CASTLE_CONST* data, size_type size) CASTLE_NOEXCEPT
    {
        client_state* state = find_client(handle);
        if (state == nullptr) // LCOV_EXCL_BR_LINE
        {
            return;
        }
        state->last_received_ms = now_ms_;
        // LCOV_EXCL_START
        // can not cover branches code
        if (data == nullptr || size > (MaxPacketSize + ReceiveBufferSize) - state->rx_size)
        {
            state->close_pending = true;
            return;
        }
        // LCOV_EXCL_STOP
        for (size_type i = 0U; i < size; ++i)
        {
            state->rx[state->rx_size++] = data[i];
        }
        size_type consumed = 0U;
        while (consumed < state->rx_size && !state->close_pending)
        {
            packet_view packet{};
            castle::status result = inspect_packet(state->rx + consumed, state->rx_size - consumed, MaxPacketSize, packet);
            if (result == castle::status::empty)
            {
                break;
            }
            if (!castle::succeeded(result) || !castle::succeeded(handle_packet(*state, packet)))
            {
                state->close_pending = true;
                break;
            }
            consumed += packet.packet_size;
        }
        if (consumed != 0U)
        {
            size_type remain = state->rx_size - consumed;
            for (size_type i = 0U; i < remain; ++i)
            {
                state->rx[i] = state->rx[consumed + i];
            }
            state->rx_size = remain;
        }
    }

    castle::status handle_packet(client_state& state, packet_view CASTLE_CONST& packet) CASTLE_NOEXCEPT
    {
        packet_reader reader(packet.body, packet.remaining_length);
        if (state.protocol_level == 0U && packet.type != packet_type::connect)
        {
            return castle::status::invalid_argument;
        }
        switch (packet.type)
        {
            case packet_type::connect: return handle_connect(state, reader);
            case packet_type::publish: return handle_publish(state, packet, reader);
            case packet_type::puback: return handle_client_ack(state, reader, outgoing_state::await_puback, packet_type::puback);
            case packet_type::pubrec: return handle_client_ack(state, reader, outgoing_state::await_pubrec, packet_type::pubrec);
            case packet_type::pubcomp: return handle_client_ack(state, reader, outgoing_state::await_pubcomp, packet_type::pubcomp);
            case packet_type::pubrel: return handle_client_pubrel(state, reader);
            case packet_type::subscribe: return handle_subscribe(state, reader);
            case packet_type::unsubscribe: return handle_unsubscribe(state, reader);
            case packet_type::pingreq:
                if (packet.remaining_length != 0U || state.protocol_level == 0U) // LCOV_EXCL_BR_LINE
                {
                    return castle::status::invalid_argument;
                }
                return send_packet(state.handle, packet_type::pingresp, 0U, nullptr, 0U);
            case packet_type::disconnect:
                if (state.protocol_level == 0U) // LCOV_EXCL_BR_LINE
                {
                    return castle::status::invalid_argument;
                }
                if (state.protocol_level == 5U && reader.remaining() != 0U)
                {
                    uint8_t reason = 0U;
                    (void)reader.get_u8(reason);
                    properties_type props;
                    if (reader.remaining() != 0U && !castle::succeeded(decode_properties(reader, props, true)))
                    {
                        return reader.status();
                    }
                }
                else if (state.protocol_level != 5U && reader.remaining() != 0U)
                {
                    return castle::status::invalid_argument;
                }
                state.will_pending = false;
                state.close_pending = true;
                return castle::status::ok;
            case packet_type::auth: return castle::status::invalid_config;
            default: return castle::status::invalid_argument;
        }
    }

    castle::status handle_connect(client_state& state, packet_reader& reader) CASTLE_NOEXCEPT
    {
        if (state.protocol_level != 0U)
        {
            return castle::status::invalid_argument;
        }
        char protocol_name[8U]{}; size_type name_size = 0U;
        uint8_t level = 0U; uint8_t flags = 0U; uint16_t keep_alive = 0U;
        if (!castle::succeeded(reader.get_utf8(protocol_name, sizeof(protocol_name), name_size)) ||
            !castle::succeeded(reader.get_u8(level)) || !castle::succeeded(reader.get_u8(flags)) ||
            !castle::succeeded(reader.get_u16(keep_alive)))
        {
            return castle::status::invalid_argument;
        }
        state.protocol_level = level;
        bool name_ok = (level == 3U && text_equal(protocol_name, "MQIsdp")) ||
                       ((level == 4U || level == 5U) && text_equal(protocol_name, "MQTT"));
        if (!name_ok || (flags & 0x01U) != 0U || ((flags & 0x04U) == 0U && (flags & 0x38U) != 0U) ||
            ((flags & 0x04U) != 0U && ((flags & 0x18U) >> 3U) == 3U) ||
            ((flags & 0x40U) != 0U && (flags & 0x80U) == 0U))
        {
            (void)send_connack(state, false, level > 5U ? reason_code::unsupported_protocol_version : reason_code::malformed_packet);
            return castle::status::invalid_argument;
        }
        properties_type connect_properties;
        if (level == 5U)
        {
            castle::status status = decode_properties(reader, connect_properties, true);
            if (!castle::succeeded(status))
            {
                return status;
            }
            if (auto CASTLE_CONST* expiry = connect_properties.find(property_id::session_expiry_interval))
            {
                state.session_expiry_seconds = expiry->integer;
            }
        }
        char id[MaxTopicLength]{}; size_type id_size = 0U;
        bool clean = (flags & 0x02U) != 0U;
        if (!castle::succeeded(reader.get_utf8(id, sizeof(id), id_size)))
        {
            (void)send_connack(state, false, reason_code::client_identifier_not_valid);
            return castle::status::invalid_argument;
        }
        // A zero-length ClientId is only allowed with a clean session; the server assigns a unique one.
        bool assigned = id_size == 0U;
        if (assigned && (!clean || !castle::succeeded(assign_client_id(id, sizeof(id), id_size))))
        {
            (void)send_connack(state, false, reason_code::client_identifier_not_valid);
            return castle::status::invalid_argument;
        }
        bool will = (flags & 0x04U) != 0U;
        if (will && level == 5U)
        {
            properties_type will_properties;
            castle::status status = decode_properties(reader, will_properties, true);
            if (!castle::succeeded(status))
            {
                return status;
            }
        }
        char will_topic[MaxTopicLength]{}; size_type will_topic_size = 0U;
        uint8_t will_payload[MaxPayloadSize]{}; size_type will_payload_size = 0U;
        if (will)
        {
            if (!castle::succeeded(reader.get_utf8(will_topic, sizeof(will_topic), will_topic_size)) ||
                !castle::succeeded(validate_topic_name(castle::container::string_view(will_topic, will_topic_size))) ||
                !castle::succeeded(reader.get_binary(will_payload, sizeof(will_payload), will_payload_size)))
            {
                return castle::status::invalid_argument;
            }
        }
        char username[MaxTopicLength]{}; size_type username_size = 0U;
        uint8_t password[MaxTopicLength]{}; size_type password_size = 0U;
        bool has_username = (flags & 0x80U) != 0U;
        bool has_password = (flags & 0x40U) != 0U;
        if (has_username && !castle::succeeded(reader.get_utf8(username, sizeof(username), username_size)))
        {
            return castle::status::invalid_argument;
        }
        if (has_password && !castle::succeeded(reader.get_binary(password, sizeof(password), password_size)))
        {
            return castle::status::invalid_argument;
        }
        if (reader.remaining() != 0U)
        {
            return castle::status::invalid_argument;
        }
        if (authentication_enabled_ && (!has_username || !has_password ||
            !text_equal(expected_username_, castle::container::string_view(username, username_size)) ||
            !binary_equal(expected_password_, password, password_size)))
        {
            (void)send_connack(state, false, reason_code::bad_user_name_or_password);
            return castle::status::not_configured;
        }

        bool session_present = !clean && session_exists(id);
        if (clean)
        {
            clear_session(id);
        }
        if (!ensure_session(id, clean, state.session_expiry_seconds))
        {
            (void)send_connack(state, false, reason_code::server_busy);
            return castle::status::full;
        }
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            client_state& other = clients_[i];
            if (&other != &state && other.used && other.client_id[0U] != '\0' && text_equal(other.client_id, id))
            {
                other.close_pending = true;
                other.will_pending = false;
            }
        }
        copy_text(state.client_id, castle::container::string_view(id, id_size));
        state.protocol_level = level;
        state.clean_session = clean;
        state.keep_alive_seconds = keep_alive;
        state.will_present = will;
        state.will_pending = will;
        state.will_retain = (flags & 0x20U) != 0U;
        state.will_quality = static_cast<qos>((flags >> 3U) & 0x03U);
        if (will)
        {
            copy_text(state.will_topic, castle::container::string_view(will_topic, will_topic_size));
            state.will_payload_size = will_payload_size;
            for (size_type i = 0U; i < will_payload_size; ++i)
            {
                state.will_payload[i] = will_payload[i];
            }
        }
        return send_connack(state, session_present, reason_code::success, assigned ? id : nullptr, id_size);
    }

    bool binary_equal(char CASTLE_CONST* expected, uint8_t CASTLE_CONST* value, size_type size) CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type i = 0U;
        while (expected[i] != '\0')
        {
            if (i >= size || static_cast<uint8_t>(expected[i]) != value[i])
            {
                return false;
            }
            ++i;
        }
        return i == size;
    }

    bool session_exists(char CASTLE_CONST* id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            if (sessions_[i].used && text_equal(sessions_[i].client_id, id))
            {
                return true;
            }
        }
        return false;
    }
    void prune_sessions() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            if (sessions_[i].used && sessions_[i].expires_at_ms != 0U &&
                static_cast<int32_t>(now_ms_ - sessions_[i].expires_at_ms) >= 0)
            {
                char expired_id[MaxTopicLength]{};
                copy_text(expired_id, castle::container::string_view(sessions_[i].client_id));
                clear_session(expired_id);
            }
        }
    }

    /** @brief Writes a server-assigned ClientId ("castle-N") that does not match an existing session. */
    castle::status assign_client_id(char* id, size_type capacity, size_type& size) CASTLE_NOEXCEPT
    {
        static CASTLE_CONSTEXPR char prefix[] = "castle-";
        CASTLE_CONSTEXPR size_type prefix_size = sizeof(prefix) - 1U;
        for (size_type attempt = 0U; attempt <= MaxClients; ++attempt)
        {
            uint32_t value = next_assigned_id_++;
            char digits[10U]{};
            size_type digit_count = 0U;
            do
            {
                digits[digit_count++] = static_cast<char>('0' + value % 10U);
                value /= 10U;
            } while (value != 0U);
            if (prefix_size + digit_count + 1U > capacity)
            {
                return castle::status::invalid_argument;
            }
            size = 0U;
            for (size_type i = 0U; i < prefix_size; ++i) { id[size++] = prefix[i]; }
            for (size_type i = digit_count; i > 0U; --i) { id[size++] = digits[i - 1U]; }
            id[size] = '\0';
            if (!session_exists(id))
            {
                return castle::status::ok;
            }
        }
        return castle::status::full;
    }

    void clear_session(char CASTLE_CONST* id) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxSubscriptions; ++i)
        {
            if (subscriptions_[i].used && text_equal(subscriptions_[i].client_id, id))
            {
                subscriptions_[i].used = false;
            }
        }
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            if (sessions_[i].used && text_equal(sessions_[i].client_id, id))
            {
                sessions_[i].used = false;
            }
        }
    }
    bool ensure_session(char CASTLE_CONST* id, bool clean, uint32_t expiry_seconds) CASTLE_NOEXCEPT
    {
        (void)clean;
        (void)expiry_seconds;
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            if (sessions_[i].used && text_equal(sessions_[i].client_id, id))
            {
                sessions_[i].expires_at_ms = 0U; // Session is active; expiry starts when it disconnects.
                return true;
            }
        }
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            if (!sessions_[i].used)
            {
                sessions_[i].used = true;
                copy_text(sessions_[i].client_id, castle::container::string_view(id));
                sessions_[i].expires_at_ms = 0U; // Active session.
                return true;
            }
        }
        return false;
    }

    castle::status send_connack(client_state& state, bool session_present, reason_code reason,
                                char CASTLE_CONST* assigned_id = nullptr, size_type assigned_size = 0U) CASTLE_NOEXCEPT
    {
        packet_writer body(tx_, MaxPacketSize);
        (void)body.put_u8(session_present ? 1U : 0U);
        (void)body.put_u8(static_cast<uint8_t>(reason));
        if (state.protocol_level == 5U)
        {
            if (assigned_id == nullptr)
            {
                (void)body.put_u8(0U);
            }
            else
            {
                castle::container::string_view assigned(assigned_id, assigned_size);
                (void)body.put_varint(static_cast<uint32_t>(3U + assigned.size()));
                (void)body.put_u8(static_cast<uint8_t>(property_id::assigned_client_identifier));
                (void)body.put_utf8(assigned);
            }
        }
        if (!castle::succeeded(body.status())) // LCOV_EXCL_BR_LINE
        {
            return body.status();
        }
        return send_packet(state.handle, packet_type::connack, 0U, body.data(), body.size());
    }

    castle::status handle_publish(client_state& state, packet_view CASTLE_CONST& packet, packet_reader& reader) CASTLE_NOEXCEPT
    {
        uint8_t flags = packet.flags;
        uint8_t quality_value = static_cast<uint8_t>((flags >> 1U) & 0x03U);
        if (quality_value > 2U) // LCOV_EXCL_BR_LINE
        {
            return castle::status::invalid_argument;
        }
        qos quality = static_cast<qos>(quality_value);
        char topic[MaxTopicLength]{}; size_type topic_size = 0U;
        if (!castle::succeeded(reader.get_utf8(topic, sizeof(topic), topic_size)) ||
            !castle::succeeded(validate_topic_name(castle::container::string_view(topic, topic_size))))
        {
            return castle::status::invalid_argument;
        }
        uint16_t identifier = 0U;
        if (quality != qos::at_most_once && (!castle::succeeded(reader.get_u16(identifier)) || identifier == 0U))
        {
            return castle::status::invalid_argument;
        }
        if (state.protocol_level == 5U)
        {
            properties_type props;
            castle::status status = decode_properties(reader, props, true);
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
        if (quality == qos::exactly_once)
        {
            qos_entry* entry = find_qos(state.incoming_qos2, identifier);
            if (entry == nullptr)
            {
                entry = allocate_qos(state.incoming_qos2);
                if (entry == nullptr)
                {
                    return castle::status::full;
                }
                entry->used = true;
                entry->identifier = identifier;
                entry->quality = 2U;
                copy_text(entry->topic, castle::container::string_view(topic, topic_size));
                entry->payload_size = payload_size;
                for (size_type i = 0U; i < payload_size; ++i)
                {
                    entry->payload[i] = reader.current()[i];
                }
                entry->retain = (flags & 1U) != 0U;
            }
            return send_identifier(state.handle, packet_type::pubrec, identifier, false);
        }
        publish_message(castle::container::string_view(topic, topic_size), reader.current(), payload_size,
                        quality, (flags & 1U) != 0U);
        return quality == qos::at_least_once ? send_identifier(state.handle, packet_type::puback, identifier, false) : castle::status::ok;
    }

    castle::status handle_client_pubrel(client_state& state, packet_reader& reader) CASTLE_NOEXCEPT
    {
        uint16_t identifier = 0U;
        if (!castle::succeeded(reader.get_u16(identifier)) || identifier == 0U)
        {
            return castle::status::invalid_argument;
        }
        if (state.protocol_level == 5U && reader.remaining() != 0U)
        {
            uint8_t reason = 0U;
            (void)reader.get_u8(reason);
            properties_type props;
            if (reader.remaining() != 0U && !castle::succeeded(decode_properties(reader, props, true)))
            {
                return reader.status();
            }
        }
        else if (state.protocol_level != 5U && reader.remaining() != 0U)
        {
            return castle::status::invalid_argument;
        }
        qos_entry* entry = find_qos(state.incoming_qos2, identifier);
        if (entry != nullptr)
        {
            publish_message(castle::container::string_view(entry->topic), entry->payload, entry->payload_size,
                            qos::exactly_once, entry->retain);
            entry->used = false;
        }
        return send_identifier(state.handle, packet_type::pubcomp, identifier, false);
    }

    castle::status handle_client_ack(client_state& state, packet_reader& reader,
                                     outgoing_state expected, packet_type type) CASTLE_NOEXCEPT
    {
        uint16_t identifier = 0U;
        if (!castle::succeeded(reader.get_u16(identifier)) || identifier == 0U)
        {
            return castle::status::invalid_argument;
        }
        uint8_t reason = 0U;
        if (state.protocol_level == 5U && reader.remaining() != 0U)
        {
            (void)reader.get_u8(reason);
            properties_type props;
            if (reader.remaining() != 0U && !castle::succeeded(decode_properties(reader, props, true)))
            {
                return reader.status();
            }
        }
        else if (state.protocol_level != 5U && reader.remaining() != 0U)
        {
            return castle::status::invalid_argument;
        }
        qos_entry* entry = find_qos(state.outgoing, identifier);
        if (entry == nullptr || entry->state != expected)
        {
            return castle::status::invalid_argument;
        }
        if (reason >= 0x80U)
        {
            entry->used = false;
            return to_status(static_cast<reason_code>(reason));
        }
        if (type == packet_type::pubrec)
        {
            entry->state = outgoing_state::await_pubcomp;
            return send_identifier(state.handle, packet_type::pubrel, identifier, false);
        }
        entry->used = false;
        return castle::status::ok;
    }

    castle::status handle_subscribe(client_state& state, packet_reader& reader) CASTLE_NOEXCEPT
    {
        uint16_t identifier = 0U;
        if (!castle::succeeded(reader.get_u16(identifier)) || identifier == 0U)
        {
            return castle::status::invalid_argument;
        }
        if (state.protocol_level == 5U)
        {
            properties_type props;
            castle::status status = decode_properties(reader, props, true);
            if (!castle::succeeded(status))
            {
                return status;
            }
        }
        uint8_t reasons[MaxSubscriptions]{}; size_type reason_count = 0U;
        char filters[MaxSubscriptions][MaxTopicLength]{};
        qos requested[MaxSubscriptions]{};
        while (reader.remaining() != 0U)
        {
            if (reason_count >= MaxSubscriptions)
            {
                return castle::status::full;
            }
            size_type filter_size = 0U;
            uint8_t options = 0U;
            if (!castle::succeeded(reader.get_utf8(filters[reason_count], MaxTopicLength, filter_size)) ||
                !castle::succeeded(validate_topic_filter(castle::container::string_view(filters[reason_count], filter_size))) ||
                !castle::succeeded(reader.get_u8(options)))
            {
                return castle::status::invalid_argument;
            }
            uint8_t requested_qos = options & 0x03U;
            bool invalid_options = requested_qos > 2U;
            if (state.protocol_level == 5U)
            {
                invalid_options = invalid_options || (options & 0xC0U) != 0U || ((options >> 4U) & 0x03U) == 3U;
            }
            else
            {
                invalid_options = invalid_options || (options & 0xFCU) != 0U;
            }
            if (invalid_options)
            {
                return castle::status::invalid_argument;
            }
            requested[reason_count] = static_cast<qos>(requested_qos);
            if (add_subscription(state.client_id, castle::container::string_view(filters[reason_count]), requested[reason_count]))
            {
                reasons[reason_count] = requested_qos;
            }
            else
            {
                reasons[reason_count] = state.protocol_level == 5U ? 0x97U : 0x80U;
            }
            ++reason_count;
        }
        if (reason_count == 0U)
        {
            return castle::status::invalid_argument;
        }
        packet_writer body(tx_, MaxPacketSize);
        (void)body.put_u16(identifier);
        if (state.protocol_level == 5U)
        {
            (void)body.put_u8(0U);
        }
        (void)body.put_bytes(reasons, reason_count);
        castle::status result = send_packet(state.handle, packet_type::suback, 0U, body.data(), body.size());
        if (!castle::succeeded(result))
        {
            return result;
        }
        for (size_type f = 0U; f < reason_count; ++f)
        {
            if (reasons[f] > 2U)
            {
                continue;
            }
            for (size_type r = 0U; r < MaxRetained; ++r)
            {
                retained_entry CASTLE_CONST& retained = retained_[r];
                if (retained.used && topic_matches(castle::container::string_view(filters[f]), castle::container::string_view(retained.topic)))
                {
                    qos delivery = static_cast<uint8_t>(retained.quality) < static_cast<uint8_t>(requested[f])
                        ? retained.quality : requested[f];
                    (void)send_publish_to(state.handle, castle::container::string_view(retained.topic),
                                          retained.payload, retained.payload_size, delivery, true);
                }
            }
        }
        return castle::status::ok;
    }

    castle::status handle_unsubscribe(client_state& state, packet_reader& reader) CASTLE_NOEXCEPT
    {
        uint16_t identifier = 0U;
        if (!castle::succeeded(reader.get_u16(identifier)) || identifier == 0U)
        {
            return castle::status::invalid_argument;
        }
        if (state.protocol_level == 5U)
        {
            properties_type props;
            castle::status status = decode_properties(reader, props, true);
            if (!castle::succeeded(status))
            {
                return status;
            }
        }
        uint8_t results[MaxSubscriptions]{}; size_type count = 0U;
        while (reader.remaining() != 0U)
        {
            if (count >= MaxSubscriptions)
            {
                return castle::status::full;
            }
            char filter[MaxTopicLength]{};
            size_type filter_size = 0U;
            if (!castle::succeeded(reader.get_utf8(filter, sizeof(filter), filter_size)) ||
                !castle::succeeded(validate_topic_filter(castle::container::string_view(filter, filter_size))))
            {
                return castle::status::invalid_argument;
            }
            bool removed = remove_subscription(state.client_id, filter);
            results[count++] = state.protocol_level == 5U ? static_cast<uint8_t>(removed ? 0U : 0x11U) : 0U;
        }
        if (count == 0U)
        {
            return castle::status::invalid_argument;
        }
        packet_writer body(tx_, MaxPacketSize);
        (void)body.put_u16(identifier);
        if (state.protocol_level == 5U)
        {
            (void)body.put_u8(0U);
            (void)body.put_bytes(results, count);
        }
        return send_packet(state.handle, packet_type::unsuback, 0U, body.data(), body.size());
    }

    bool add_subscription(char CASTLE_CONST* client_id, castle::container::string_view filter, qos quality) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxSubscriptions; ++i)
        {
            if (subscriptions_[i].used && text_equal(subscriptions_[i].client_id, client_id) && text_equal(subscriptions_[i].filter, filter))
            {
                subscriptions_[i].maximum_quality = quality;
                return true;
            }
        }
        for (size_type i = 0U; i < MaxSubscriptions; ++i)
        {
            if (!subscriptions_[i].used)
            {
                subscriptions_[i].used = true;
                copy_text(subscriptions_[i].client_id, castle::container::string_view(client_id));
                copy_text(subscriptions_[i].filter, filter);
                subscriptions_[i].maximum_quality = quality;
                return true;
            }
        }
        return false;
    }
    bool remove_subscription(char CASTLE_CONST* client_id, char CASTLE_CONST* filter) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxSubscriptions; ++i)
        {
            if (subscriptions_[i].used && text_equal(subscriptions_[i].client_id, client_id) && text_equal(subscriptions_[i].filter, filter))
            {
                subscriptions_[i].used = false;
                return true;
            }
        }
        return false;
    }

    void publish_message(castle::container::string_view topic, uint8_t CASTLE_CONST* payload,
                         size_type payload_size, qos quality, bool retain) CASTLE_NOEXCEPT
    {
        if (retain)
        {
            retained_entry* target = find_retained(topic);
            if (payload_size == 0U)
            {
                if (target != nullptr)
                {
                    target->used = false;
                }
            }
            else
            {
                if (target == nullptr)
                {
                    target = allocate_retained();
                }
                if (target != nullptr)
                {
                    target->used = true;
                    copy_text(target->topic, topic);
                    target->payload_size = payload_size;
                    target->quality = quality;
                    for (size_type i = 0U; i < payload_size; ++i)
                    {
                        target->payload[i] = payload[i];
                    }
                }
            }
        }
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            client_state& recipient = clients_[i];
            if (!recipient.used || recipient.client_id[0U] == '\0' || recipient.close_pending)
            {
                continue;
            }
            uint8_t best_qos = 0U;
            bool matched = false;
            for (size_type s = 0U; s < MaxSubscriptions; ++s)
            {
                subscription_entry CASTLE_CONST& sub = subscriptions_[s];
                if (sub.used && text_equal(sub.client_id, recipient.client_id) &&
                    topic_matches(castle::container::string_view(sub.filter), topic))
                {
                    matched = true;
                    uint8_t current = static_cast<uint8_t>(sub.maximum_quality);
                    if (current > best_qos)
                    {
                        best_qos = current;
                    }
                }
            }
            if (matched)
            {
                qos delivery = static_cast<uint8_t>(quality) < best_qos ? quality : static_cast<qos>(best_qos);
                (void)send_publish_to(recipient.handle, topic, payload, payload_size, delivery, false);
            }
        }
    }

    retained_entry* find_retained(castle::container::string_view topic) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxRetained; ++i)
        {
            if (retained_[i].used && text_equal(retained_[i].topic, topic))
            {
                return &retained_[i];
            }
        }
        return nullptr;
    }
    retained_entry* allocate_retained() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxRetained; ++i)
        {
            if (!retained_[i].used)
            {
                return &retained_[i];
            }
        }
        return nullptr;
    }
    qos_entry* find_qos(qos_entry* entries, uint16_t identifier) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            if (entries[i].used && entries[i].identifier == identifier)
            {
                return &entries[i];
            }
        }
        return nullptr;
    }
    qos_entry* allocate_qos(qos_entry* entries) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxOutstanding; ++i)
        {
            if (!entries[i].used)
            {
                return &entries[i];
            }
        }
        return nullptr;
    }

    castle::status send_publish_to(client_handle handle, castle::container::string_view topic,
                                   uint8_t CASTLE_CONST* payload, size_type payload_size,
                                   qos quality, bool retain) CASTLE_NOEXCEPT
    {
        client_state* state = find_client(handle);
        if (state == nullptr || topic.size() >= MaxTopicLength || payload_size > MaxPayloadSize) // LCOV_EXCL_BR_LINE
        {
            return castle::status::out_of_range;
        }
        uint16_t identifier = 0U;
        qos_entry* entry = nullptr;
        if (quality != qos::at_most_once)
        {
            entry = allocate_qos(state->outgoing);
            if (entry == nullptr)
            {
                return castle::status::full;
            }
            bool found_identifier = false;
            for (uint32_t attempt = 0U; attempt < 65535U; ++attempt) // LCOV_EXCL_BR_LINE
            {
                ++state->next_packet_identifier;
                if (state->next_packet_identifier == 0U)
                {
                    ++state->next_packet_identifier;
                }
                if (find_qos(state->outgoing, state->next_packet_identifier) == nullptr)
                {
                    identifier = state->next_packet_identifier;
                    found_identifier = true;
                    break;
                }
            }
            if (!found_identifier) // LCOV_EXCL_BR_LINE
            {
                return castle::status::full;
            }
            entry->used = true;
            entry->identifier = identifier;
            entry->quality = static_cast<uint8_t>(quality);
            entry->state = quality == qos::at_least_once ? outgoing_state::await_puback : outgoing_state::await_pubrec;
            copy_text(entry->topic, topic);
            entry->payload_size = payload_size;
            for (size_type i = 0U; i < payload_size; ++i)
            {
                entry->payload[i] = payload[i];
            }
            entry->retain = retain;
        }
        packet_writer body(tx_, MaxPacketSize);
        (void)body.put_utf8(topic);
        if (quality != qos::at_most_once)
        {
            (void)body.put_u16(identifier);
        }
        if (state->protocol_level == 5U)
        {
            (void)body.put_u8(0U);
        }
        (void)body.put_bytes(payload, payload_size);
        if (!castle::succeeded(body.status())) // LCOV_EXCL_BR_LINE
        {
            if (entry != nullptr)
            {
                entry->used = false;
            }
            return body.status();
        }
        uint8_t flags = static_cast<uint8_t>(static_cast<uint8_t>(quality) << 1U);
        if (retain)
        {
            flags |= 0x01U;
        }
        castle::status result = send_packet(handle, packet_type::publish, flags, body.data(), body.size());
        if (!castle::succeeded(result) && entry != nullptr)
        {
            entry->used = false;
        }
        return result;
    }

    castle::status send_identifier(client_handle handle, packet_type type, uint16_t identifier, bool duplicate) CASTLE_NOEXCEPT
    {
        client_state* state = find_client(handle);
        if (state == nullptr) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_found;
        }
        packet_writer body(tx_, MaxPacketSize);
        (void)body.put_u16(identifier);
        if (state->protocol_level == 5U && type == packet_type::pubrel)
        {
            (void)body.put_u8(0U); // Success reason code
            (void)body.put_u8(0U); // Property Length
        }
        uint8_t flags = type == packet_type::pubrel ? static_cast<uint8_t>(duplicate ? 0x0AU : 0x02U) : 0U; // LCOV_EXCL_BR_LINE
        return send_packet(handle, type, flags, body.data(), body.size());
    }

    castle::status send_packet(client_handle handle, packet_type type, uint8_t flags,
                               uint8_t CASTLE_CONST* body, size_type body_size) CASTLE_NOEXCEPT
    {
        size_type written = 0U;
        castle::status result = encode_packet(type, flags, body, body_size, frame_, sizeof(frame_), written);
        if (!castle::succeeded(result))
        {
            return result;
        }
        size_type sent = 0U;
        result = server_.send_all(handle, frame_, written, sent, nullptr);
        return castle::succeeded(result) && sent != written ? castle::status::data_loss : result; // LCOV_EXCL_BR_LINE
    }

    void publish_will(client_state& state) CASTLE_NOEXCEPT
    {
        if (state.will_present && state.will_pending && state.will_topic[0U] != '\0') // LCOV_EXCL_BR_LINE
        {
            publish_message(castle::container::string_view(state.will_topic), state.will_payload,
                            state.will_payload_size, state.will_quality, state.will_retain);
        }
        state.will_pending = false;
    }
    void close_state(client_state& state, bool unexpected) CASTLE_NOEXCEPT
    {
        if (unexpected && state.will_pending) // LCOV_EXCL_BR_LINE
        {
            publish_will(state);
        }
        if (state.client_id[0U] != '\0')
        {
            bool keep = state.protocol_level == 5U ? state.session_expiry_seconds != 0U : !state.clean_session;
            if (!keep)
            {
                clear_session(state.client_id);
            }
            else
            {
                for (size_type i = 0U; i < MaxClients; ++i)
                {
                    if (sessions_[i].used && text_equal(sessions_[i].client_id, state.client_id))
                    {
                        sessions_[i].expires_at_ms = state.session_expiry_seconds == 0U ? 0U : now_ms_ + state.session_expiry_seconds * 1000U;
                    }
                }
            }
        }
        state.used = false;
    }

    server_type server_{};
    client_state clients_[MaxClients]{};
    subscription_entry subscriptions_[MaxSubscriptions]{};
    retained_entry retained_[MaxRetained]{};
    session_entry sessions_[MaxClients]{};
    char expected_username_[MaxTopicLength]{};
    char expected_password_[MaxTopicLength]{};
    uint8_t tx_[MaxPacketSize]{};
    uint8_t frame_[MaxPacketSize]{};
    uint32_t now_ms_ = 0U;
    uint32_t next_assigned_id_ = 0U;
    bool authentication_enabled_ = false;
};

/** @brief Default-capacity standalone MQTT broker. */
using broker = basic_broker<>;

} // namespace mqtt
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_MQTT_BROKER_HPP
