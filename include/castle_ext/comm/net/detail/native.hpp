// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file native.hpp
 * @brief BSD socket calls translated into `castle::status`; the only code that touches the host socket ABI.
 *
 * Callers clear `system_error` first; a failing call reports its `errno` through it. Linux and Zephyr share one
 * body written against the POSIX socket names (Zephyr needs `CONFIG_POSIX_API`); only headers and a few optional
 * macros differ. Without `CASTLE_USING_LINUX_BSD_SOCKET` or `CASTLE_USING_ZEPHYR_BSD_SOCKET` every function still
 * exists and returns `not_configured`, so the socket, client and server classes need no preprocessor branches.
 */
#ifndef CASTLE_EXT_COMM_NET_DETAIL_NATIVE_HPP
#define CASTLE_EXT_COMM_NET_DETAIL_NATIVE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/move.hpp"
#include "castle_ext/comm/net/endpoint.hpp"
#include "castle_ext/io/fd/control.hpp"
#include "castle_ext/io/fd/descriptor.hpp"
#include "castle_ext/io/fd/poll.hpp"

#if CASTLE_USING_LINUX_BSD_SOCKET
    #include <errno.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <string.h>
    #include <sys/socket.h>
#elif CASTLE_USING_ZEPHYR_BSD_SOCKET
    #include <errno.h>
    #include <string.h>
    #include <zephyr/net/socket.h>
#else
    // Unsupported platform
#endif

