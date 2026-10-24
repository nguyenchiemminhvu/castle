// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
#ifndef CASTLE_EXT_PROTOCOLS_HTTP_CLIENT_HPP
#define CASTLE_EXT_PROTOCOLS_HTTP_CLIENT_HPP

#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/protocols/http/http1.hpp"
#include "castle_ext/protocols/http/http2.hpp"
#include "castle_ext/net/endpoint.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace http
{

struct http1_policy {};
struct http2_policy {};

template <typename Policy>
struct protocol_policy_traits;

template <>
struct protocol_policy_traits<http1_policy>
{
    static CASTLE_INLINE CASTLE_CONSTEXPR version protocol = version::http_1_1;
    static CASTLE_INLINE CASTLE_CONSTEXPR bool multiplexed = false;
};

template <>
struct protocol_policy_traits<http2_policy>
{
    static CASTLE_INLINE CASTLE_CONSTEXPR version protocol = version::http_2;
    static CASTLE_INLINE CASTLE_CONSTEXPR bool multiplexed = true;
};

namespace detail
{
CASTLE_INLINE bool append_authority_decimal(char* out, size_type capacity, size_type& used, uint32_t value) CASTLE_NOEXCEPT
{
    char reverse[10U]; size_type count = 0U;
    do { reverse[count++] = static_cast<char>('0' + value % 10U); value /= 10U; } while (value != 0U);
    if (count > capacity - used) { return false; }
    while (count != 0U) { out[used++] = reverse[--count]; }
    return true;
}

CASTLE_INLINE bool append_authority_hex(char* out, size_type capacity, size_type& used, uint16_t value) CASTLE_NOEXCEPT
{
    static char const digits[] = "0123456789abcdef";
    char reverse[4U]; size_type count = 0U;
    do { reverse[count++] = digits[value & 0x0FU]; value = static_cast<uint16_t>(value >> 4U); } while (value != 0U);
    if (count > capacity - used) { return false; }
    while (count != 0U) { out[used++] = reverse[--count]; }
    return true;
}

CASTLE_INLINE castle::status format_authority(net::endpoint const& remote, char* output, size_type capacity) CASTLE_NOEXCEPT
{
    if (output == nullptr || capacity < 8U) { return castle::status::invalid_argument; }
    size_type used = 0U;
    if (remote.family() == net::address_family::ipv4)
    {
        uint8_t const* address = remote.address_bytes();
        for (size_type i = 0U; i < 4U; ++i)
        {
            if (!append_authority_decimal(output, capacity, used, address[i])) { return castle::status::out_of_range; }
            if (i != 3U) { if (used >= capacity) { return castle::status::out_of_range; } output[used++] = '.'; }
        }
    }
    else
    {
        if (used >= capacity) { return castle::status::out_of_range; }
        output[used++] = '[';
        uint8_t const* address = remote.address_bytes();
        for (size_type i = 0U; i < 16U; i += 2U)
        {
            uint16_t group = static_cast<uint16_t>((static_cast<uint16_t>(address[i]) << 8U) | address[i + 1U]);
            if (!append_authority_hex(output, capacity, used, group)) { return castle::status::out_of_range; }
            if (i != 14U) { if (used >= capacity) { return castle::status::out_of_range; } output[used++] = ':'; }
        }
        if (used >= capacity) { return castle::status::out_of_range; }
        output[used++] = ']';
    }
    if (used >= capacity) { return castle::status::out_of_range; }
    output[used++] = ':';
    if (!append_authority_decimal(output, capacity, used, remote.port()) || used >= capacity)
    { return castle::status::out_of_range; }
    output[used] = '\0';
    return castle::status::ok;
}

CASTLE_INLINE castle::status copy_authority(string_view value, char* output, size_type capacity) CASTLE_NOEXCEPT
{
    if (output == nullptr || value.size() >= capacity) { return castle::status::out_of_range; }
    for (size_type i = 0U; i < value.size(); ++i)
    {
        if (value[i] == '\r' || value[i] == '\n' || value[i] == ' ' || value[i] == '\t')
        { return castle::status::invalid_argument; }
        output[i] = value[i];
    }
    output[value.size()] = '\0';
    return castle::status::ok;
}
} // namespace detail

/**
 * @brief Polled HTTP/1.1 client adapter over a CASTLE TCP-client-compatible transport.
 * @tparam Transport Usually `castle::net::tcp_client<ReceiveBufferSize>`.
 *
 * The transport is passed by reference and remains owned by the caller. It must provide the
 * `connect`, `send_some`, `receive_some`, `close`, `is_open`, and `socket().finish_connect()` /
 * `socket().wait_readable()` operations exposed by CASTLE's TCP client. This dependent-type adapter
 * avoids pulling the platform FD backend into protocol-only builds and also permits deterministic tests.
 */
template <typename Transport,
          size_type MaxRequestSize = 2048U,
          size_type MaxBodySize = 1024U,
          size_type MaxHeaders = 16U,
          size_type MaxHeaderNameSize = 64U,
          size_type MaxHeaderValueSize = 256U,
          size_type MaxLineSize = 384U,
          size_type ReceiveBufferSize = 512U,
          bool EnableChunked = false,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
class http1_client
{
    static_assert(MaxRequestSize >= 64U, "HTTP request capacity is too small");
    static_assert(ReceiveBufferSize > 0U, "HTTP receive buffer must be non-zero");
public:
    using headers_type = basic_headers<MaxHeaders, MaxHeaderNameSize, MaxHeaderValueSize>;
    using parser_type = http1_response_parser<MaxBodySize, MaxHeaders, MaxHeaderNameSize,
                                               MaxHeaderValueSize, MaxLineSize, EnableChunked, CallbackStorageSize>;
    using request_type = http1_request<MaxRequestSize, MaxHeaders, MaxHeaderNameSize, MaxHeaderValueSize>;
    using response_callback = castle::callbacks::function<void(castle::status, response_view const&), CallbackStorageSize>;
    using body_callback = typename parser_type::body_callback;

    explicit http1_client(Transport& transport) CASTLE_NOEXCEPT : transport_(transport) {}

    template <typename Callback>
    castle::status set_response_callback(Callback&& callback)
    {
        response_callback next(CASTLE_FORWARD<Callback>(callback));
        if (!next) { return castle::status::invalid_callback; }
        response_callback_ = CASTLE_MOVE(next);
        return castle::status::ok;
    }

    template <typename Callback>
    castle::status set_body_callback(Callback&& callback)
    {
        return parser_.set_body_callback(CASTLE_FORWARD<Callback>(callback));
    }

    void clear_response_callback() CASTLE_NOEXCEPT { response_callback_.reset(); }
    void clear_body_callback() CASTLE_NOEXCEPT { parser_.clear_body_callback(); }

    castle::status connect(net::endpoint const& remote, string_view authority = string_view(),
                           int timeout_ms = 0, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (state_ != state::disconnected && state_ != state::ready) { return castle::status::invalid_config; }
        castle::status host_result = set_authority(authority, remote);
        if (!castle::succeeded(host_result)) { return host_result; }
        castle::status result = transport_.connect(remote, timeout_ms, true, system_error);
        if (castle::succeeded(result)) { state_ = state::ready; connected_ = true; }
        else if (result == castle::status::time_out) { state_ = state::connecting; connected_ = false; }
        else { state_ = state::disconnected; connected_ = false; }
        return result;
    }

    castle::status request(method verb, string_view path, headers_type const& headers,
                           string_view body = string_view(), string_view custom_method = string_view(),
                           bool keep_alive = true) CASTLE_NOEXCEPT
    {
        if (!connected_ || state_ != state::ready) { return castle::status::not_configured; }
        castle::status result = request_.build(verb, path, string_view(authority_), headers,
                                               body, custom_method, keep_alive);
        if (!castle::succeeded(result)) { return result; }
        parser_.reset(verb == method::head);
        bytes_sent_ = 0U;
        bool has_connection_header = false;
        bool request_connection_close = false;
        for (size_type i = 0U; i < headers.size(); ++i)
        {
            header_view field = headers[i];
            if (ascii_iequal(field.name, string_view("connection")))
            {
                has_connection_header = true;
                if (detail::contains_token(field.value, string_view("close"))) { request_connection_close = true; }
            }
        }
        request_keep_alive_ = has_connection_header ? !request_connection_close : keep_alive;
        state_ = state::sending;
        return castle::status::ok;
    }

    /** @brief Advances connect/send/receive state by one non-blocking step. */
    castle::status poll(int timeout_ms = 0, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (state_ == state::connecting)
        {
            castle::status result = transport_.socket().finish_connect(timeout_ms, system_error);
            if (!castle::succeeded(result)) { return result; }
            connected_ = true;
            state_ = state::ready;
            return castle::status::ok;
        }
        if (state_ == state::sending)
        {
            size_type sent = 0U;
            string_view remaining = request_.data().substr(bytes_sent_);
            castle::status result = transport_.send_some(remaining.data(), remaining.size(), sent, system_error);
            bytes_sent_ += sent;
            if (result == castle::status::full || result == castle::status::empty) { return result; }
            if (!castle::succeeded(result)) { return fail(result); }
            if (bytes_sent_ == request_.size()) { state_ = state::receiving; }
            else { return castle::status::ok; }
        }
        if (state_ == state::receiving)
        {
            castle::status ready = transport_.socket().wait_readable(timeout_ms, system_error);
            if (!castle::succeeded(ready)) { return ready; }
            size_type received = 0U;
            bool peer_closed = false;
            castle::status result = transport_.receive_some(receive_buffer_, ReceiveBufferSize,
                                                             received, system_error, &peer_closed);
            if (!castle::succeeded(result) && result != castle::status::empty) { return fail(result); }
            if (received != 0U || peer_closed)
            {
                result = parser_.feed(receive_buffer_, received, peer_closed);
                if (!castle::succeeded(result)) { return fail(result); }
                if (parser_.complete()) { return complete_response(); }
            }
            return peer_closed ? fail(castle::status::data_loss) : result;
        }
        return state_ == state::ready ? castle::status::ok : castle::status::not_configured;
    }

    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        state_ = state::disconnected;
        connected_ = false;
        return transport_.close(system_error);
    }

    bool connected() CASTLE_CONST CASTLE_NOEXCEPT { return connected_; }
    bool busy() CASTLE_CONST CASTLE_NOEXCEPT { return state_ == state::sending || state_ == state::receiving || state_ == state::connecting; }
    error_code last_error() CASTLE_CONST CASTLE_NOEXCEPT { return parser_.last_error(); }
    response_view response() CASTLE_CONST CASTLE_NOEXCEPT { return parser_.response(); }
    Transport& transport() CASTLE_NOEXCEPT { return transport_; }

private:
    enum class state : uint8_t { disconnected, connecting, ready, sending, receiving, failed };

    castle::status set_authority(string_view value, net::endpoint const& remote) CASTLE_NOEXCEPT
    {
        if (value.empty()) { return make_numeric_authority(remote); }
        if (value.size() >= sizeof(authority_)) { return castle::status::out_of_range; }
        for (size_type i = 0U; i < value.size(); ++i)
        {
            if (value[i] == '\r' || value[i] == '\n' || value[i] == ' ' || value[i] == '\t')
            { return castle::status::invalid_argument; }
            authority_[i] = value[i];
        }
        authority_[value.size()] = '\0';
        return castle::status::ok;
    }

    castle::status make_numeric_authority(net::endpoint const& remote) CASTLE_NOEXCEPT
    {
        size_type used = 0U;
        if (remote.family() == net::address_family::ipv4)
        {
            uint8_t const* address = remote.address_bytes();
            for (size_type i = 0U; i < 4U; ++i)
            {
                if (!append_decimal(authority_, sizeof(authority_), used, address[i])) { return castle::status::out_of_range; }
                if (i != 3U && used < sizeof(authority_)) { authority_[used++] = '.'; }
            }
        }
        else
        {
            if (used < sizeof(authority_)) { authority_[used++] = '['; } else { return castle::status::out_of_range; }
            uint8_t const* address = remote.address_bytes();
            for (size_type i = 0U; i < 16U; i += 2U)
            {
                uint16_t group = static_cast<uint16_t>((static_cast<uint16_t>(address[i]) << 8U) | address[i + 1U]);
                if (!append_hex(authority_, sizeof(authority_), used, group)) { return castle::status::out_of_range; }
                if (i != 14U) { authority_[used++] = ':'; }
            }
            authority_[used++] = ']';
        }
        if (used >= sizeof(authority_)) { return castle::status::out_of_range; }
        authority_[used++] = ':';
        if (!append_decimal(authority_, sizeof(authority_), used, remote.port())) { return castle::status::out_of_range; }
        authority_[used] = '\0';
        return castle::status::ok;
    }

    static bool append_decimal(char* out, size_type capacity, size_type& used, uint32_t value) CASTLE_NOEXCEPT
    {
        char reverse[10U]; size_type count = 0U;
        do { reverse[count++] = static_cast<char>('0' + value % 10U); value /= 10U; } while (value != 0U);
        if (count > capacity - used) { return false; }
        while (count != 0U) { out[used++] = reverse[--count]; }
        return true;
    }

    static bool append_hex(char* out, size_type capacity, size_type& used, uint16_t value) CASTLE_NOEXCEPT
    {
        static char const digits[] = "0123456789abcdef";
        char reverse[4U]; size_type count = 0U;
        do { reverse[count++] = digits[value & 0x0FU]; value = static_cast<uint16_t>(value >> 4U); } while (value != 0U);
        if (count > capacity - used) { return false; }
        while (count != 0U) { out[used++] = reverse[--count]; }
        return true;
    }

    castle::status complete_response() CASTLE_NOEXCEPT
    {
        state_ = state::ready;
        response_view view = parser_.response();
        if (!view.keep_alive || !request_keep_alive_)
        {
            (void)transport_.close(nullptr);
            connected_ = false;
            state_ = state::disconnected;
        }
        if (response_callback_) { response_callback_(castle::status::ok, view); }
        return castle::status::ok;
    }

    castle::status fail(castle::status result) CASTLE_NOEXCEPT
    {
        state_ = state::failed;
        connected_ = false;
        (void)transport_.close(nullptr);
        if (response_callback_)
        {
            response_view view = parser_.response();
            response_callback_(result, view);
        }
        return result;
    }

    Transport& transport_;
    request_type request_{};
    parser_type parser_{};
    response_callback response_callback_{};
    char authority_[64U]{};
    uint8_t receive_buffer_[ReceiveBufferSize]{};
    size_type bytes_sent_ = 0U;
    state state_ = state::disconnected;
    bool connected_ = false;
    bool request_keep_alive_ = true;
};

