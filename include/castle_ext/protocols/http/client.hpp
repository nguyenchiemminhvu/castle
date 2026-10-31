// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file client.hpp
 * @brief Polled HTTP/1.1 and HTTP/2 clients over a CASTLE TCP client.
 *
 * Both clients borrow a transport such as `castle::net::tcp_client`, never allocate, and report completion
 * through a `void(castle::status, response_view const&)` callback. `protocol_policy_traits` selects the
 * client type at compile time, so no virtual dispatch is involved.
 *
 * @code
 * using namespace castle::protocols::http;
 *
 * castle::net::tcp_client<512U> tcp;
 * http1_client<castle::net::tcp_client<512U>> client(tcp);
 * client.set_response_callback([](castle::status result, response_view const& response)
 * {
 *     // response.status_code, response.headers and response.body are valid inside the callback.
 * });
 *
 * client.connect(castle::net::endpoint::ipv4(192U, 0U, 2U, 1U, 80U), "api.local:80");
 * basic_headers<> headers;
 * client.request(method::get, "/status", headers);
 * while (client.busy())
 * {
 *     client.poll(10);
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_HTTP_CLIENT_HPP
#define CASTLE_EXT_PROTOCOLS_HTTP_CLIENT_HPP

#include "castle/core/config.hpp"

#if CASTLE_USING_BSD_SOCKET_BASED_HTTP_PROTOCOL

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

namespace detail
{
/** @brief Fixed Host / `:authority` text: caller supplied, or the numeric `address:port` of the remote endpoint. */
class authority_buffer
{
public:
    /** @brief Formats `remote` as `a.b.c.d:port` or `[x:x:...]:port`; the result always fits. */
    void assign(net::endpoint CASTLE_CONST& remote) CASTLE_NOEXCEPT
    {
        size_ = 0U;
        uint8_t CASTLE_CONST* address = remote.address_bytes();
        if (remote.family() == net::address_family::ipv4)
        {
            for (size_type i = 0U; i < 4U; ++i)
            {
                append_decimal(address[i]);
                if (i != 3U)
                {
                    append('.');
                }
            }
        }
        else
        {
            append('[');
            for (size_type i = 0U; i < 16U; i += 2U)
            {
                uint16_t group = static_cast<uint16_t>((static_cast<uint16_t>(address[i]) << 8U) | address[i + 1U]);
                append_hex(group);
                if (i != 14U)
                {
                    append(':');
                }
            }
            append(']');
        }
        append(':');
        append_decimal(remote.port());
        text_[size_] = '\0';
    }

    /**
     * @brief Copies a caller-supplied authority.
     *
     * @return `ok`, `out_of_range`, or `invalid_argument` for whitespace/control characters.
     */
    castle::status assign(string_view value) CASTLE_NOEXCEPT
    {
        size_ = 0U;
        if (value.size() >= sizeof(text_))
        {
            return castle::status::out_of_range;
        }
        if (!is_visible_text(value))
        {
            return castle::status::invalid_argument;
        }
        for (size_type i = 0U; i < value.size(); ++i)
        {
            text_[size_++] = value[i];
        }
        text_[size_] = '\0';
        return castle::status::ok;
    }

    /** @brief Returns the stored authority. */
    string_view view() CASTLE_CONST CASTLE_NOEXCEPT { return string_view(text_, size_); }

private:
    /** @brief Appends one character; the caller guarantees capacity. */
    void append(char c) CASTLE_NOEXCEPT
    {
        text_[size_++] = c;
    }

    /** @brief Appends `value` in decimal; the caller guarantees capacity. */
    void append_decimal(uint32_t value) CASTLE_NOEXCEPT
    {
        char reverse[10U];
        size_type count = 0U;
        do
        {
            reverse[count++] = static_cast<char>('0' + value % 10U);
            value /= 10U;
        } while (value != 0U);
        while (count != 0U)
        {
            append(reverse[--count]);
        }
    }

    /** @brief Appends `value` as lowercase hexadecimal without padding; the caller guarantees capacity. */
    void append_hex(uint16_t value) CASTLE_NOEXCEPT
    {
        static char CASTLE_CONST digits[] = "0123456789abcdef";
        char reverse[4U];
        size_type count = 0U;
        do
        {
            reverse[count++] = digits[value & 0x0FU];
            value = static_cast<uint16_t>(value >> 4U);
        } while (value != 0U);
        while (count != 0U)
        {
            append(reverse[--count]);
        }
    }

    static CASTLE_INLINE CASTLE_CONSTEXPR size_type capacity = 64U;
    static_assert(capacity > 47U, "authority buffer must hold the longest numeric IPv6 authority");

    char text_[capacity]{};
    size_type size_ = 0U;
};
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
    using response_callback = castle::callbacks::function<void(castle::status, response_view CASTLE_CONST&), CallbackStorageSize>;
    using body_callback = typename parser_type::body_callback;

    /** @brief Binds the client to a transport that must outlive it. */
    explicit http1_client(Transport& transport) CASTLE_NOEXCEPT : transport_(transport) {}

    /**
     * @brief Installs the `void(castle::status, response_view const&)` completion callback.
     *
     * @return `ok`, or `invalid_callback` for an empty callable.
     */
    template <typename Callback>
    castle::status set_response_callback(Callback&& callback)
    {
        response_callback next(CASTLE_FORWARD<Callback>(callback));
        if (!next)
        {
            return castle::status::invalid_callback;
        }
        response_callback_ = CASTLE_MOVE(next);
        return castle::status::ok;
    }

    /**
     * @brief Installs a callback that receives every body chunk, including bytes beyond `MaxBodySize`.
     *
     * @return `ok`, or `invalid_callback` for an empty callable.
     */
    template <typename Callback>
    castle::status set_body_callback(Callback&& callback)
    {
        return parser_.set_body_callback(CASTLE_FORWARD<Callback>(callback));
    }

    /** @brief Removes the response callback. */
    void clear_response_callback() CASTLE_NOEXCEPT { response_callback_.reset(); }

    /** @brief Removes the body callback. */
    void clear_body_callback() CASTLE_NOEXCEPT { parser_.clear_body_callback(); }

    /** @brief Connects using the numeric `address:port` of `remote` as the Host header. */
    castle::status connect(net::endpoint CASTLE_CONST& remote, int timeout_ms = 0, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (!can_connect())
        {
            return castle::status::invalid_config;
        }
        authority_.assign(remote);
        return connect_transport(remote, timeout_ms, system_error);
    }

    /** @brief Connects using `authority` (for example `api.test:80`) as the Host header. */
    castle::status connect(net::endpoint CASTLE_CONST& remote, string_view authority,
                           int timeout_ms = 0, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (!can_connect())
        {
            return castle::status::invalid_config;
        }
        castle::status named = authority_.assign(authority);
        return castle::succeeded(named) ? connect_transport(remote, timeout_ms, system_error) : named;
    }

    /**
     * @brief Queues a request on the open connection; `poll()` sends it and receives the response.
     *
     * @param keep_alive Sent as the Connection header unless `headers` already has one.
     * @return `ok`, `not_configured` when not connected or busy, or the `http1_request::build()` failure.
     */
    castle::status request(method verb, string_view path, headers_type CASTLE_CONST& headers,
                           string_view body = string_view(), string_view custom_method = string_view(),
                           bool keep_alive = true) CASTLE_NOEXCEPT
    {
        if (!connected_ || state_ != state::ready)
        {
            return castle::status::not_configured;
        }
        castle::status result = request_.build(verb, path, authority_.view(), headers,
                                               body, custom_method, keep_alive);
        if (!castle::succeeded(result))
        {
            return result;
        }
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
                if (detail::contains_token(field.value, string_view("close")))
                {
                    request_connection_close = true;
                }
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
            if (!castle::succeeded(result))
            {
                return result;
            }
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
            if (result == castle::status::full || result == castle::status::empty)
            {
                return result;
            }
            if (!castle::succeeded(result))
            {
                return fail(result);
            }
            if (bytes_sent_ == request_.size())
            {
                state_ = state::receiving;
            }
            else
            {
                return castle::status::ok;
            }
        }
        if (state_ == state::receiving)
        {
            castle::status ready = transport_.socket().wait_readable(timeout_ms, system_error);
            if (!castle::succeeded(ready))
            {
                return ready;
            }
            size_type received = 0U;
            bool peer_closed = false;
            castle::status result = transport_.receive_some(receive_buffer_, ReceiveBufferSize,
                                                             received, system_error, &peer_closed);
            if (!castle::succeeded(result) && result != castle::status::empty)
            {
                return fail(result);
            }
            if (received != 0U || peer_closed)
            {
                size_type consumed = 0U;
                result = parser_.feed(receive_buffer_, received, peer_closed, &consumed);
                if (!castle::succeeded(result))
                {
                    return fail(result);
                }
                // Bytes after a complete response (or a closing peer) make the connection unsafe to reuse.
                if (parser_.complete())
                {
                    return complete_response(!peer_closed && consumed == received);
                }
            }
            return result;
        }
        return state_ == state::ready ? castle::status::ok : castle::status::not_configured;
    }

    /** @brief Closes the transport. */
    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        state_ = state::disconnected;
        connected_ = false;
        return transport_.close(system_error);
    }

    /** @brief Returns true while the connection is usable. */
    bool connected() CASTLE_CONST CASTLE_NOEXCEPT { return connected_; }

    /** @brief Returns true while a connect or request is in flight. */
    bool busy() CASTLE_CONST CASTLE_NOEXCEPT { return state_ == state::sending || state_ == state::receiving || state_ == state::connecting; }

    /** @brief Returns the cause of the last response failure. */
    error_code last_error() CASTLE_CONST CASTLE_NOEXCEPT { return parser_.last_error(); }

    /** @brief Returns the latest response; valid until the next request. */
    response_view response() CASTLE_CONST CASTLE_NOEXCEPT { return parser_.response(); }

    /** @brief Returns the borrowed transport. */
    Transport& transport() CASTLE_NOEXCEPT { return transport_; }

