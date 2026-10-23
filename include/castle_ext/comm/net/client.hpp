// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file client.hpp
 * @brief Allocation-free TCP and UDP clients with inline receive callbacks.
 *
 * A client owns one typed BSD socket and a compile-time-sized receive buffer.
 * `process_once()` waits through CASTLE's FD poll helper, receives at most one
 * bufferful (one UDP datagram for UDP), and invokes an optional inline callback.
 * Applications may also use the direct send/receive methods for custom event loops.
 */
#ifndef CASTLE_EXT_COMM_NET_CLIENT_HPP
#define CASTLE_EXT_COMM_NET_CLIENT_HPP

#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle_ext/comm/net/detail/callback_dispatcher.hpp"
#include "castle_ext/comm/net/socket.hpp"

#include <stdint.h>

namespace castle
{
namespace comm
{
namespace net
{

/**
 * @brief TCP or UDP client built around a fixed receive buffer and one inline callback.
 * @tparam Protocol Compile-time transport (`tcp` or `udp`).
 * @tparam ReceiveBufferSize Maximum bytes accepted by one `process_once()` call.
 * @tparam CallbackStorageSize Inline bytes reserved for the receive callback.
 *
 * Receive callbacks have the signature `void(endpoint const&, uint8_t const*, size_type)`.
 * The endpoint is the peer given to `connect()` for TCP or the source of a received UDP datagram.
 * The data pointer is valid only for the duration of the callback.
 *
 * The callback runs in place, so a mutable callable keeps its state between calls. It may call
 * `set_receive_callback()`, `clear_receive_callback()` or `close()`; a replacement or clear takes
 * effect when the callback returns. It must not move or destroy the client, or call `process_once()`
 * (rejected with `invalid_config`).
 */
template <socket_protocol Protocol,
          size_type ReceiveBufferSize = 256U,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
class basic_client
{
    static_assert(ReceiveBufferSize > 0U, "Network client receive buffer must not be empty");
    static_assert(CallbackStorageSize > 0U, "Network client callback storage must not be empty");

public:
    using socket_type = basic_socket<Protocol>;
    using callback_type = castle::callbacks::function<void(endpoint CASTLE_CONST&, uint8_t CASTLE_CONST*, size_type),
                                                       CallbackStorageSize>;

    /** @brief Constructs a closed client with an empty receive callback. */
    basic_client() CASTLE_DEFAULT;

    basic_client(CASTLE_CONST basic_client&) CASTLE_DELETE;
    basic_client& operator=(CASTLE_CONST basic_client&) CASTLE_DELETE;

    /** @brief Transfers socket ownership, peer and callback state. */
    basic_client(basic_client&& other) CASTLE_NOEXCEPT
        : socket_(castle::move(other.socket_))
        , receive_callback_(castle::move(other.receive_callback_))
        , peer_(other.peer_)
    {
        other.peer_ = endpoint();
    }

    /** @brief Closes the current socket and takes the state from `other`. */
    basic_client& operator=(basic_client&& other) CASTLE_NOEXCEPT
    {
        if (this != &other) // LCOV_EXCL_BR_LINE
        {
            socket_ = castle::move(other.socket_);
            receive_callback_ = castle::move(other.receive_callback_);
            peer_ = other.peer_;
            other.peer_ = endpoint();
        }
        return *this;
    }

    /** @brief Opens the underlying socket. Non-blocking mode is enabled by default. */
    castle::status open(address_family family = address_family::ipv4,
                        bool nonblocking = true,
                        int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return socket_.open(family, nonblocking, system_error);
    }

    /** @brief Binds the local endpoint, opening the socket with its address family when needed. */
    castle::status bind(endpoint CASTLE_CONST& local, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (!socket_.is_open()) // LCOV_EXCL_BR_LINE
        {
            CASTLE_CONST castle::status opened = open(local.family(), true, system_error);
            if (!castle::succeeded(opened)) // LCOV_EXCL_BR_LINE
            {
                return opened;
            }
        }
        return socket_.bind(local, system_error);
    }

    /**
     * @brief Connects to a numeric peer; a pending non-blocking connect is completed via poll().
     * @param remote Numeric IPv4/IPv6 peer endpoint.
     * @param timeout_ms Connection wait in milliseconds; zero does not wait, negative waits forever.
     * @param nonblocking Selects the socket mode when this call opens the client.
     * @return `ok`; `time_out` while the connection is still pending, in which case the socket stays open
     *         for `socket().finish_connect()` or `close()`; otherwise the failure, after which a socket
     *         opened by this call is closed again so a retry may use any address family.
     */
    castle::status connect(endpoint CASTLE_CONST& remote,
                           int timeout_ms = 0,
                           bool nonblocking = true,
                           int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        CASTLE_CONST bool opened_here = !socket_.is_open();
        if (opened_here) // LCOV_EXCL_BR_LINE
        {
            CASTLE_CONST castle::status opened = open(remote.family(), nonblocking, system_error);
            if (!castle::succeeded(opened)) // LCOV_EXCL_BR_LINE
            {
                return opened;
            }
        }
        castle::status result = socket_.connect(remote, system_error);
        if (result == castle::status::time_out) // LCOV_EXCL_BR_LINE
        {
            result = socket_.finish_connect(timeout_ms, system_error);
        }
        if (castle::succeeded(result) || result == castle::status::time_out)
        {
            peer_ = remote;
        }
        else if (opened_here) // LCOV_EXCL_BR_LINE
        {
            (void)socket_.close(nullptr);
        }
        return result;
    }