/**
 * @brief Polled HTTP/2 client adapter over a CASTLE TCP-client-compatible transport.
 * @note The underlying session uses prior knowledge on plain TCP, sends the 24-byte preface and SETTINGS,
 *       and never emits an HTTP/1.1 Upgrade exchange.
 */
template <typename Transport,
          size_type MaxConcurrentStreams = 4U,
          size_type MaxHeaderBlockSize = 2048U,
          size_type MaxFrameSize = 16384U,
          size_type MaxBodySize = 1024U,
          size_type MaxHeaders = 24U,
          size_type MaxHeaderNameSize = 64U,
          size_type MaxHeaderValueSize = 256U,
          size_type MaxDynamicTableBytes = 0U,
          size_type MaxDynamicEntries = (MaxDynamicTableBytes / 32U) + 1U,
          size_type MaxOutputSize = 8192U,
          size_type ReceiveBufferSize = 512U,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
class http2_client
{
public:
    using session_type = http2_session<MaxConcurrentStreams, MaxHeaderBlockSize, MaxFrameSize,
        MaxBodySize, MaxHeaders, MaxHeaderNameSize, MaxHeaderValueSize,
        MaxDynamicTableBytes, MaxDynamicEntries, MaxOutputSize, CallbackStorageSize>;
    using headers_type = typename session_type::headers_type;