private:
    /** @brief Connection and request progress. */
    enum class state : uint8_t { disconnected, connecting, ready, sending, receiving, failed };

    /** @brief True when no connection is open or pending. */
    bool can_connect() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_ == state::disconnected || state_ == state::failed;
    }

    /** @brief Connects the transport and records the resulting state. */
    castle::status connect_transport(net::endpoint CASTLE_CONST& remote, int timeout_ms, int* system_error) CASTLE_NOEXCEPT
    {
        castle::status result = transport_.connect(remote, timeout_ms, true, system_error);
        connected_ = castle::succeeded(result);
        if (castle::succeeded(result))
        {
            state_ = state::ready;
        }
        else
        {
            state_ = result == castle::status::time_out ? state::connecting : state::disconnected;
        }
        return result;
    }

    /** @brief Closes the connection unless it can be reused, then reports the response. */
    castle::status complete_response(bool connection_reusable) CASTLE_NOEXCEPT
    {
        state_ = state::ready;
        response_view view = parser_.response();
        if (!connection_reusable || !view.keep_alive || !request_keep_alive_)
        {
            (void)transport_.close(nullptr);
            connected_ = false;
            state_ = state::disconnected;
        }
        if (response_callback_)
        {
            response_callback_(castle::status::ok, view);
        }
        return castle::status::ok;
    }

    /** @brief Closes the transport and reports `result` with the partial response. */
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
    detail::authority_buffer authority_{};
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

    /** @brief Binds the client to a transport that must outlive it. */
    explicit http2_client(Transport& transport) CASTLE_NOEXCEPT : transport_(transport) {}

    /** @brief Installs the response callback of the session. */
    template <typename Callback>
    castle::status set_response_callback(Callback&& callback) { return session_.set_response_callback(CASTLE_FORWARD<Callback>(callback)); }

    /** @brief Installs the body callback of the session. */
    template <typename Callback>
    castle::status set_body_callback(Callback&& callback) { return session_.set_body_callback(CASTLE_FORWARD<Callback>(callback)); }

    /** @brief Removes the response callback. */
    void clear_response_callback() CASTLE_NOEXCEPT { session_.clear_response_callback(); }

    /** @brief Removes the body callback. */
    void clear_body_callback() CASTLE_NOEXCEPT { session_.clear_body_callback(); }

    /** @brief Connects using the numeric `address:port` of `remote` as the default `:authority`. */
    castle::status connect(net::endpoint CASTLE_CONST& remote, int timeout_ms = 0,
                           int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        authority_.assign(remote);
        return connect_transport(remote, timeout_ms, system_error);
    }

    /** @brief Connects using `authority` (for example `h2.test:80`) as the default `:authority`. */
    castle::status connect(net::endpoint CASTLE_CONST& remote, string_view authority, int timeout_ms = 0,
                           int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::status named = authority_.assign(authority);
        return castle::succeeded(named) ? connect_transport(remote, timeout_ms, system_error) : named;
    }