namespace castle
{
namespace comm
{
namespace net
{
namespace detail
{

using castle::io::fd::detail::clear_system_error;

/** @brief Result of every operation on a target without BSD sockets. */
CASTLE_INLINE castle::status unsupported(int* system_error) CASTLE_NOEXCEPT
{
    clear_system_error(system_error);
    return castle::status::not_configured;
}

#if CASTLE_USING_LINUX_BSD_SOCKET || CASTLE_USING_ZEPHYR_BSD_SOCKET

using castle::io::fd::detail::capture_system_error;
using castle::io::fd::detail::would_block;

/** @brief Largest byte count one send or receive accepts: the positive `ssize_t` range. */
static CASTLE_CONSTEXPR size_type max_transfer_size = static_cast<size_type>(-1) >> 1U;

/** @brief `recv` flag that reports a datagram's real length; zero where the target lacks it, so truncation goes unseen. */
#if defined(MSG_TRUNC)
static CASTLE_CONSTEXPR int datagram_length_flag = MSG_TRUNC;
#elif defined(ZSOCK_MSG_TRUNC)
static CASTLE_CONSTEXPR int datagram_length_flag = ZSOCK_MSG_TRUNC;
#else
static CASTLE_CONSTEXPR int datagram_length_flag = 0;
#endif

/** @brief Records `errno` and reports a failed system call. */
CASTLE_INLINE castle::status system_failure(int* system_error) CASTLE_NOEXCEPT
{
    capture_system_error(system_error);
    return castle::status::system_call_error;
}

CASTLE_INLINE castle::status to_native_endpoint(endpoint CASTLE_CONST& value,
                                                 struct sockaddr_storage& storage,
                                                 socklen_t& length) CASTLE_NOEXCEPT
{
    storage = sockaddr_storage();
    if (value.family() == address_family::ipv4)
    {
        struct sockaddr_in native_address = {};
        native_address.sin_family = AF_INET;
        native_address.sin_port = htons(value.port());
        ::memcpy(&native_address.sin_addr, value.address_bytes(), 4U);
        ::memcpy(&storage, &native_address, sizeof(native_address));
        length = static_cast<socklen_t>(sizeof(native_address));
        return castle::status::ok;
    }
    if (value.family() == address_family::ipv6) // LCOV_EXCL_BR_LINE
    {
        struct sockaddr_in6 native_address = {};
        native_address.sin6_family = AF_INET6;
        native_address.sin6_port = htons(value.port());
        // Zephyr stores the scope as an 8-bit interface index.
        native_address.sin6_scope_id = static_cast<decltype(native_address.sin6_scope_id)>(value.scope_id());
        ::memcpy(&native_address.sin6_addr, value.address_bytes(), 16U);
        ::memcpy(&storage, &native_address, sizeof(native_address));
        length = static_cast<socklen_t>(sizeof(native_address));
        return castle::status::ok;
    }
    length = 0;
    return castle::status::invalid_argument;
}

CASTLE_INLINE castle::status from_native_endpoint(struct sockaddr_storage CASTLE_CONST& storage,
                                                   socklen_t length,
                                                   endpoint& value) CASTLE_NOEXCEPT
{
    if (storage.ss_family == AF_INET && length >= static_cast<socklen_t>(sizeof(struct sockaddr_in)))
    {
        struct sockaddr_in native_address;
        ::memcpy(&native_address, &storage, sizeof(native_address));
        CASTLE_CONST uint8_t* bytes = reinterpret_cast<CASTLE_CONST uint8_t*>(&native_address.sin_addr);
        value = endpoint::ipv4(bytes[0], bytes[1], bytes[2], bytes[3], ntohs(native_address.sin_port));
        return castle::status::ok;
    }
    if (storage.ss_family == AF_INET6 && length >= static_cast<socklen_t>(sizeof(struct sockaddr_in6)))
    {
        struct sockaddr_in6 native_address;
        ::memcpy(&native_address, &storage, sizeof(native_address));
        value = endpoint::ipv6(reinterpret_cast<CASTLE_CONST uint8_t*>(&native_address.sin6_addr),
                               ntohs(native_address.sin6_port), native_address.sin6_scope_id);
        return castle::status::ok;
    }
    return castle::status::invalid_argument;
}

/**
 * @brief Completes a new descriptor: close-on-exec, non-blocking mode and SIGPIPE suppression.
 * @param flags_applied True when `socket()`/`accept4()` already set close-on-exec and non-blocking atomically.
 */
CASTLE_INLINE castle::status finish_new_socket(castle::io::fd::basic_fd& descriptor,
                                               bool nonblocking,
                                               bool flags_applied,
                                               int* system_error) CASTLE_NOEXCEPT
{
    // LCOV_EXCL_START
    if (!flags_applied)
    {
        castle::status result = descriptor.set_close_on_exec(true, system_error);
        // Targets without descriptor flags report not_configured and have no exec() to leak into.
        if (!castle::succeeded(result) && result != castle::status::not_configured)
        {
            return result;
        }
        if (nonblocking)
        {
            result = descriptor.set_nonblocking(true, system_error);
            if (!castle::succeeded(result))
            {
                return result;
            }
        }
    }
#if !defined(MSG_NOSIGNAL) && defined(SO_NOSIGPIPE)
    CASTLE_CONST int enabled = 1;
    if (::setsockopt(descriptor.native_handle(), SOL_SOCKET, SO_NOSIGPIPE, &enabled,
                     static_cast<socklen_t>(sizeof(enabled))) != 0)
    {
        return system_failure(system_error);
    }
#endif
    return castle::status::ok;
    // LCOV_EXCL_STOP
}

/** @brief Creates a TCP (`stream`) or UDP socket and moves it into `out` on success. */
CASTLE_INLINE castle::status open_socket(address_family family,
                                         bool stream,
                                         bool nonblocking,
                                         castle::io::fd::basic_fd& out,
                                         int* system_error) CASTLE_NOEXCEPT
{
    int type = stream ? SOCK_STREAM : SOCK_DGRAM;
    bool flags_applied = false;
#if defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)
    type |= SOCK_CLOEXEC | (nonblocking ? SOCK_NONBLOCK : 0);
    flags_applied = true;
#endif
    CASTLE_CONST int native = ::socket(family == address_family::ipv4 ? AF_INET : AF_INET6, type,
                                       stream ? IPPROTO_TCP : IPPROTO_UDP);
    // LCOV_EXCL_START
    if (native < 0)
    {
        return system_failure(system_error);
    }
    // LCOV_EXCL_STOP

    castle::io::fd::basic_fd descriptor(native);
    CASTLE_CONST castle::status prepared = finish_new_socket(descriptor, nonblocking, flags_applied, system_error);

    // LCOV_EXCL_START
    if (!castle::succeeded(prepared))
    {
        return prepared;
    }
    // LCOV_EXCL_STOP

    out = CASTLE_MOVE(descriptor);
    return castle::status::ok;
}

CASTLE_INLINE castle::status bind_socket(int fd, endpoint CASTLE_CONST& local, int* system_error) CASTLE_NOEXCEPT
{
    struct sockaddr_storage address;
    socklen_t length = 0;
    CASTLE_CONST castle::status converted = to_native_endpoint(local, address, length);

    // LCOV_EXCL_START
    if (!castle::succeeded(converted))
    {
        return converted;
    }
    // LCOV_EXCL_STOP

    if (::bind(fd, reinterpret_cast<struct sockaddr*>(&address), length) != 0)
    {
        return system_failure(system_error);
    }
    return castle::status::ok;
}

/** @brief Starts a connect; a connection still in progress is `time_out` with `EINPROGRESS`/`EALREADY`. */
CASTLE_INLINE castle::status connect_socket(int fd, endpoint CASTLE_CONST& remote, int* system_error) CASTLE_NOEXCEPT
{
    struct sockaddr_storage address;
    socklen_t length = 0;
    CASTLE_CONST castle::status converted = to_native_endpoint(remote, address, length);

    // LCOV_EXCL_START
    if (!castle::succeeded(converted))
    {
        return converted;
    }
    // LCOV_EXCL_STOP

    if (::connect(fd, reinterpret_cast<struct sockaddr*>(&address), length) == 0)
    {
        return castle::status::ok;
    }
    if (errno == EINPROGRESS || errno == EALREADY) // LCOV_EXCL_BR_LINE
    {
        capture_system_error(system_error);
        return castle::status::time_out;
    }
    return system_failure(system_error);
}

/** @brief Waits for writability, then confirms through `SO_ERROR` and `getpeername()` that the connect finished. */
CASTLE_INLINE castle::status finish_connect_socket(int fd, int timeout_ms, int* system_error) CASTLE_NOEXCEPT
{
    short ready = 0;
    CASTLE_CONST castle::status waited = castle::io::fd::wait_fd(fd, castle::io::fd::poll_write,
                                                                  timeout_ms, ready, system_error);
    // LCOV_EXCL_START
    if (!castle::succeeded(waited))
    {
        return waited;
    }
    // LCOV_EXCL_STOP

    int pending_error = 0;
    socklen_t error_length = static_cast<socklen_t>(sizeof(pending_error));
    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &pending_error, &error_length) != 0) // LCOV_EXCL_BR_LINE
    {
        return system_failure(system_error);
    }
    if (pending_error != 0)
    {
        if (system_error != nullptr) // LCOV_EXCL_BR_LINE
        {
            *system_error = pending_error;
        }
        return castle::status::system_call_error;
    }

