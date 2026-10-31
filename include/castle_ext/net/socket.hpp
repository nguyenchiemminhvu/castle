// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file socket.hpp
 * @brief Move-only, typed BSD TCP/UDP socket wrapper built on CASTLE's FD layer.
 *
 * `basic_socket<socket_protocol::tcp>` and `basic_socket<socket_protocol::udp>` own one native
 * socket through `castle::io::fd::basic_fd`. The wrapper validates state and arguments, maps
 * would-block conditions to `empty`/`full`, and leaves the BSD calls to `detail/native.hpp`.
 * It performs no dynamic allocation and does not resolve host names.
 */
#ifndef CASTLE_EXT_NET_SOCKET_HPP
#define CASTLE_EXT_NET_SOCKET_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/move.hpp"
#include "castle_ext/net/detail/native.hpp"
#include "castle_ext/net/endpoint.hpp"
#include "castle_ext/io/fd/descriptor.hpp"
#include "castle_ext/io/fd/poll.hpp"

#include <stdint.h>

namespace castle
{
namespace net
{

/** @brief Compile-time transport selected by a socket, client or server type. */
enum class socket_protocol : uint8_t
{
    tcp = 0U,
    udp = 1U
};

/** @brief Direction selected by `basic_socket::shutdown()`. */
enum class shutdown_mode : uint8_t
{
    receive = 0U,
    send = 1U,
    both = 2U
};

/**
 * @brief Owning BSD socket whose TCP/UDP protocol is fixed at compile time.
 * @tparam Protocol `socket_protocol::tcp` for a stream socket or `socket_protocol::udp`
 *         for a datagram socket.
 *
 * A socket is move-only. Stream send/receive operations may transfer fewer bytes than requested;
 * `send_all()` continues only while progress is possible and never busy-waits on a non-blocking
 * descriptor. A would-block receive is `empty`, a would-block send is `full`, and native errors
 * are `system_call_error` with `errno` in the optional `system_error` output.
 */
template <socket_protocol Protocol>
class basic_socket
{
public:
    static CASTLE_INLINE CASTLE_CONSTEXPR socket_protocol protocol = Protocol;

    /** @brief Constructs a closed socket. */
    basic_socket() CASTLE_DEFAULT;

    /** @brief Closes the socket through the owned `basic_fd`. */
    ~basic_socket() CASTLE_DEFAULT;

    basic_socket(CASTLE_CONST basic_socket&) CASTLE_DELETE;
    basic_socket& operator=(CASTLE_CONST basic_socket&) CASTLE_DELETE;

    /** @brief Transfers descriptor ownership and socket state. */
    basic_socket(basic_socket&& other) CASTLE_NOEXCEPT
        : descriptor_(CASTLE_MOVE(other.descriptor_))
        , family_(other.family_)
        , nonblocking_(other.nonblocking_)
    {
        other.nonblocking_ = false;
    }

    /** @brief Closes the current socket and takes ownership from `other`. */
    basic_socket& operator=(basic_socket&& other) CASTLE_NOEXCEPT
    {
        if (this != &other) // LCOV_EXCL_BR_LINE
        {
            descriptor_ = CASTLE_MOVE(other.descriptor_);
            family_ = other.family_;
            nonblocking_ = other.nonblocking_;
            other.nonblocking_ = false;
        }
        return *this;
    }

    /**
     * @brief Creates an IPv4 or IPv6 socket with close-on-exec set.
     * @param family Address family used by later bind/connect operations.
     * @param nonblocking True to enable `O_NONBLOCK` immediately (the default).
     * @return `ok`, `invalid_config` if already open, `invalid_argument`, `system_call_error`,
     *         or `not_configured`.
     */
    castle::status open(address_family family = address_family::ipv4,
                        bool nonblocking = true,
                        int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (is_open())
        {
            return castle::status::invalid_config;
        }
        if (family != address_family::ipv4 && family != address_family::ipv6)
        {
            return castle::status::invalid_argument;
        }
        CASTLE_CONST castle::status result = detail::open_socket(family, Protocol == socket_protocol::tcp,
                                                                  nonblocking, descriptor_, system_error);
        if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            return result;
        }
        family_ = family;
        nonblocking_ = nonblocking;
        return castle::status::ok;
    }