private:
    /** @brief Connects the transport and starts the session once it is established. */
    castle::status connect_transport(net::endpoint CASTLE_CONST& remote, int timeout_ms,
                                     int* system_error) CASTLE_NOEXCEPT
    {
        castle::status result = transport_.connect(remote, timeout_ms, true, system_error);
        if (castle::succeeded(result))
        {
            connected_ = true;
            return session_.start();
        }
        if (result == castle::status::time_out)
        {
            connecting_ = true;
        }
        return result;
    }

public:
    /**
     * @brief Opens a stream using the default authority.
     *
     * @return See `http2_session::open_stream()`.
     */
    castle::status open_stream(method verb, string_view path, headers_type CASTLE_CONST& headers,
                               string_view body, uint32_t& stream_id,
                               string_view custom_method = string_view()) CASTLE_NOEXCEPT
    {
        return open_stream(verb, path, authority_.view(), headers, body, stream_id, custom_method);
    }

    /** @brief Opens a stream with an explicit `:authority`; an empty one selects the default. */
    castle::status open_stream(method verb, string_view path, string_view authority,
                               headers_type CASTLE_CONST& headers, string_view body,
                               uint32_t& stream_id, string_view custom_method = string_view()) CASTLE_NOEXCEPT
    {
        if (!connected_)
        {
            return castle::status::not_configured;
        }
        string_view effective_authority = authority.empty() ? authority_.view() : authority;
        return session_.open_stream(verb, path, effective_authority, headers, body, stream_id, custom_method);
    }

    /** @brief Pumps pending output and one readable receive event without busy waiting. */
    castle::status poll(int timeout_ms = 0, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (connecting_)
        {
            castle::status result = transport_.socket().finish_connect(timeout_ms, system_error);
            if (!castle::succeeded(result))
            {
                return result;
            }
            connecting_ = false;
            connected_ = true;
            // A fresh session always accepts the preface and SETTINGS.
            result = session_.start();
            if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
            {
                return result;
            }
        }
        if (!connected_)
        {
            return castle::status::not_configured;
        }
        castle::status sent = flush_output(system_error);
        bool CASTLE_CONST write_blocked = sent == castle::status::full || sent == castle::status::empty;
        if (!castle::succeeded(sent) && !write_blocked)
        {
            return sent;
        }
        // A blocked write must not prevent reads: the peer may need to send WINDOW_UPDATE,
        // SETTINGS or response DATA before the queued request bytes can make progress.
        castle::status ready = transport_.socket().wait_readable(timeout_ms, system_error);
        if (!castle::succeeded(ready))
        {
            return ready;
        }
        size_type received = 0U;
        bool peer_closed = false;
        castle::status result = transport_.receive_some(receive_buffer_, ReceiveBufferSize,
                                                         received, system_error, &peer_closed);
        if (!castle::succeeded(result) && result != castle::status::empty)
        {
            return result;
        }
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

    /** @brief Closes the transport. */
    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        connected_ = false;
        connecting_ = false;
        return transport_.close(system_error);
    }

    /** @brief Returns true while the connection is usable. */
    bool connected() CASTLE_CONST CASTLE_NOEXCEPT { return connected_; }

    /** @brief Returns the protocol session. */
    session_type& session() CASTLE_NOEXCEPT { return session_; }

    /** @brief Returns the protocol session. */
    session_type CASTLE_CONST& session() CASTLE_CONST CASTLE_NOEXCEPT { return session_; }

    /** @brief Returns the response of `stream_id`. */
    response_view response(uint32_t stream_id) CASTLE_CONST CASTLE_NOEXCEPT { return session_.response(stream_id); }

    /** @brief Returns the borrowed transport. */
    Transport& transport() CASTLE_NOEXCEPT { return transport_; }