    struct sockaddr_storage peer;
    socklen_t peer_length = static_cast<socklen_t>(sizeof(peer));
    if (::getpeername(fd, reinterpret_cast<struct sockaddr*>(&peer), &peer_length) != 0)
    {
        return system_failure(system_error);
    }
    return castle::status::ok;
}

CASTLE_INLINE castle::status listen_socket(int fd, int backlog, int* system_error) CASTLE_NOEXCEPT
{
    return ::listen(fd, backlog) == 0 ? castle::status::ok : system_failure(system_error); // LCOV_EXCL_BR_LINE
}

/** @brief Accepts one connection into `out`; would-block is `empty`. `remote` changes only on success. */
CASTLE_INLINE castle::status accept_socket(int listener,
                                           bool nonblocking,
                                           castle::io::fd::basic_fd& out,
                                           endpoint& remote,
                                           int* system_error) CASTLE_NOEXCEPT
{
    struct sockaddr_storage native_remote = {};
    socklen_t remote_length = static_cast<socklen_t>(sizeof(native_remote));
    struct sockaddr* address = reinterpret_cast<struct sockaddr*>(&native_remote);
#if defined(__linux__) && defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK) && defined(_GNU_SOURCE)
    CASTLE_CONST int accepted = ::accept4(listener, address, &remote_length,
                                          SOCK_CLOEXEC | (nonblocking ? SOCK_NONBLOCK : 0));
    CASTLE_CONST bool flags_applied = true;
#else
    CASTLE_CONST int accepted = ::accept(listener, address, &remote_length);
    CASTLE_CONST bool flags_applied = false;
#endif
    if (accepted < 0)
    {
        capture_system_error(system_error);
        return would_block(errno) ? castle::status::empty : castle::status::system_call_error; // LCOV_EXCL_BR_LINE
    }

    castle::io::fd::basic_fd descriptor(accepted);
    castle::status result = finish_new_socket(descriptor, nonblocking, flags_applied, system_error);
    if (castle::succeeded(result)) // LCOV_EXCL_BR_LINE
    {
        result = from_native_endpoint(native_remote, remote_length, remote);
    }
    if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
    {
        return result;
    }
    out = CASTLE_MOVE(descriptor);
    return castle::status::ok;
}

/** @brief One `send()` (or `sendto()` when `remote` is set); would-block is `full`. */
CASTLE_INLINE castle::status send_bytes(int fd,
                                        CASTLE_CONST void* data,
                                        size_type size,
                                        endpoint CASTLE_CONST* remote,
                                        size_type& sent,
                                        int* system_error) CASTLE_NOEXCEPT
{
    sent = 0U;
    if (size > max_transfer_size) // LCOV_EXCL_BR_LINE
    {
        return castle::status::out_of_range;
    }
    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;
#endif
    ssize_t result = -1;
    if (remote == nullptr)
    {
        result = ::send(fd, data, size, flags);
    }
    else
    {
        struct sockaddr_storage address;
        socklen_t length = 0;
        CASTLE_CONST castle::status converted = to_native_endpoint(*remote, address, length);
        if (!castle::succeeded(converted)) // LCOV_EXCL_BR_LINE
        {
            return converted;
        }
        result = ::sendto(fd, data, size, flags, reinterpret_cast<struct sockaddr*>(&address), length);
    }
    if (result < 0)
    {
        capture_system_error(system_error);
        return would_block(errno) ? castle::status::full : castle::status::system_call_error;
    }
    sent = static_cast<size_type>(result);
    return castle::status::ok;
}

/**
 * @brief One `recvfrom()`; would-block is `empty`.
 * @param source Receives the datagram source when not null.
 * @param datagram True for a datagram socket, whose real length is requested so a short buffer shows as `truncated`.
 * @param truncated Set when datagram bytes that did not fit `capacity` were dropped.
 */
CASTLE_INLINE castle::status receive_bytes(int fd,
                                           void* buffer,
                                           size_type capacity,
                                           endpoint* source,
                                           bool datagram,
                                           size_type& received,
                                           bool& truncated,
                                           int* system_error) CASTLE_NOEXCEPT
{
    received = 0U;
    truncated = false;
    if (capacity > max_transfer_size) // LCOV_EXCL_BR_LINE
    {
        return castle::status::out_of_range;
    }

    struct sockaddr_storage native_source = {};
    socklen_t source_length = static_cast<socklen_t>(sizeof(native_source));
    struct sockaddr* address = source != nullptr ? reinterpret_cast<struct sockaddr*>(&native_source) : nullptr;
    CASTLE_CONST ssize_t result = ::recvfrom(fd, buffer, capacity, datagram ? datagram_length_flag : 0, address,
                                              source != nullptr ? &source_length : nullptr);
    if (result < 0)
    {
        capture_system_error(system_error);
        return would_block(errno) ? castle::status::empty : castle::status::system_call_error;
    }
    if (source != nullptr)
    {
        CASTLE_CONST castle::status converted = from_native_endpoint(native_source, source_length, *source);
        if (!castle::succeeded(converted)) // LCOV_EXCL_BR_LINE
        {
            return converted;
        }
    }
    received = static_cast<size_type>(result);
    if (received > capacity)
    {
        received = capacity;
        truncated = true;
    }
    return castle::status::ok;
}

CASTLE_INLINE castle::status wait_socket(int fd, short events, int timeout_ms, short& revents,
                                         int* system_error) CASTLE_NOEXCEPT
{
    return castle::io::fd::wait_fd(fd, events, timeout_ms, revents, system_error);
}

CASTLE_INLINE castle::status wait_socket_readable(int fd, int timeout_ms, int* system_error) CASTLE_NOEXCEPT
{
    short ready = 0;
    return castle::io::fd::wait_fd(fd, castle::io::fd::poll_read, timeout_ms, ready, system_error);
}

CASTLE_INLINE castle::status set_integer_option(int fd, int level, int option, bool enabled,
                                                int* system_error) CASTLE_NOEXCEPT
{
    CASTLE_CONST int value = enabled ? 1 : 0; // LCOV_EXCL_BR_LINE
    if (::setsockopt(fd, level, option, &value, static_cast<socklen_t>(sizeof(value))) != 0) // LCOV_EXCL_BR_LINE
    {
        return system_failure(system_error);
    }
    return castle::status::ok;
}

CASTLE_INLINE castle::status set_reuse_address(int fd, bool enabled, int* system_error) CASTLE_NOEXCEPT
{
    return set_integer_option(fd, SOL_SOCKET, SO_REUSEADDR, enabled, system_error);
}

CASTLE_INLINE castle::status set_keep_alive(int fd, bool enabled, int* system_error) CASTLE_NOEXCEPT
{
    return set_integer_option(fd, SOL_SOCKET, SO_KEEPALIVE, enabled, system_error);
}

CASTLE_INLINE castle::status set_no_delay(int fd, bool enabled, int* system_error) CASTLE_NOEXCEPT
{
    return set_integer_option(fd, IPPROTO_TCP, TCP_NODELAY, enabled, system_error);
}

CASTLE_INLINE castle::status shutdown_socket(int fd, bool receive, bool send, int* system_error) CASTLE_NOEXCEPT
{
    CASTLE_CONST int how = (receive && send) ? SHUT_RDWR : (receive ? SHUT_RD : SHUT_WR); // LCOV_EXCL_BR_LINE
    return ::shutdown(fd, how) == 0 ? castle::status::ok : system_failure(system_error); // LCOV_EXCL_BR_LINE
}

/** @brief Reads the connected peer (`peer`) or the bound local endpoint. */
CASTLE_INLINE castle::status query_endpoint(int fd, bool peer, endpoint& out, int* system_error) CASTLE_NOEXCEPT
{
    struct sockaddr_storage address = {};
    socklen_t length = static_cast<socklen_t>(sizeof(address));
    struct sockaddr* native = reinterpret_cast<struct sockaddr*>(&address);
    CASTLE_CONST int result = peer ? ::getpeername(fd, native, &length) : ::getsockname(fd, native, &length);
    if (result != 0) // LCOV_EXCL_BR_LINE
    {
        return system_failure(system_error);
    }
    return from_native_endpoint(address, length, out);
}

#else

CASTLE_INLINE castle::status open_socket(address_family, bool, bool, castle::io::fd::basic_fd&,
                                         int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status bind_socket(int, endpoint CASTLE_CONST&, int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status connect_socket(int, endpoint CASTLE_CONST&, int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status finish_connect_socket(int, int, int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status listen_socket(int, int, int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status accept_socket(int, bool, castle::io::fd::basic_fd&, endpoint&,
                                           int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status send_bytes(int, CASTLE_CONST void*, size_type, endpoint CASTLE_CONST*, size_type&,
                                        int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status receive_bytes(int, void*, size_type, endpoint*, bool, size_type&, bool&,
                                           int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status wait_socket(int, short, int, short&, int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status wait_socket_readable(int, int, int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status set_reuse_address(int, bool, int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status set_keep_alive(int, bool, int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status set_no_delay(int, bool, int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status shutdown_socket(int, bool, bool, int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

CASTLE_INLINE castle::status query_endpoint(int, bool, endpoint&, int* system_error) CASTLE_NOEXCEPT
{
    return unsupported(system_error);
}

#endif

} // namespace detail
} // namespace net
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_NET_DETAIL_NATIVE_HPP