    explicit http2_client(Transport& transport) CASTLE_NOEXCEPT : transport_(transport) {}

    template <typename Callback>
    castle::status set_response_callback(Callback&& callback) { return session_.set_response_callback(CASTLE_FORWARD<Callback>(callback)); }
    template <typename Callback>
    castle::status set_body_callback(Callback&& callback) { return session_.set_body_callback(CASTLE_FORWARD<Callback>(callback)); }
    void clear_response_callback() CASTLE_NOEXCEPT { session_.clear_response_callback(); }
    void clear_body_callback() CASTLE_NOEXCEPT { session_.clear_body_callback(); }

    castle::status connect(net::endpoint const& remote, int timeout_ms = 0,
                           int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::status formatted = detail::format_authority(remote, authority_, sizeof(authority_));
        if (!castle::succeeded(formatted)) { return formatted; }
        return connect_transport(remote, timeout_ms, system_error);
    }

    castle::status connect(net::endpoint const& remote, string_view authority, int timeout_ms = 0,
                           int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::status copied = detail::copy_authority(authority, authority_, sizeof(authority_));
        if (!castle::succeeded(copied)) { return copied; }
        return connect_transport(remote, timeout_ms, system_error);
    }

private:
    castle::status connect_transport(net::endpoint const& remote, int timeout_ms,
                                     int* system_error) CASTLE_NOEXCEPT
    {
        castle::status result = transport_.connect(remote, timeout_ms, true, system_error);
        if (castle::succeeded(result))
        {
            connected_ = true;
            return session_.start();
        }
        if (result == castle::status::time_out) { connecting_ = true; }
        return result;
    }

public:
    castle::status open_stream(method verb, string_view path, headers_type const& headers,
                               string_view body, uint32_t& stream_id,
                               string_view custom_method = string_view()) CASTLE_NOEXCEPT
    {
        return open_stream(verb, path, string_view(authority_), headers, body, stream_id, custom_method);
    }

