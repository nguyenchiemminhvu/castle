// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file client.hpp
 * @brief Fixed-buffer DNS resolver over CASTLE UDP and TCP client transports.
 */
#ifndef CASTLE_EXT_PROTOCOLS_DNS_CLIENT_HPP
#define CASTLE_EXT_PROTOCOLS_DNS_CLIENT_HPP

#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle/container/string_view.hpp"
#include "castle_ext/net/client.hpp"
#include "castle_ext/net/detail/callback_dispatcher.hpp"
#include "castle_ext/protocols/dns/message.hpp"
#include "castle_ext/protocols/dns/types.hpp"

#include <stdint.h>
#include <limits.h>

namespace castle
{
namespace protocols
{
namespace dns
{

/**
 * @brief DNS resolver using caller-owned CASTLE UDP/TCP client transports.
 *
 * The transport types must expose the operations of `castle::net::udp_client` and
 * `castle::net::tcp_client`. No socket is created until the first query. One instance
 * supports one outstanding operation and is not internally synchronized.
 *
 * @tparam UdpTransport UDP client type, normally `castle::net::udp_client<512U>`.
 * @tparam TcpTransport TCP client type, normally `castle::net::tcp_client<2048U>`.
 * @tparam MaxRecords Maximum retained records in a response.
 * @tparam TcpMessageCapacity Maximum DNS-over-TCP response body accepted by this instance.
 * @tparam CallbackStorageSize Inline callback storage.
 */
template <typename UdpTransport = net::udp_client<512U>,
          typename TcpTransport = net::tcp_client<2048U>,
          size_type MaxRecords = 8U,
          size_type TcpMessageCapacity = 2048U,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
class basic_client
{
    static_assert(TcpMessageCapacity >= message::header_size, "DNS TCP message capacity must hold a DNS header");
    static_assert(TcpMessageCapacity <= message::max_message_size, "DNS TCP capacity exceeds the DNS wire limit");

public:
    using response_type = response<MaxRecords>;
    using callback_type = castle::callbacks::function<void(castle::status, response_type CASTLE_CONST&), CallbackStorageSize>;

    /** @brief Creates a resolver borrowing UDP and TCP transports; both must outlive it. */
    basic_client(UdpTransport& udp, TcpTransport& tcp, client_config config = client_config()) CASTLE_NOEXCEPT
        : udp_(udp), tcp_(tcp), config_(config), next_id_(config.transaction_id_seed)
    {
    }

    basic_client(CASTLE_CONST basic_client&) CASTLE_DELETE;
    basic_client& operator=(CASTLE_CONST basic_client&) CASTLE_DELETE;

    /** @brief Replaces the resolver retry policy. Zero timeout is treated as a one-millisecond wait. */
    void configure(client_config config) CASTLE_NOEXCEPT
    {
        config_ = config;
        next_id_ = config.transaction_id_seed;
        if (config_.timeout_ms == 0U)
        {
            config_.timeout_ms = 1U;
        }
    }

    /** @brief Installs an asynchronous completion callback. Signature: `void(status, response CASTLE_CONST&)`. */
    template <typename Callback>
    castle::status set_callback(Callback&& callback)
    {
        callback_type next(CASTLE_FORWARD<Callback>(callback));
        if (!next)
        {
            return castle::status::invalid_callback;
        }
        dispatcher_.replace(callback_, CASTLE_MOVE(next));
        return castle::status::ok;
    }

    /** @brief Clears the asynchronous completion callback. */
    void clear_callback() CASTLE_NOEXCEPT { dispatcher_.clear(callback_); }

    /** @brief Returns true while an asynchronous request or its callback is active. */
    bool busy() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_ != async_state::idle || dispatcher_.is_dispatching();
    }

    /** @brief Returns the most recently completed asynchronous response. */
    response_type CASTLE_CONST& last_response() CASTLE_CONST CASTLE_NOEXCEPT { return response_; }