    /** @brief Closes the client socket. */
    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        peer_ = endpoint();
        return socket_.close(system_error);
    }

    /** @brief Returns true while the underlying socket is open. */
    CASTLE_NODISCARD bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return socket_.is_open(); }
    /** @brief Returns the native descriptor, or -1 when closed. */
    CASTLE_NODISCARD int native_handle() CASTLE_CONST CASTLE_NOEXCEPT { return socket_.native_handle(); }
    /** @brief Provides access to the underlying typed socket for advanced socket options. */
    CASTLE_NODISCARD socket_type& socket() CASTLE_NOEXCEPT { return socket_; }
    /** @brief Provides const access to the underlying typed socket. */
    CASTLE_NODISCARD socket_type CASTLE_CONST& socket() CASTLE_CONST CASTLE_NOEXCEPT { return socket_; }

    /** @brief Enables or disables non-blocking I/O on the underlying socket. */
    castle::status set_nonblocking(bool enabled = true, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return socket_.set_nonblocking(enabled, system_error);
    }

    /** @brief Sends bytes on a connected TCP/UDP client socket. */
    castle::status send_some(CASTLE_CONST void* data, size_type size, size_type& bytes_sent,
                             int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return socket_.send_some(data, size, bytes_sent, system_error);
    }

    /** @brief Sends all bytes on a TCP connection or returns on the first blocked/error result. */
    castle::status send_all(CASTLE_CONST void* data, size_type size, size_type& bytes_sent,
                            int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return socket_.send_all(data, size, bytes_sent, system_error);
    }

    /** @brief Sends one UDP datagram to an explicit peer endpoint. */
    castle::status send_to(CASTLE_CONST void* data, size_type size, endpoint CASTLE_CONST& remote,
                           size_type& bytes_sent, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return socket_.send_to(data, size, remote, bytes_sent, system_error);
    }

    /** @brief Receives from a connected socket; optional `peer_closed` reports TCP EOF. */
    castle::status receive_some(void* data, size_type capacity, size_type& bytes_received,
                                int* system_error = nullptr, bool* peer_closed = nullptr) CASTLE_NOEXCEPT
    {
        return socket_.receive_some(data, capacity, bytes_received, system_error, peer_closed);
    }

    /** @brief Receives one UDP datagram into caller-owned storage and reports its source endpoint. */
    castle::status receive_from(void* data, size_type capacity, size_type& bytes_received,
                                endpoint& remote, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return socket_.receive_from(data, capacity, bytes_received, remote, system_error);
    }

    /**
     * @brief Replaces the receive callback; the callable is stored inline and must be copyable.
     * @return `ok`, or `invalid_callback` for a null function pointer. A callable too large for
     *         `CallbackStorageSize` fails to compile.
     */
    template <typename Callback>
    castle::status set_receive_callback(Callback&& callback)
    {
        callback_type next(CASTLE_FORWARD<Callback>(callback));
        if (!next)
        {
            return castle::status::invalid_callback;
        }
        dispatcher_.replace(receive_callback_, castle::move(next));
        return castle::status::ok;
    }

    /** @brief Clears the receive callback without closing the socket. */
    void clear_receive_callback() CASTLE_NOEXCEPT { dispatcher_.clear(receive_callback_); }

    /**
     * @brief Waits for and processes one receive event using the internal fixed buffer.
     * @param timeout_ms Wait time in milliseconds; zero polls without blocking, negative waits forever.
     * @return `ok` after a packet/stream chunk is received, `time_out` for no readiness,
     *         `empty` for a non-blocking receive with no data, `not_configured` when closed or after
     *         TCP peer EOF (which closes the socket), `data_loss` for a UDP datagram larger than the
     *         buffer (dropped without a callback), `invalid_config` when called from the callback, or
     *         the underlying socket error status.
     */
    castle::status process_once(int timeout_ms = 0, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (dispatcher_.is_dispatching())
        {
            return castle::status::invalid_config;
        }
        if (!socket_.is_open())
        {
            return castle::status::not_configured;
        }
        castle::status result = socket_.wait_readable(timeout_ms, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }

        size_type received = 0U;
        endpoint source = peer_;
        bool peer_closed = false;
        if (Protocol == socket_protocol::udp)
        {
            result = socket_.receive_from(buffer_, ReceiveBufferSize, received, source, system_error);
        }
        else
        {
            result = socket_.receive_some(buffer_, ReceiveBufferSize, received, system_error, &peer_closed);
        }
        if (peer_closed)
        {
            (void)close(nullptr);
            return castle::status::not_configured;
        }
        if (!castle::succeeded(result))
        {
            return result;
        }
        if (receive_callback_)
        {
            dispatcher_.invoke(receive_callback_, source, buffer_, received);
        }
        return castle::status::ok;
    }

private:
    socket_type socket_;
    callback_type receive_callback_;
    detail::callback_dispatcher<callback_type> dispatcher_;
    endpoint peer_;
    uint8_t buffer_[ReceiveBufferSize]{};
};

/** @brief Fixed-buffer TCP client alias. */
template <size_type ReceiveBufferSize = 256U,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
using tcp_client = basic_client<socket_protocol::tcp, ReceiveBufferSize, CallbackStorageSize>;

/** @brief Fixed-buffer UDP client alias. */
template <size_type ReceiveBufferSize = 256U,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
using udp_client = basic_client<socket_protocol::udp, ReceiveBufferSize, CallbackStorageSize>;

} // namespace net
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_NET_CLIENT_HPP
