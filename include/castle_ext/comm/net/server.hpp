// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file server.hpp
 * @brief Fixed-capacity TCP and UDP servers with per-peer inline callbacks.
 *
 * TCP servers accept connections into a compile-time-sized peer table. UDP
 * servers learn peers from datagram source endpoints and keep a bounded table
 * of those endpoints. Every slot may own its own receive callback, initialized
 * from an optional server default. The peer table, the receive buffer and every
 * callback are members of the server, so `sizeof(server)` is its whole memory cost.
 */
#ifndef CASTLE_EXT_COMM_NET_SERVER_HPP
#define CASTLE_EXT_COMM_NET_SERVER_HPP

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
 * @brief Bounded TCP connection / UDP peer manager.
 * @tparam Protocol `socket_protocol::tcp` or `socket_protocol::udp`.
 * @tparam MaxClients Maximum accepted TCP connections or remembered UDP peers.
 * @tparam ReceiveBufferSize Maximum bytes processed in one receive operation; held as a member.
 * @tparam CallbackStorageSize Inline bytes reserved for each peer callback.
 *
 * Callback signature: `void(client_handle, endpoint const&, uint8_t const*, size_type)`.
 * Callbacks execute synchronously on the thread calling `process_client()` or `process_datagram()`,
 * after socket I/O has completed, and run in place so mutable callables keep their state. A callback may
 * replace or clear its own callback or close its own peer (both take effect when it returns), and may
 * send, accept or manage other peers; it must not call `process_client()`/`process_datagram()` (rejected
 * with `invalid_config`) or destroy the server. The server is not internally synchronized; callers
 * sharing it across threads must serialize its operations.
 */
template <socket_protocol Protocol,
          size_type MaxClients = 4U,
          size_type ReceiveBufferSize = 256U,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
class basic_server
{
    static_assert(MaxClients > 0U, "Network server must reserve at least one client slot");
    static_assert(ReceiveBufferSize > 0U, "Network server receive buffer must not be empty");
    static_assert(CallbackStorageSize > 0U, "Network server callback storage must not be empty");

public:
    /** @brief Generation-checked handle to a TCP connection or remembered UDP peer. */
    struct client_handle
    {
        size_type index;
        uint32_t generation;

        CASTLE_CONSTEXPR client_handle() CASTLE_NOEXCEPT
            : index(invalid_index())
            , generation(0U)
        {
        }

        CASTLE_CONSTEXPR client_handle(size_type slot, uint32_t value) CASTLE_NOEXCEPT
            : index(slot)
            , generation(value)
        {
        }

        /** @brief Tests whether this handle has the invalid sentinel index. */
        CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_valid() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return index != invalid_index() && generation != 0U;
        }

        /** @brief Returns the invalid slot index. */
        static CASTLE_CONSTEXPR size_type invalid_index() CASTLE_NOEXCEPT
        {
            return static_cast<size_type>(-1);
        }