private:
    /** @brief Sends queued session bytes until the transport blocks. */
    castle::status flush_output(int* system_error) CASTLE_NOEXCEPT
    {
        while (session_.output_size() != 0U)
        {
            uint8_t CASTLE_CONST* data = session_.pending_output();
            size_type size = session_.pending_output_size();
            size_type sent = 0U;
            castle::status result = transport_.send_some(data, size, sent, system_error);
            if (sent != 0U)
            {
                // send_some never reports more bytes than were offered, which consume_output accepts.
                castle::status consumed = session_.consume_output(sent);
                if (!castle::succeeded(consumed)) // LCOV_EXCL_BR_LINE
                {
                    return consumed;
                }
            }
            if (result == castle::status::full || result == castle::status::empty)
            {
                return result;
            }
            if (!castle::succeeded(result))
            {
                return result;
            }
            if (sent == 0U)
            {
                return castle::status::empty;
            }
        }
        return castle::status::ok;
    }

    Transport& transport_;
    session_type session_{};
    uint8_t receive_buffer_[ReceiveBufferSize]{};
    bool connected_ = false;
    bool connecting_ = false;
    detail::authority_buffer authority_{};
};

/** @brief Selects the HTTP/1.1 client. */
struct http1_policy {};

/** @brief Selects the HTTP/2 client. */
struct http2_policy {};

/** @brief Compile-time description of a protocol driver; selects the client type without virtual dispatch. */
template <typename Policy>
struct protocol_policy_traits;

template <>
struct protocol_policy_traits<http1_policy>
{
    static CASTLE_INLINE CASTLE_CONSTEXPR version protocol = version::http_1_1;
    static CASTLE_INLINE CASTLE_CONSTEXPR bool multiplexed = false;
    template <typename Transport>
    using client = http1_client<Transport>;
};

template <>
struct protocol_policy_traits<http2_policy>
{
    static CASTLE_INLINE CASTLE_CONSTEXPR version protocol = version::http_2;
    static CASTLE_INLINE CASTLE_CONSTEXPR bool multiplexed = true;
    template <typename Transport>
    using client = http2_client<Transport>;
};

template <typename Transport, typename Policy = http1_policy>
using basic_http_client = typename protocol_policy_traits<Policy>::template client<Transport>;

} // namespace http
} // namespace protocols
} // namespace castle

#endif // CASTLE_USING_BSD_SOCKET_BASED_HTTP_PROTOCOL

#endif // CASTLE_EXT_PROTOCOLS_HTTP_CLIENT_HPP