    /** @brief Returns the status delivered by the most recent asynchronous completion. */
    castle::status last_status() CASTLE_CONST CASTLE_NOEXCEPT { return last_status_; }

    /**
     * @brief Performs a blocking DNS lookup, using UDP and transparently retrying over TCP on truncation.
     * @param nameserver Numeric resolver endpoint; port zero selects port 53.
     * @param name Domain name to query.
     * @param type Requested resource-record type.
     * @param output Caller-owned fixed-capacity response destination.
     * @param system_error Optional native errno output.
     * @return `ok`, `not_found` for NXDOMAIN, `time_out`, `full`, or the underlying socket/codec status.
     */
    castle::status resolve(net::endpoint CASTLE_CONST& nameserver,
                           castle::container::string_view name,
                           record_type type,
                           response_type& output,
                           int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (busy())
        {
            return castle::status::invalid_config;
        }
        output.clear();
        net::endpoint peer = dns_peer(nameserver);
        uint16_t id = next_id_++;
        size_type query_size = 0U;
        castle::status result = message::encode_query(name, type, id, query_, sizeof(query_), query_size);
        if (!castle::succeeded(result))
        {
            return result;
        }

        result = prepare_udp(peer.family(), false, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }

        for (uint16_t attempt = 0U; attempt <= config_.retries; ++attempt)
        {
            size_type sent = 0U;
            result = udp_.send_to(query_, query_size, peer, sent, system_error);
            if (!castle::succeeded(result))
            {
                return result;
            }
            if (sent != query_size)
            {
                return castle::status::data_loss;
            }

            result = udp_.socket().wait_readable(static_cast<int>(effective_timeout()), system_error);
            if (result == castle::status::time_out)
            {
                continue;
            }
            if (!castle::succeeded(result))
            {
                return result;
            }

            size_type received = 0U;
            net::endpoint source;
            result = udp_.receive_from(udp_packet_, sizeof(udp_packet_), received, source, system_error);
            if (result == castle::status::data_loss)
            {
                if (source != peer)
                {
                    continue;
                }
                return resolve_tcp(peer, name, type, id, query_size, output, system_error);
            }
            if (!castle::succeeded(result))
            {
                if (result == castle::status::empty)
                {
                    continue;
                }
                return result;
            }
            if (source != peer)
            {
                continue;
            }
            if (received > message::classic_udp_limit || message::is_truncated_packet(udp_packet_, received))
            {
                return resolve_tcp(peer, name, type, id, query_size, output, system_error);
            }
            return parse_and_validate(udp_packet_, received, id, name, type, output);
        }
        return castle::status::time_out;
    }