        /** @brief Tests identity of two generation-checked handles. */
        CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(client_handle CASTLE_CONST& other) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return index == other.index && generation == other.generation;
        }

        /** @brief Tests handle inequality. */
        CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(client_handle CASTLE_CONST& other) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return !(*this == other);
        }
    };

    using socket_type = basic_socket<Protocol>;
    using callback_type = castle::callbacks::function<void(client_handle, endpoint CASTLE_CONST&,
                                                           uint8_t CASTLE_CONST*, size_type),
                                                       CallbackStorageSize>;

    /** @brief Constructs an unopened server with no clients or callbacks. */
    basic_server() CASTLE_DEFAULT;

    basic_server(CASTLE_CONST basic_server&) CASTLE_DELETE;
    basic_server& operator=(CASTLE_CONST basic_server&) CASTLE_DELETE;

    /**
     * @brief Opens, binds and prepares the TCP listener or UDP socket.
     *
     * TCP listeners enable `SO_REUSEADDR`; UDP sockets do not, so a second server cannot silently share a port.
     */
    castle::status open(endpoint CASTLE_CONST& local,
                        int backlog = 8,
                        bool nonblocking = true,
                        int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (listener_.is_open())
        {
            return castle::status::invalid_config;
        }

        castle::status result = listener_.open(local.family(), nonblocking, system_error);
        if (castle::succeeded(result) && Protocol == socket_protocol::tcp)
        {
            result = listener_.set_reuse_address(true, system_error);
        }
        if (castle::succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            result = listener_.bind(local, system_error);
        }
        if (castle::succeeded(result) && Protocol == socket_protocol::tcp)
        {
            result = listener_.listen(backlog, system_error);
        }
        if (!castle::succeeded(result))
        {
            (void)listener_.close(nullptr);
        }
        return result;
    }

    /**
     * @brief Closes the listening socket and every peer, then resets the peer table.
     * @return `ok`, or the first failure; `system_error` describes that failure.
     */
    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::status result = listener_.close(system_error);
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            if (!slots_[i].used)
            {
                continue;
            }
            CASTLE_CONST castle::status closed = release_slot(i, castle::succeeded(result) ? system_error : nullptr); // LCOV_EXCL_BR_LINE
            if (castle::succeeded(result)) // LCOV_EXCL_BR_LINE
            {
                result = closed;
            }
        }
        return result;
    }

    /** @brief Returns true while the server's listening/bound socket is open. */
    CASTLE_NODISCARD bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return listener_.is_open(); }
    /** @brief Returns the listening/bound socket's native descriptor, or -1 when closed. */
    CASTLE_NODISCARD int native_handle() CASTLE_CONST CASTLE_NOEXCEPT { return listener_.native_handle(); }
    /** @brief Returns the number of active TCP connections or remembered UDP peers. */
    CASTLE_NODISCARD size_type client_count() CASTLE_CONST CASTLE_NOEXCEPT { return client_count_; }
    /** @brief Returns the compile-time client/peer capacity. */
    static CASTLE_CONSTEXPR size_type client_capacity() CASTLE_NOEXCEPT { return MaxClients; }
    /** @brief Provides access to the underlying TCP listener or UDP socket. */
    CASTLE_NODISCARD socket_type& socket() CASTLE_NOEXCEPT { return listener_; }

    /**
     * @brief Returns the handle of the active client in slot `index`, or an invalid handle.
     *
     * Iterating `0 .. client_capacity()` visits every client, for example to pick a victim when a UDP peer
     * table is full.
     */
    CASTLE_NODISCARD client_handle client_at(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (index >= MaxClients || !slots_[index].used || slots_[index].closing)
        {
            return client_handle();
        }
        return client_handle(index, slots_[index].generation);
    }

    /**
     * @brief Returns the descriptor to poll for one client, or -1 for a stale handle.
     *
     * A TCP client has its own descriptor; UDP peers all share the listening socket's descriptor. The
     * result can be registered with `castle::io::fd::basic_fd_event_manager` to wait on every client at once.
     */
    CASTLE_NODISCARD int client_native_handle(client_handle handle) CASTLE_CONST CASTLE_NOEXCEPT
    {
        slot CASTLE_CONST* target = find_slot(handle);
        if (target == nullptr)
        {
            return -1;
        }
        return Protocol == socket_protocol::tcp ? target->socket.native_handle() : listener_.native_handle();
    }

    /** @brief Stores the callback copied into each subsequently accepted/discovered peer slot. */
    template <typename Callback>
    castle::status set_default_callback(Callback&& callback)
    {
        callback_type next(CASTLE_FORWARD<Callback>(callback));
        if (!next) // LCOV_EXCL_BR_LINE
        {
            return castle::status::invalid_callback;
        }
        default_callback_ = CASTLE_MOVE(next);
        return castle::status::ok;
    }

    /** @brief Clears the default callback; already accepted/discovered peers retain theirs. */
    void clear_default_callback() CASTLE_NOEXCEPT { default_callback_ = nullptr; }

    /**
     * @brief Accepts one TCP connection into the first available fixed slot.
     * @param timeout_ms Readiness wait in milliseconds; zero polls, negative waits forever.
     * @return `ok`, `full` when all client slots are occupied (checked first, so it never waits), `empty` when
     *         no connection arrived in time, `invalid_config` for a UDP server, `not_configured`, or a socket error.
     *         A pending connection that found no free slot stays in the OS accept queue.
     */
    castle::status accept_client(client_handle& out, int timeout_ms = 0,
                                 int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        out = client_handle();
        detail::clear_system_error(system_error);
        CASTLE_IF_CONSTEXPR (Protocol != socket_protocol::tcp)
        {
            return castle::status::invalid_config;
        }
        else
        {
            if (!listener_.is_open())
            {
                return castle::status::not_configured;
            }
            CASTLE_CONST size_type index = find_free_slot();
            if (index == MaxClients)
            {
                return castle::status::full;
            }

            CASTLE_CONST castle::status waited = listener_.wait_readable(timeout_ms, system_error);
            if (waited == castle::status::time_out)
            {
                return castle::status::empty;
            }
            if (!castle::succeeded(waited)) // LCOV_EXCL_BR_LINE
            {
                return waited;
            }

            endpoint remote;
            CASTLE_CONST castle::status result = listener_.accept(slots_[index].socket, remote, system_error);
            if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
            {
                return result;
            }
            out = activate_slot(index, remote);
            return castle::status::ok;
        }
    }

    /** @brief Replaces the receive callback for one active client/peer handle. */
    template <typename Callback>
    castle::status set_client_callback(client_handle handle, Callback&& callback)
    {
        slot* target = find_slot(handle);
        if (target == nullptr)
        {
            return castle::status::not_found;
        }
        callback_type next(CASTLE_FORWARD<Callback>(callback));
        if (!next) // LCOV_EXCL_BR_LINE
        {
            return castle::status::invalid_callback;
        }
        dispatcher_.replace(target->callback, CASTLE_MOVE(next));
        return castle::status::ok;
    }

    /** @brief Clears the receive callback for one active client/peer handle. */
    castle::status clear_client_callback(client_handle handle) CASTLE_NOEXCEPT
    {
        slot* target = find_slot(handle);
        if (target == nullptr) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_found;
        }
        dispatcher_.clear(target->callback);
        return castle::status::ok;
    }

    /**
     * @brief Waits for and processes one TCP client's next stream chunk.
     * @param handle Generation-checked client handle returned by `accept_client()`.
     * @param timeout_ms Readiness wait in milliseconds; zero polls, negative waits forever.
     * @return `ok` after data is consumed, `empty` when no data is ready, `not_configured` after peer EOF,
     *         `not_found` for a stale handle, `invalid_config` for a UDP server or a call from a callback,
     *         or a socket error status. EOF and fatal receive errors such as a connection reset close the
     *         connection and free its slot.
     */
    castle::status process_client(client_handle handle, int timeout_ms = 0,
                                  int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (Protocol != socket_protocol::tcp || dispatcher_.is_dispatching())
        {
            return castle::status::invalid_config;
        }
        slot* target = find_slot(handle);
        if (target == nullptr) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_found;
        }

        castle::status result = target->socket.wait_readable(timeout_ms, system_error);
        if (result == castle::status::time_out) // LCOV_EXCL_BR_LINE
        {
            return castle::status::empty;
        }
        if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            return result;
        }

        size_type received = 0U;
        bool peer_closed = false;
        result = target->socket.receive_some(buffer_, ReceiveBufferSize, received, system_error, &peer_closed);
        if (peer_closed)
        {
            (void)release_slot(handle.index, nullptr);
            return castle::status::not_configured;
        }
        if (result == castle::status::system_call_error)
        {
            (void)release_slot(handle.index, nullptr);
            return result;
        }
        if (!castle::succeeded(result))
        {
            return result;
        }
        dispatch(handle, received);
        return castle::status::ok;
    }

    /**
     * @brief Waits for one UDP datagram, tracks its source as a bounded peer, and dispatches it.
     * @param out Receives the generation-checked peer handle for the datagram source.
     * @param timeout_ms Readiness wait in milliseconds; zero polls, negative waits forever.
     * @param source Optional; receives the datagram's source endpoint when the result is `ok` or `full`.
     * @return `ok`, `empty` on timeout/would-block, `full` when a new peer arrives at capacity (the datagram is
     *         dropped and no callback runs; `source` tells who it came from), `data_loss` for a datagram larger
     *         than the receive buffer (dropped), `invalid_config` for a TCP server or a call from a callback,
     *         or the underlying socket error status.
     */
    castle::status process_datagram(client_handle& out, int timeout_ms = 0,
                                    int* system_error = nullptr, endpoint* source = nullptr) CASTLE_NOEXCEPT
    {
        out = client_handle();
        if (source != nullptr)
        {
            *source = endpoint();
        }
        detail::clear_system_error(system_error);
        if (Protocol != socket_protocol::udp || dispatcher_.is_dispatching())
        {
            return castle::status::invalid_config;
        }
        if (!listener_.is_open())
        {
            return castle::status::not_configured;
        }

        castle::status result = listener_.wait_readable(timeout_ms, system_error);
        if (result == castle::status::time_out)
        {
            return castle::status::empty;
        }
        if (!castle::succeeded(result))
        {
            return result;
        }

        size_type received = 0U;
        endpoint remote;
        result = listener_.receive_from(buffer_, ReceiveBufferSize, received, remote, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
        if (source != nullptr)
        {
            *source = remote;
        }

        size_type index = find_peer(remote);
        if (index == MaxClients)
        {
            index = find_free_slot();
            if (index == MaxClients)
            {
                return castle::status::full;
            }
            (void)activate_slot(index, remote);
        }
        out = client_handle(index, slots_[index].generation);
        dispatch(out, received);
        return castle::status::ok;
    }

    /** @brief Sends bytes to a TCP connection or one UDP peer identified by handle. */
    castle::status send_some(client_handle handle, CASTLE_CONST void* data, size_type size,
                             size_type& bytes_sent, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        bytes_sent = 0U;
        detail::clear_system_error(system_error);
        slot* target = find_slot(handle);
        if (target == nullptr) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_found;
        }
        if (Protocol == socket_protocol::tcp)
        {
            return target->socket.send_some(data, size, bytes_sent, system_error);
        }
        return listener_.send_to(data, size, target->peer, bytes_sent, system_error);
    }

    /** @brief Writes all bytes to one TCP peer; UDP servers must use `send_some()` for datagrams. */
    castle::status send_all(client_handle handle, CASTLE_CONST void* data, size_type size,
                            size_type& bytes_sent, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        bytes_sent = 0U;
        detail::clear_system_error(system_error);
        if (Protocol != socket_protocol::tcp)
        {
            return castle::status::invalid_config;
        }
        slot* target = find_slot(handle);
        if (target == nullptr)
        {
            return castle::status::not_found;
        }
        return target->socket.send_all(data, size, bytes_sent, system_error);
    }

    /** @brief Returns the remote endpoint for one active client/peer. */
    castle::status client_endpoint(client_handle handle, endpoint& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        slot CASTLE_CONST* target = find_slot(handle);
        if (target == nullptr)
        {
            return castle::status::not_found;
        }
        out = target->peer;
        return castle::status::ok;
    }

    /** @brief Closes a TCP connection or forgets a UDP peer, invalidating its handle. */
    castle::status close_client(client_handle handle, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (find_slot(handle) == nullptr)
        {
            detail::clear_system_error(system_error);
            return castle::status::not_found;
        }
        return release_slot(handle.index, system_error);
    }