    /** @brief Closes the socket; returns `not_configured` when it is already closed. */
    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        nonblocking_ = false;
        return descriptor_.close(system_error);
    }

    /** @brief Returns true while the native socket is owned. */
    CASTLE_NODISCARD bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return descriptor_.is_open(); }
    /** @brief Returns the owned native socket descriptor, or -1 when closed. */
    CASTLE_NODISCARD int native_handle() CASTLE_CONST CASTLE_NOEXCEPT { return descriptor_.native_handle(); }
    /** @brief Returns the address family selected by `open()`. */
    CASTLE_NODISCARD address_family family() CASTLE_CONST CASTLE_NOEXCEPT { return family_; }
    /** @brief Returns whether this socket currently has `O_NONBLOCK` enabled. */
    CASTLE_NODISCARD bool is_nonblocking() CASTLE_CONST CASTLE_NOEXCEPT { return nonblocking_; }

    /** @brief Enables or disables non-blocking mode while preserving other descriptor flags. */
    castle::status set_nonblocking(bool enabled = true, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        CASTLE_CONST castle::status result = descriptor_.set_nonblocking(enabled, system_error);
        if (castle::succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            nonblocking_ = enabled;
        }
        return result;
    }

    /** @brief Enables/disables `SO_REUSEADDR`, normally before binding a TCP listener. */
    castle::status set_reuse_address(bool enabled = true, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        return detail::set_reuse_address(native_handle(), enabled, system_error);
    }

    /** @brief Enables/disables `SO_KEEPALIVE` for TCP connections. */
    castle::status set_keep_alive(bool enabled = true, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (Protocol != socket_protocol::tcp)
        {
            return castle::status::invalid_config;
        }
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        return detail::set_keep_alive(native_handle(), enabled, system_error);
    }

    /** @brief Enables/disables TCP_NODELAY for a TCP stream socket. */
    castle::status set_no_delay(bool enabled = true, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (Protocol != socket_protocol::tcp)
        {
            return castle::status::invalid_config;
        }
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        return detail::set_no_delay(native_handle(), enabled, system_error);
    }

    /** @brief Binds the socket to a local numeric IPv4 or IPv6 endpoint. */
    castle::status bind(endpoint CASTLE_CONST& local, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        if (local.family() != family_)
        {
            return castle::status::invalid_argument;
        }
        return detail::bind_socket(native_handle(), local, system_error);
    }

    /**
     * @brief Connects to a numeric peer without waiting for a non-blocking connect to finish.
     * @return `ok` when connected, `time_out` with `system_error` set to `EINPROGRESS`/`EALREADY`
     *         while establishment is pending, or `system_call_error` on failure.
     */
    castle::status connect(endpoint CASTLE_CONST& remote, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        if (remote.family() != family_)
        {
            return castle::status::invalid_argument;
        }
        return detail::connect_socket(native_handle(), remote, system_error);
    }

    /**
     * @brief Completes a pending TCP connect, optionally waiting for writability first.
     * @param timeout_ms Milliseconds to wait; zero only checks readiness, negative waits forever.
     * @return `ok` once connected, `time_out` while still pending, `invalid_config` for UDP, or
     *         `system_call_error` (with the connect error in `system_error`).
     */
    castle::status finish_connect(int timeout_ms = 0, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (Protocol != socket_protocol::tcp)
        {
            return castle::status::invalid_config;
        }
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        return detail::finish_connect_socket(native_handle(), timeout_ms, system_error);
    }

    /** @brief Starts listening for incoming TCP connections. */
    castle::status listen(int backlog = 8, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        if (Protocol != socket_protocol::tcp)
        {
            return castle::status::invalid_config;
        }
        if (backlog < 0) // LCOV_EXCL_BR_LINE
        {
            return castle::status::invalid_argument;
        }
        return detail::listen_socket(native_handle(), backlog, system_error);
    }

    /**
     * @brief Accepts one TCP peer into an empty output socket that inherits this socket's blocking mode.
     * @return `ok`, `empty` when a non-blocking listener has no pending peer, `invalid_config` for a UDP
     *         listener or an already-open `out`, `not_configured`, or `system_call_error`.
     */
    castle::status accept(basic_socket<socket_protocol::tcp>& out, endpoint& remote,
                          int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (Protocol != socket_protocol::tcp)
        {
            return castle::status::invalid_config;
        }
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        if (out.is_open())
        {
            return castle::status::invalid_config;
        }
        CASTLE_CONST castle::status result = detail::accept_socket(native_handle(), nonblocking_,
                                                                    out.descriptor_, remote, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
        out.family_ = family_;
        out.nonblocking_ = nonblocking_;
        return castle::status::ok;
    }

    /**
     * @brief Sends one chunk on a connected socket.
     * @return `ok`; `full` when a stream cannot accept data now; `data_loss` when a datagram was cut short.
     */
    castle::status send_some(CASTLE_CONST void* buffer, size_type size, size_type& bytes_sent,
                             int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        bytes_sent = 0U;
        detail::clear_system_error(system_error);
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        if (buffer == nullptr && size != 0U)
        {
            return castle::status::invalid_argument;
        }
        if (Protocol == socket_protocol::tcp && size == 0U) // LCOV_EXCL_BR_LINE
        {
            return castle::status::ok;
        }
        CASTLE_CONST castle::status result = detail::send_bytes(native_handle(), buffer, size, nullptr,
                                                                bytes_sent, system_error);
        return completed_send(result, size, bytes_sent);
    }

    /** @brief Writes until all bytes are accepted or the first non-success result occurs (TCP only). */
    castle::status send_all(CASTLE_CONST void* buffer, size_type size, size_type& bytes_sent,
                            int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        bytes_sent = 0U;
        detail::clear_system_error(system_error);
        if (Protocol != socket_protocol::tcp)
        {
            return castle::status::invalid_config;
        }
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        if (buffer == nullptr && size != 0U)
        {
            return castle::status::invalid_argument;
        }
        while (bytes_sent < size)
        {
            size_type count = 0U;
            CASTLE_CONST castle::status result = send_some(static_cast<CASTLE_CONST uint8_t*>(buffer) + bytes_sent,
                                                           size - bytes_sent, count, system_error);
            bytes_sent += count;
            if (!castle::succeeded(result))
            {
                return result;
            }
        }
        return castle::status::ok;
    }

    /**
     * @brief Receives from a connected socket.
     * @param peer_closed Optional; set for TCP EOF, which is `empty` with zero bytes, to tell it from would-block.
     * @return `ok`, `empty`, or for UDP `data_loss` when the datagram exceeded `capacity` (the rest is dropped).
     */
    castle::status receive_some(void* buffer, size_type capacity, size_type& bytes_received,
                                int* system_error = nullptr, bool* peer_closed = nullptr) CASTLE_NOEXCEPT
    {
        bytes_received = 0U;
        if (peer_closed != nullptr)
        {
            *peer_closed = false;
        }
        detail::clear_system_error(system_error);
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        if (buffer == nullptr && capacity != 0U)
        {
            return castle::status::invalid_argument;
        }
        if (Protocol == socket_protocol::tcp && capacity == 0U)
        {
            return castle::status::ok;
        }

        bool truncated = false;
        CASTLE_CONST castle::status result = detail::receive_bytes(native_handle(), buffer, capacity, nullptr,
                                                                    Protocol == socket_protocol::udp,
                                                                    bytes_received, truncated, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
        if (Protocol == socket_protocol::tcp)
        {
            if (bytes_received != 0U)
            {
                return castle::status::ok;
            }
            if (peer_closed != nullptr) // LCOV_EXCL_BR_LINE
            {
                *peer_closed = true;
            }
            return castle::status::empty;
        }
        return truncated ? castle::status::data_loss : castle::status::ok;
    }

    /** @brief Sends exactly one UDP datagram to `remote`; available only on UDP sockets. */
    castle::status send_to(CASTLE_CONST void* buffer, size_type size, endpoint CASTLE_CONST& remote,
                           size_type& bytes_sent, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        bytes_sent = 0U;
        detail::clear_system_error(system_error);
        if (Protocol != socket_protocol::udp)
        {
            return castle::status::invalid_config;
        }
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        if ((buffer == nullptr && size != 0U) || remote.family() != family_)
        {
            return castle::status::invalid_argument;
        }
        CASTLE_CONST castle::status result = detail::send_bytes(native_handle(), buffer, size, &remote,
                                                                bytes_sent, system_error);
        return completed_send(result, size, bytes_sent);
    }

    /**
     * @brief Receives one UDP datagram and reports its source endpoint.
     * @return `ok`, `empty` when none is queued, or `data_loss` when it exceeded `capacity` (the rest is dropped).
     */
    castle::status receive_from(void* buffer, size_type capacity, size_type& bytes_received,
                                endpoint& remote, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        bytes_received = 0U;
        detail::clear_system_error(system_error);
        if (Protocol != socket_protocol::udp)
        {
            return castle::status::invalid_config;
        }
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        if (buffer == nullptr && capacity != 0U) // LCOV_EXCL_BR_LINE
        {
            return castle::status::invalid_argument;
        }

        bool truncated = false;
        CASTLE_CONST castle::status result = detail::receive_bytes(native_handle(), buffer, capacity, &remote,
                                                                    true, bytes_received, truncated, system_error);
        if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            return result;
        }
        return truncated ? castle::status::data_loss : castle::status::ok;
    }

    /** @brief Waits for requested native poll events using CASTLE's `fd::wait_fd()`. */
    castle::status wait(short events, int timeout_ms, short& revents,
                        int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        revents = 0;
        detail::clear_system_error(system_error);
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        return detail::wait_socket(native_handle(), events, timeout_ms, revents, system_error);
    }

    /** @brief Waits until the socket is readable; `ok` when ready, `time_out` when the wait expired. */
    castle::status wait_readable(int timeout_ms, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        return detail::wait_socket_readable(native_handle(), timeout_ms, system_error);
    }

    /** @brief Returns the local endpoint populated by `bind()` or assigned by the OS. */
    castle::status local_endpoint(endpoint& out, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return query_endpoint(false, out, system_error);
    }

    /** @brief Returns the connected peer endpoint. */
    castle::status peer_endpoint(endpoint& out, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return query_endpoint(true, out, system_error);
    }

    /** @brief Shuts down the receive side, send side, or both directions. */
    castle::status shutdown(shutdown_mode mode = shutdown_mode::both,
                            int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (!is_open()) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_configured;
        }
        if (mode != shutdown_mode::receive && mode != shutdown_mode::send && mode != shutdown_mode::both)
        {
            return castle::status::invalid_argument;
        }
        return detail::shutdown_socket(native_handle(), mode != shutdown_mode::send,
                                       mode != shutdown_mode::receive, system_error);
    }