    /**
     * @brief Starts a non-blocking lookup. Call `poll(now_ms)` until `busy()` becomes false.
     * @param nameserver Numeric DNS resolver endpoint; port zero selects 53.
     * @param name Domain name copied into the encoded request before returning.
     * @param type Query type.
     * @param now_ms Monotonic millisecond tick supplied by the application.
     * @return `ok` when started; otherwise a validation or socket error.
     */
    castle::status start_resolve(net::endpoint CASTLE_CONST& nameserver,
                                 castle::container::string_view name,
                                 record_type type,
                                 uint32_t now_ms,
                                 int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (busy())
        {
            return castle::status::invalid_config;
        }
        response_.clear();
        server_ = dns_peer(nameserver);
        query_id_ = next_id_++;
        castle::status result = message::encode_query(name, type, query_id_, query_, sizeof(query_), query_size_);
        if (!castle::succeeded(result))
        {
            return result;
        }
        // LCOV_EXCL_START
        // encode_query() already caps names below this size, so the bound cannot trigger.
        if (name.size() >= sizeof(async_name_storage_))
        {
            return castle::status::out_of_range;
        }
        // LCOV_EXCL_STOP
        for (size_type i = 0U; i < name.size(); ++i)
        {
            async_name_storage_[i] = name[i];
        }
        async_name_storage_[name.size()] = '\0';
        async_name_ = castle::container::string_view(async_name_storage_, name.size());
        result = prepare_udp(server_.family(), true, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
        result = send_udp_query(system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
        attempts_ = 1U;
        last_attempt_ms_ = now_ms;
        async_type_ = type;
        state_ = async_state::udp_wait;
        last_status_ = castle::status::empty;
        return castle::status::ok;
    }

    /**
     * @brief Advances one step of the asynchronous state machine without blocking.
     * @param now_ms Monotonic millisecond tick from the same clock used by `start_resolve()`.
     * @return `empty` while waiting, `ok` after progress/success, or the completed/error status.
     */
    castle::status poll(uint32_t now_ms, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (dispatcher_.is_dispatching())
        {
            return castle::status::invalid_config;
        }
        if (state_ == async_state::idle)
        {
            return castle::status::not_configured;
        }
        if (state_ == async_state::udp_wait)
        {
            size_type received = 0U;
            net::endpoint source;
            castle::status result = udp_.receive_from(udp_packet_, sizeof(udp_packet_), received, source, system_error);
            if (result == castle::status::data_loss)
            {
                if (source != server_)
                {
                    return castle::status::empty;
                }
                return begin_tcp_async(now_ms, system_error);
            }
            if (result == castle::status::empty)
            {
                if (elapsed(now_ms, last_attempt_ms_) < effective_timeout())
                {
                    return castle::status::empty;
                }
                if (attempts_ <= config_.retries)
                {
                    result = send_udp_query(system_error);
                    if (!castle::succeeded(result))
                    {
                        return complete(result);
                    }
                    ++attempts_;
                    last_attempt_ms_ = now_ms;
                    return castle::status::ok;
                }
                return complete(castle::status::time_out);
            }
            if (!castle::succeeded(result))
            {
                return complete(result);
            }
            if (source != server_)
            {
                return castle::status::empty;
            }
            if (received > message::classic_udp_limit || message::is_truncated_packet(udp_packet_, received))
            {
                return begin_tcp_async(now_ms, system_error);
            }
            return complete(parse_and_validate(udp_packet_, received, query_id_, async_name_, async_type_, response_));
        }
        if (state_ >= async_state::tcp_connect && elapsed(now_ms, tcp_started_ms_) >= effective_timeout()) // LCOV_EXCL_BR_LINE
        {
            return complete(castle::status::time_out);
        }
        if (state_ == async_state::tcp_connect)
        {
            castle::status result = tcp_.socket().finish_connect(0, system_error);
            if (result == castle::status::time_out || result == castle::status::full || result == castle::status::empty)
            {
                return castle::status::empty;
            }
            if (!castle::succeeded(result))
            {
                return complete(result);
            }
            state_ = async_state::tcp_send;
            return castle::status::ok;
        }
        if (state_ == async_state::tcp_send)
        {
            size_type sent = 0U;
            castle::status result = tcp_.send_some(tcp_query_ + tcp_query_offset_,
                                                   tcp_query_size_ - tcp_query_offset_, sent, system_error);
            tcp_query_offset_ += sent;
            if (result == castle::status::full || result == castle::status::empty)
            {
                return castle::status::empty;
            }
            if (!castle::succeeded(result))
            {
                return complete(result);
            }
            if (tcp_query_offset_ == tcp_query_size_)
            {
                tcp_prefix_offset_ = 0U;
                state_ = async_state::tcp_prefix;
            }
            return castle::status::ok;
        }
        if (state_ == async_state::tcp_prefix)
        {
            castle::status result = receive_async_bytes(tcp_prefix_ + tcp_prefix_offset_,
                                                         2U - tcp_prefix_offset_, tcp_prefix_offset_, system_error);
            if (result == castle::status::empty)
            {
                return result;
            }
            if (!castle::succeeded(result))
            {
                return complete(result);
            }
            if (tcp_prefix_offset_ == 2U)
            {
                tcp_message_size_ = static_cast<size_type>((static_cast<uint16_t>(tcp_prefix_[0U]) << 8U) | tcp_prefix_[1U]);
                if (tcp_message_size_ < message::header_size || tcp_message_size_ > TcpMessageCapacity)
                {
                    return complete(castle::status::full);
                }
                tcp_message_offset_ = 0U;
                state_ = async_state::tcp_body;
            }
            return castle::status::ok;
        }
        if (state_ == async_state::tcp_body) // LCOV_EXCL_BR_LINE
        {
            castle::status result = receive_async_bytes(tcp_message_ + tcp_message_offset_,
                                                         tcp_message_size_ - tcp_message_offset_,
                                                         tcp_message_offset_, system_error);
            if (result == castle::status::empty)
            {
                return result;
            }
            if (!castle::succeeded(result))
            {
                return complete(result);
            }
            if (tcp_message_offset_ == tcp_message_size_)
            {
                return complete(parse_and_validate(tcp_message_, tcp_message_size_, query_id_,
                                                    async_name_, async_type_, response_));
            }
            return castle::status::ok;
        }
        // LCOV_EXCL_START
        // Every async_state is handled above, so the state machine cannot reach this fallback.
        return complete(castle::status::invalid_config);
        // LCOV_EXCL_STOP
    }

private:
    enum class async_state : uint8_t
    {
        idle,
        udp_wait,
        tcp_connect,
        tcp_send,
        tcp_prefix,
        tcp_body
    };

    static uint32_t elapsed(uint32_t now, uint32_t then) CASTLE_NOEXCEPT { return now - then; }

    uint32_t effective_timeout() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (config_.timeout_ms == 0U)
        {
            return 1U;
        }
        CASTLE_CONST uint32_t maximum = static_cast<uint32_t>(INT_MAX);
        return config_.timeout_ms > maximum ? maximum : config_.timeout_ms;
    }

    static net::endpoint dns_peer(net::endpoint CASTLE_CONST& input) CASTLE_NOEXCEPT
    {
        uint16_t port = input.port() == 0U ? 53U : input.port();
        if (input.family() == net::address_family::ipv4)
        {
            CASTLE_CONST uint8_t* a = input.address_bytes();
            return net::endpoint::ipv4(a[0U], a[1U], a[2U], a[3U], port);
        }
        return net::endpoint::ipv6(input.address_bytes(), port, input.scope_id());
    }

    castle::status prepare_udp(net::address_family family, bool nonblocking, int* system_error) CASTLE_NOEXCEPT
    {
        if (udp_.is_open() && udp_.socket().family() != family)
        {
            castle::status closed = udp_.close(system_error);
            if (!castle::succeeded(closed) && closed != castle::status::not_configured)
            {
                return closed;
            }
        }
        if (!udp_.is_open())
        {
            return udp_.open(family, nonblocking, system_error);
        }
        return udp_.set_nonblocking(nonblocking, system_error);
    }

    castle::status send_udp_query(int* system_error) CASTLE_NOEXCEPT
    {
        size_type sent = 0U;
        castle::status result = udp_.send_to(query_, query_size_, server_, sent, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
        return sent == query_size_ ? castle::status::ok : castle::status::data_loss;
    }

    template <size_type N>
    castle::status parse_and_validate(CASTLE_CONST uint8_t* data,
                                      size_type size,
                                      uint16_t id,
                                      castle::container::string_view expected_name,
                                      record_type expected_type,
                                      response<N>& output) CASTLE_NOEXCEPT
    {
        castle::status result = message::parse_response(data, size, id, output);
        if (!castle::succeeded(result))
        {
            return result;
        }
        if (!same_name(expected_name, output.question_name) ||
            output.question_type != expected_type || output.question_class != record_class::internet)
        {
            return castle::status::invalid_argument;
        }
        if (output.truncated)
        {
            return castle::status::data_loss;
        }
        if (output.rcode == response_code::name_error)
        {
            return castle::status::not_found;
        }
        if (output.rcode == response_code::format_error)
        {
            return castle::status::invalid_argument;
        }
        if (output.rcode != response_code::no_error)
        {
            return castle::status::unknown_error;
        }
        return castle::status::ok;
    }

    static bool same_name(castle::container::string_view left, CASTLE_CONST char* right) CASTLE_NOEXCEPT
    {
        size_type left_size = left.size();
        size_type right_size = 0U;
        while (right[right_size] != '\0')
        {
            ++right_size;
        }
        if (left_size != 0U && left[left_size - 1U] == '.') // LCOV_EXCL_BR_LINE
        {
            --left_size;
        }
        if (left_size != right_size)
        {
            return false;
        }
        for (size_type i = 0U; i < left_size; ++i)
        {
            char a = left[i];
            char b = right[i];
            if (a >= 'A' && a <= 'Z')
            {
                a = static_cast<char>(a - 'A' + 'a');
            }
            if (b >= 'A' && b <= 'Z')
            {
                b = static_cast<char>(b - 'A' + 'a');
            }
            if (a != b)
            {
                return false;
            }
        }
        return true;
    }

    castle::status resolve_tcp(net::endpoint CASTLE_CONST& peer,
                               castle::container::string_view name,
                               record_type type,
                               uint16_t id,
                               size_type query_size,
                               response_type& output,
                               int* system_error) CASTLE_NOEXCEPT
    {
        if (tcp_.is_open())
        {
            (void)tcp_.close(system_error);
        }
        castle::status result = tcp_.connect(peer, static_cast<int>(effective_timeout()), false, system_error);
        if (result == castle::status::time_out)
        {
            result = tcp_.socket().finish_connect(static_cast<int>(effective_timeout()), system_error);
        }
        if (!castle::succeeded(result))
        {
            (void)tcp_.close(nullptr);
            return result;
        }

        uint8_t framed_query[514U];
        framed_query[0U] = static_cast<uint8_t>(query_size >> 8U);
        framed_query[1U] = static_cast<uint8_t>(query_size & 0xFFU);
        for (size_type i = 0U; i < query_size; ++i)
        {
            framed_query[i + 2U] = query_[i];
        }
        size_type sent = 0U;
        result = tcp_.send_all(framed_query, query_size + 2U, sent, system_error);
        if (!castle::succeeded(result) || sent != query_size + 2U)
        {
            (void)tcp_.close(nullptr);
            return castle::succeeded(result) ? castle::status::data_loss : result;
        }

        uint8_t prefix[2U]{};
        result = receive_exact(prefix, sizeof(prefix), static_cast<int>(effective_timeout()), system_error);
        if (!castle::succeeded(result))
        {
            (void)tcp_.close(nullptr);
            return result;
        }
        size_type body_size = static_cast<size_type>((static_cast<uint16_t>(prefix[0U]) << 8U) | prefix[1U]);
        if (body_size < message::header_size || body_size > TcpMessageCapacity)
        {
            (void)tcp_.close(nullptr);
            return castle::status::full;
        }
        result = receive_exact(tcp_message_, body_size, static_cast<int>(effective_timeout()), system_error);
        (void)tcp_.close(nullptr);
        if (!castle::succeeded(result))
        {
            return result;
        }
        return parse_and_validate(tcp_message_, body_size, id, name, type, output);
    }

    castle::status receive_exact(void* output, size_type wanted, int timeout_ms, int* system_error) CASTLE_NOEXCEPT
    {
        size_type offset = 0U;
        while (offset < wanted)
        {
            castle::status ready = tcp_.socket().wait_readable(timeout_ms, system_error);
            if (!castle::succeeded(ready))
            {
                return ready;
            }
            size_type received = 0U;
            bool peer_closed = false;
            castle::status result = tcp_.receive_some(static_cast<uint8_t*>(output) + offset,
                                                       wanted - offset, received, system_error, &peer_closed);
            offset += received;
            if (peer_closed)
            {
                return castle::status::data_loss;
            }
            if (!castle::succeeded(result) && result != castle::status::empty)
            {
                return result;
            }
            if (received == 0U && result == castle::status::empty)
            {
                return castle::status::time_out;
            }
        }
        return castle::status::ok;
    }

    castle::status begin_tcp_async(uint32_t now_ms, int* system_error) CASTLE_NOEXCEPT
    {
        tcp_started_ms_ = now_ms;
        if (tcp_.is_open())
        {
            (void)tcp_.close(nullptr);
        }
        tcp_query_[0U] = static_cast<uint8_t>(query_size_ >> 8U);
        tcp_query_[1U] = static_cast<uint8_t>(query_size_ & 0xFFU);
        for (size_type i = 0U; i < query_size_; ++i)
        {
            tcp_query_[i + 2U] = query_[i];
        }
        tcp_query_size_ = query_size_ + 2U;
        tcp_query_offset_ = 0U;
        castle::status result = tcp_.connect(server_, 0, true, system_error);
        if (result == castle::status::time_out)
        {
            state_ = async_state::tcp_connect;
            return castle::status::ok;
        }
        if (!castle::succeeded(result))
        {
            return complete(result);
        }
        state_ = async_state::tcp_send;
        return castle::status::ok;
    }

    castle::status receive_async_bytes(uint8_t* output, size_type wanted, size_type& offset,
                                       int* system_error) CASTLE_NOEXCEPT
    {
        size_type received = 0U;
        bool peer_closed = false;
        castle::status result = tcp_.receive_some(output, wanted, received, system_error, &peer_closed);
        offset += received;
        if (peer_closed)
        {
            return castle::status::data_loss;
        }
        if (result == castle::status::empty || result == castle::status::full)
        {
            return castle::status::empty;
        }
        if (!castle::succeeded(result))
        {
            return result;
        }
        return castle::status::ok;
    }

    castle::status complete(castle::status result) CASTLE_NOEXCEPT
    {
        bool close_tcp = state_ >= async_state::tcp_connect;
        state_ = async_state::idle;
        if (close_tcp && tcp_.is_open())
        {
            (void)tcp_.close(nullptr);
        }
        last_status_ = result;
        if (callback_)
        {
            response_type snapshot = response_;
            dispatcher_.invoke(callback_, result, snapshot);
        }
        return result;
    }

    UdpTransport& udp_;
    TcpTransport& tcp_;
    client_config config_;
    uint16_t next_id_;
    uint16_t query_id_ = 0U;
    uint8_t query_[512U]{};
    size_type query_size_ = 0U;
    uint8_t udp_packet_[message::classic_udp_limit + 1U]{};
    uint8_t tcp_message_[TcpMessageCapacity]{};
    uint8_t tcp_query_[514U]{};
    size_type tcp_query_size_ = 0U;
    size_type tcp_query_offset_ = 0U;
    uint8_t tcp_prefix_[2U]{};
    size_type tcp_prefix_offset_ = 0U;
    size_type tcp_message_size_ = 0U;
    size_type tcp_message_offset_ = 0U;
    net::endpoint server_{};
    char async_name_storage_[256U]{};
    castle::container::string_view async_name_{};
    record_type async_type_ = record_type::a;
    uint16_t attempts_ = 0U;
    uint32_t last_attempt_ms_ = 0U;
    uint32_t tcp_started_ms_ = 0U;
    async_state state_ = async_state::idle;
    response_type response_{};
    castle::status last_status_ = castle::status::not_configured;
    callback_type callback_{};
    net::detail::callback_dispatcher<callback_type> dispatcher_{};
};

/** @brief Default resolver over CASTLE's 512-byte UDP and 2048-byte TCP clients. */
using client = basic_client<>;

} // namespace dns
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_DNS_CLIENT_HPP