    castle::status open_stream(method verb, string_view path, string_view authority,
                               headers_type const& headers, string_view body,
                               uint32_t& stream_id, string_view custom_method = string_view()) CASTLE_NOEXCEPT
    {
        if (!connected_) { return castle::status::not_configured; }
        string_view effective_authority = authority.empty() ? string_view(authority_) : authority;
        return session_.open_stream(verb, path, effective_authority, headers, body, stream_id, custom_method);
    }

    /** @brief Pumps pending output and one readable receive event without busy waiting. */
    castle::status poll(int timeout_ms = 0, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (connecting_)
        {
            castle::status result = transport_.socket().finish_connect(timeout_ms, system_error);
            if (!castle::succeeded(result)) { return result; }
            connecting_ = false;
            connected_ = true;
            result = session_.start();
            if (!castle::succeeded(result)) { return result; }
        }
        if (!connected_) { return castle::status::not_configured; }
        castle::status sent = flush_output(system_error);
        bool const write_blocked = sent == castle::status::full || sent == castle::status::empty;
        if (!castle::succeeded(sent) && !write_blocked) { return sent; }
        // A blocked write must not prevent reads: the peer may need to send WINDOW_UPDATE,
        // SETTINGS or response DATA before the queued request bytes can make progress.
        castle::status ready = transport_.socket().wait_readable(timeout_ms, system_error);
        if (!castle::succeeded(ready)) { return ready; }
        size_type received = 0U;
        bool peer_closed = false;
        castle::status result = transport_.receive_some(receive_buffer_, ReceiveBufferSize,
                                                         received, system_error, &peer_closed);
        if (!castle::succeeded(result) && result != castle::status::empty) { return result; }
        if (received != 0U || peer_closed)
        {
            result = session_.feed(receive_buffer_, received, peer_closed);
            if (!castle::succeeded(result))
            {
                connected_ = false;
                (void)transport_.close(nullptr);
                return result;
            }
            return flush_output(system_error);
        }
        return result;
    }

    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        connected_ = false; connecting_ = false;
        return transport_.close(system_error);
    }

    bool connected() CASTLE_CONST CASTLE_NOEXCEPT { return connected_; }
    session_type& session() CASTLE_NOEXCEPT { return session_; }
    session_type const& session() CASTLE_CONST CASTLE_NOEXCEPT { return session_; }
    response_view response(uint32_t stream_id) CASTLE_CONST CASTLE_NOEXCEPT { return session_.response(stream_id); }
    Transport& transport() CASTLE_NOEXCEPT { return transport_; }