private:
    struct slot
    {
        socket_type socket;
        endpoint peer;
        callback_type callback;
        uint32_t generation = 0U;
        bool used = false;
        bool closing = false; // release requested while this slot's callback was running
    };

    static uint32_t next_generation(uint32_t value) CASTLE_NOEXCEPT
    {
        ++value;
        return value == 0U ? 1U : value; // LCOV_EXCL_BR_LINE
    }

    size_type find_free_slot() CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            if (!slots_[i].used)
            {
                return i;
            }
        }
        return MaxClients;
    }

    size_type find_peer(endpoint CASTLE_CONST& peer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxClients; ++i)
        {
            if (slots_[i].used && slots_[i].peer == peer)
            {
                return i;
            }
        }
        return MaxClients;
    }

    slot CASTLE_CONST* find_slot(client_handle handle) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!handle.is_valid() || handle.index >= MaxClients)
        {
            return nullptr;
        }
        slot CASTLE_CONST& target = slots_[handle.index];
        return target.used && !target.closing && target.generation == handle.generation ? &target : nullptr;
    }

    slot* find_slot(client_handle handle) CASTLE_NOEXCEPT
    {
        return const_cast<slot*>(static_cast<basic_server CASTLE_CONST&>(*this).find_slot(handle));
    }

    /** @brief Marks a free slot as used by `peer` and returns its fresh handle. */
    client_handle activate_slot(size_type index, endpoint CASTLE_CONST& peer) CASTLE_NOEXCEPT
    {
        slot& target = slots_[index];
        target.peer = peer;
        target.callback = default_callback_;
        target.used = true;
        target.generation = next_generation(target.generation);
        ++client_count_;
        return client_handle(index, target.generation);
    }

    /** @brief Runs the slot's callback on the receive buffer, then completes a release it requested. */
    void dispatch(client_handle handle, size_type received) CASTLE_NOEXCEPT
    {
        slot& target = slots_[handle.index];
        if (target.callback)
        {
            dispatcher_.invoke(target.callback, handle, target.peer, buffer_, received);
        }
        if (target.closing)
        {
            (void)release_slot(handle.index, nullptr);
        }
    }

    /** @brief Frees a slot now, or once its running callback returns. */
    castle::status release_slot(size_type index, int* system_error) CASTLE_NOEXCEPT
    {
        slot& target = slots_[index];
        if (dispatcher_.is_running(target.callback))
        {
            target.closing = true;
            detail::clear_system_error(system_error);
            return castle::status::ok;
        }

        castle::status result = castle::status::ok;
        if (Protocol == socket_protocol::tcp)
        {
            result = target.socket.close(system_error);
            if (result == castle::status::not_configured) // LCOV_EXCL_BR_LINE
            {
                result = castle::status::ok;
            }
        }
        else
        {
            detail::clear_system_error(system_error);
        }
        target.callback = nullptr;
        target.peer = endpoint();
        target.used = false;
        target.closing = false;
        target.generation = next_generation(target.generation);
        if (client_count_ > 0U) // LCOV_EXCL_BR_LINE
        {
            --client_count_;
        }
        return result;
    }

    socket_type listener_;
    slot slots_[MaxClients];
    callback_type default_callback_;
    detail::callback_dispatcher<callback_type> dispatcher_;
    size_type client_count_ = 0U;
    uint8_t buffer_[ReceiveBufferSize]{};
};

/** @brief Fixed-capacity TCP server; `MaxClients` is the maximum simultaneous connections. */
template <size_type MaxClients = 4U,
          size_type ReceiveBufferSize = 256U,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
using tcp_server = basic_server<socket_protocol::tcp, MaxClients, ReceiveBufferSize, CallbackStorageSize>;

/** @brief Fixed-capacity UDP server; `MaxClients` is the maximum remembered source endpoints. */
template <size_type MaxClients = 4U,
          size_type ReceiveBufferSize = 256U,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
using udp_server = basic_server<socket_protocol::udp, MaxClients, ReceiveBufferSize, CallbackStorageSize>;

} // namespace net
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_NET_SERVER_HPP