private:
    template <socket_protocol> friend class basic_socket;

    /** @brief Applies the protocol's rule for a short write: stream `full`, datagram `data_loss`. */
    static castle::status completed_send(castle::status result, size_type size, size_type sent) CASTLE_NOEXCEPT
    {
        if (!castle::succeeded(result))
        {
            return result;
        }
        if (Protocol == socket_protocol::tcp)
        {
            return sent == 0U ? castle::status::full : castle::status::ok; // LCOV_EXCL_BR_LINE
        }
        return sent == size ? castle::status::ok : castle::status::data_loss; // LCOV_EXCL_BR_LINE
    }

    castle::status query_endpoint(bool peer, endpoint& out, int* system_error) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (!is_open())
        {
            return castle::status::not_configured;
        }
        return detail::query_endpoint(native_handle(), peer, out, system_error);
    }

    castle::io::fd::basic_fd descriptor_;
    address_family family_ = address_family::ipv4;
    bool nonblocking_ = false;
};

/** @brief Compile-time typed TCP socket. */
using tcp_socket = basic_socket<socket_protocol::tcp>;
/** @brief Compile-time typed UDP socket. */
using udp_socket = basic_socket<socket_protocol::udp>;

} // namespace net
} // namespace castle

#endif // CASTLE_EXT_NET_SOCKET_HPP