private:
    castle::status flush_output(int* system_error) CASTLE_NOEXCEPT
    {
        while (session_.output_size() != 0U)
        {
            uint8_t const* data = session_.pending_output();
            size_type size = session_.pending_output_size();
            size_type sent = 0U;
            castle::status result = transport_.send_some(data, size, sent, system_error);
            if (sent != 0U)
            {
                castle::status consumed = session_.consume_output(sent);
                if (!castle::succeeded(consumed)) { return consumed; }
            }
            if (result == castle::status::full || result == castle::status::empty) { return result; }
            if (!castle::succeeded(result)) { return result; }
            if (sent == 0U) { return castle::status::empty; }
        }
        return castle::status::ok;
    }

    Transport& transport_;
    session_type session_{};
    uint8_t receive_buffer_[ReceiveBufferSize]{};
    bool connected_ = false;
    bool connecting_ = false;
    char authority_[64U]{};
};

template <typename Transport, typename Policy = http1_policy>
struct client_type;

template <typename Transport>
struct client_type<Transport, http1_policy> { using type = http1_client<Transport>; };

template <typename Transport>
struct client_type<Transport, http2_policy> { using type = http2_client<Transport>; };

template <typename Transport, typename Policy = http1_policy>
using basic_http_client = typename client_type<Transport, Policy>::type;

} // namespace http
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_HTTP_CLIENT_HPP
