#include <gtest/gtest.h>

#include "castle_ext/comm/net/server.hpp"
#include "castle_ext/comm/net/client.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <errno.h>
    #include <sys/socket.h>

    #include <chrono>
#endif

namespace
{

using namespace castle::comm::net;
using castle::size_type;

#if CASTLE_USING_POSIX_APIS

TEST(NetServer, TcpServerAcceptsFixedCapacityClientsAndDispatchesCallbacks)
{
    using server_type = tcp_server<1U, 64U>;
    server_type server;
    size_type calls = 0U;
    typename server_type::client_handle callback_handle;
    ASSERT_EQ(server.set_default_callback(
        [&calls, &callback_handle](server_type::client_handle handle, endpoint const& remote,
                                  uint8_t const* data, size_type size)
        {
            if (remote.port() != 0U && size == 6U && data[0] == static_cast<uint8_t>('h'))
            {
                ++calls;
                callback_handle = handle;
            }
        }), castle::status::ok);
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(0U), 4), castle::status::ok);
    endpoint local;
    ASSERT_EQ(server.socket().local_endpoint(local), castle::status::ok);

    tcp_client<64U> first;
    ASSERT_EQ(first.connect(local, 1000), castle::status::ok);
    typename server_type::client_handle handle;
    ASSERT_EQ(server.accept_client(handle, 1000), castle::status::ok);
    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(server.client_count(), 1U);
    typename server_type::client_handle no_pending_client;
    EXPECT_EQ(server.accept_client(no_pending_client), castle::status::full);
    EXPECT_FALSE(no_pending_client.is_valid());

    const char hello[] = "hello";
    size_type bytes = 0U;
    ASSERT_EQ(first.send_all(hello, sizeof(hello), bytes), castle::status::ok);
    ASSERT_EQ(server.process_client(handle, 1000), castle::status::ok);
    EXPECT_EQ(calls, 1U);
    EXPECT_EQ(callback_handle, handle);

    endpoint remote;
    ASSERT_EQ(server.client_endpoint(handle, remote), castle::status::ok);
    EXPECT_NE(remote.port(), 0U);
    const char reply[] = "reply";
    ASSERT_EQ(server.send_all(handle, reply, sizeof(reply), bytes), castle::status::ok);
    size_type received = 0U;
    ASSERT_EQ(first.set_receive_callback([&received](endpoint const&, uint8_t const*, size_type size)
    {
        received = size;
    }), castle::status::ok);
    ASSERT_EQ(first.process_once(1000), castle::status::ok);
    EXPECT_EQ(received, sizeof(reply));

    tcp_client<64U> second;
    ASSERT_EQ(second.connect(local, 1000), castle::status::ok);
    typename server_type::client_handle rejected;
    EXPECT_EQ(server.accept_client(rejected), castle::status::full);
    EXPECT_FALSE(rejected.is_valid());
    EXPECT_EQ(server.client_count(), 1U);

    EXPECT_EQ(server.close_client(handle), castle::status::ok);
    EXPECT_EQ(server.close_client(handle), castle::status::not_found);
    EXPECT_EQ(server.set_client_callback(handle, [](server_type::client_handle, endpoint const&, uint8_t const*, size_type) {}),
              castle::status::not_found);
    EXPECT_EQ(server.client_count(), 0U);
    typename server_type::client_handle second_handle;
    ASSERT_EQ(server.accept_client(second_handle), castle::status::ok);
    EXPECT_EQ(server.client_count(), 1U);
    EXPECT_EQ(server.close_client(second_handle), castle::status::ok);
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(NetServer, TcpAcceptPollReturnsEmptyWhenNoConnectionIsPending)
{
    tcp_server<2U> server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(0U)), castle::status::ok);
    tcp_server<2U>::client_handle handle;
    EXPECT_EQ(server.accept_client(handle, 0), castle::status::empty);
    EXPECT_FALSE(handle.is_valid());
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(NetServer, TcpServerRejectsUdpOnlyOperationsAndCanOverrideCallback)
{
    tcp_server<2U> server;
    tcp_server<2U>::client_handle handle;
    EXPECT_EQ(server.accept_client(handle), castle::status::not_configured);
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(0U)), castle::status::ok);
    EXPECT_EQ(server.process_datagram(handle), castle::status::invalid_config);
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(NetServer, UdpServerLearnsPeersAndUsesPerPeerCallbacks)
{
    using server_type = udp_server<2U, 32U>;
    server_type server;
    size_type calls = 0U;
    ASSERT_EQ(server.set_default_callback(
        [&calls](server_type::client_handle, endpoint const& source, uint8_t const* data, size_type size)
        {
            if (source.port() != 0U && size == 4U && data[0] == static_cast<uint8_t>('p')) { ++calls; }
        }), castle::status::ok);
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(0U)), castle::status::ok);
    endpoint local;
    ASSERT_EQ(server.socket().local_endpoint(local), castle::status::ok);

    udp_client<32U> first;
    udp_client<32U> second;
    ASSERT_EQ(first.open(), castle::status::ok);
    ASSERT_EQ(second.open(), castle::status::ok);
    const char packet[] = "pkt"; // four bytes including the terminator
    size_type bytes = 0U;
    ASSERT_EQ(first.send_to(packet, sizeof(packet), local, bytes), castle::status::ok);

    server_type::client_handle first_peer;
    ASSERT_EQ(server.process_datagram(first_peer, 1000), castle::status::ok);
    EXPECT_TRUE(first_peer.is_valid());
    EXPECT_EQ(server.client_count(), 1U);
    EXPECT_EQ(calls, 1U);

    size_type per_peer_calls = 0U;
    ASSERT_EQ(server.set_client_callback(first_peer,
        [&per_peer_calls](server_type::client_handle, endpoint const&, uint8_t const*, size_type)
        {
            ++per_peer_calls;
        }), castle::status::ok);
    ASSERT_EQ(first.send_to(packet, sizeof(packet), local, bytes), castle::status::ok);
    server_type::client_handle same_peer;
    ASSERT_EQ(server.process_datagram(same_peer, 1000), castle::status::ok);
    EXPECT_EQ(same_peer, first_peer);
    EXPECT_EQ(per_peer_calls, 1U);

    ASSERT_EQ(second.send_to(packet, sizeof(packet), local, bytes), castle::status::ok);
    server_type::client_handle second_peer;
    ASSERT_EQ(server.process_datagram(second_peer, 1000), castle::status::ok);
    EXPECT_NE(second_peer, first_peer);
    EXPECT_EQ(server.client_count(), 2U);

    ASSERT_EQ(server.send_some(first_peer, "ack", 4U, bytes), castle::status::ok);
    size_type received = 0U;
    ASSERT_EQ(first.set_receive_callback([&received](endpoint const&, uint8_t const*, size_type size)
    {
        received = size;
    }), castle::status::ok);
    ASSERT_EQ(first.process_once(1000), castle::status::ok);
    EXPECT_EQ(received, 4U);

    EXPECT_EQ(server.send_all(first_peer, "x", 1U, bytes), castle::status::invalid_config);
    EXPECT_EQ(server.close_client(first_peer), castle::status::ok);
    EXPECT_EQ(server.close_client(first_peer), castle::status::not_found);
    EXPECT_EQ(server.client_count(), 1U);
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(NetServer, UdpCapacityOverflowAndInvalidConfiguration)
{
    using server_type = udp_server<1U, 16U>;
    server_type server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(0U)), castle::status::ok);
    server_type::client_handle rejected_accept;
    EXPECT_EQ(server.accept_client(rejected_accept), castle::status::invalid_config);
    endpoint local;
    ASSERT_EQ(server.socket().local_endpoint(local), castle::status::ok);
    udp_client<16U> first;
    udp_client<16U> second;
    ASSERT_EQ(first.open(), castle::status::ok);
    ASSERT_EQ(second.open(), castle::status::ok);
    size_type bytes = 0U;
    ASSERT_EQ(first.send_to("1", 2U, local, bytes), castle::status::ok);
    server_type::client_handle handle;
    ASSERT_EQ(server.process_datagram(handle, 1000), castle::status::ok);
    ASSERT_EQ(second.send_to("2", 2U, local, bytes), castle::status::ok);
    server_type::client_handle overflow;
    EXPECT_EQ(server.process_datagram(overflow, 1000), castle::status::full);
    EXPECT_FALSE(overflow.is_valid());
    EXPECT_EQ(server.client_count(), 1U);
    EXPECT_EQ(server.close_client(handle), castle::status::ok);
}

TEST(NetServer, TcpEofReleasesSlotAndInvalidatesHandle)
{
    tcp_server<1U> server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(0U)), castle::status::ok);
    endpoint local;
    ASSERT_EQ(server.socket().local_endpoint(local), castle::status::ok);
    tcp_client<> client;
    ASSERT_EQ(client.connect(local, 1000), castle::status::ok);
    tcp_server<1U>::client_handle handle;
    ASSERT_EQ(server.accept_client(handle, 1000), castle::status::ok);
    ASSERT_EQ(client.socket().shutdown(shutdown_mode::send), castle::status::ok);
    EXPECT_EQ(server.process_client(handle, 1000), castle::status::not_configured);
    EXPECT_EQ(server.client_count(), 0U);
    EXPECT_EQ(server.client_endpoint(handle, local), castle::status::not_found);
}

// Counts live copies of a callback capture so tests can see when the running callable is destroyed.
struct live_capture
{
    explicit live_capture(int* counter) : live(counter) { ++*live; }
    live_capture(live_capture const& other) : live(other.live) { ++*live; }
    live_capture(live_capture&& other) noexcept : live(other.live) { ++*live; }
    ~live_capture() { --*live; }
    int* live;
};

template <typename Server>
void open_loopback(Server& server, endpoint& local)
{
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(server.socket().local_endpoint(local), castle::status::ok);
}

TEST(NetServer, TcpResetConnectionFreesItsSlot)
{
    using server_type = tcp_server<1U>;
    server_type server;
    endpoint local;
    open_loopback(server, local);

    server_type::client_handle handle;
    {
        tcp_client<> client;
        ASSERT_EQ(client.connect(local, 1000), castle::status::ok);
        ASSERT_EQ(server.accept_client(handle, 1000), castle::status::ok);
        struct linger abortive = {1, 0};
        ASSERT_EQ(::setsockopt(client.native_handle(), SOL_SOCKET, SO_LINGER, &abortive, sizeof(abortive)), 0);
    } // closing with a zero linger resets the connection
    int system_error = 0;
    EXPECT_EQ(server.process_client(handle, 1000, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, ECONNRESET);
    EXPECT_EQ(server.client_count(), 0U);
    EXPECT_EQ(server.process_client(handle), castle::status::not_found);

    tcp_client<> next;
    ASSERT_EQ(next.connect(local, 1000), castle::status::ok);
    server_type::client_handle replacement;
    EXPECT_EQ(server.accept_client(replacement, 1000), castle::status::ok);
    EXPECT_NE(replacement, handle);
}

TEST(NetServer, TcpCallbackMayCloseItsOwnClientWhileRunning)
{
    using server_type = tcp_server<2U, 32U>;
    struct probe
    {
        int live = 0;
        int live_inside = -1;
        castle::status first_close = castle::status::unknown_error;
        castle::status second_close = castle::status::unknown_error;
        castle::status send_after_close = castle::status::unknown_error;
        size_type count_inside = 0U;
    } observed;

    server_type server;
    endpoint local;
    open_loopback(server, local);
    tcp_client<> client;
    ASSERT_EQ(client.connect(local, 1000), castle::status::ok);
    server_type::client_handle handle;
    ASSERT_EQ(server.accept_client(handle, 1000), castle::status::ok);
    {
        live_capture capture(&observed.live);
        ASSERT_EQ(server.set_client_callback(handle,
            [capture, &server, &observed](server_type::client_handle self, endpoint const&, uint8_t const*, size_type)
            {
                observed.first_close = server.close_client(self);
                observed.second_close = server.close_client(self);
                size_type sent = 0U;
                observed.send_after_close = server.send_some(self, "x", 1U, sent);
                observed.live_inside = *capture.live;
                observed.count_inside = server.client_count();
            }), castle::status::ok);
    }
    size_type bytes = 0U;
    ASSERT_EQ(client.send_all("hi", 3U, bytes), castle::status::ok);
    ASSERT_EQ(server.process_client(handle, 1000), castle::status::ok);

    EXPECT_EQ(observed.first_close, castle::status::ok);
    EXPECT_EQ(observed.second_close, castle::status::not_found);
    EXPECT_EQ(observed.send_after_close, castle::status::not_found);
    EXPECT_EQ(observed.live_inside, 1) << "the running callable must outlive its own close";
    EXPECT_EQ(observed.count_inside, 1U);
    EXPECT_EQ(observed.live, 0);
    EXPECT_EQ(server.client_count(), 0U);
    endpoint unused;
    EXPECT_EQ(server.client_endpoint(handle, unused), castle::status::not_found);
    EXPECT_EQ(client.process_once(1000), castle::status::not_configured); // the connection was really closed
}

TEST(NetServer, CallbackMayReplaceAndClearItsOwnCallback)
{
    using server_type = udp_server<2U, 16U>;
    server_type server;
    endpoint local;
    open_loopback(server, local);
    udp_client<> client;
    ASSERT_EQ(client.open(), castle::status::ok);
    size_type bytes = 0U;

    int replacement_calls = 0;
    ASSERT_EQ(server.set_default_callback(
        [&server, &replacement_calls](server_type::client_handle self, endpoint const&, uint8_t const*, size_type)
        {
            EXPECT_EQ(server.set_client_callback(self,
                [&replacement_calls, &server](server_type::client_handle again, endpoint const&, uint8_t const*, size_type)
                {
                    ++replacement_calls;
                    EXPECT_EQ(server.clear_client_callback(again), castle::status::ok);
                }), castle::status::ok);
        }), castle::status::ok);

    server_type::client_handle handle;
    for (int i = 0; i < 3; ++i)
    {
        ASSERT_EQ(client.send_to("p", 1U, local, bytes), castle::status::ok);
        ASSERT_EQ(server.process_datagram(handle, 1000), castle::status::ok);
    }
    EXPECT_EQ(replacement_calls, 1) << "first datagram installs, second runs and clears, third finds none";
}

TEST(NetServer, CallbackCannotReenterProcessingButMayCloseServer)
{
    using server_type = tcp_server<2U, 16U>;
    server_type server;
    endpoint local;
    open_loopback(server, local);
    tcp_client<> first;
    tcp_client<> second;
    ASSERT_EQ(first.connect(local, 1000), castle::status::ok);
    ASSERT_EQ(second.connect(local, 1000), castle::status::ok);
    server_type::client_handle first_handle;
    server_type::client_handle second_handle;
    ASSERT_EQ(server.accept_client(first_handle, 1000), castle::status::ok);
    ASSERT_EQ(server.accept_client(second_handle, 1000), castle::status::ok);

    castle::status nested = castle::status::ok;
    ASSERT_EQ(server.set_client_callback(first_handle,
        [&server, &nested, second_handle](server_type::client_handle, endpoint const&, uint8_t const*, size_type)
        {
            nested = server.process_client(second_handle);
            (void)server.close();
        }), castle::status::ok);
    size_type bytes = 0U;
    ASSERT_EQ(first.send_all("a", 1U, bytes), castle::status::ok);
    ASSERT_EQ(second.send_all("b", 1U, bytes), castle::status::ok);
    EXPECT_EQ(server.process_client(first_handle, 1000), castle::status::ok);
    EXPECT_EQ(nested, castle::status::invalid_config);
    EXPECT_FALSE(server.is_open());
    EXPECT_EQ(server.client_count(), 0U);
}

TEST(NetServer, UdpCallbackKeepsMutableStateBetweenDatagrams)
{
    using server_type = udp_server<2U, 16U>;
    server_type server;
    int last_seen = 0;
    ASSERT_EQ(server.set_default_callback(
        [&last_seen, count = 0](server_type::client_handle, endpoint const&, uint8_t const*, size_type) mutable
        {
            last_seen = ++count;
        }), castle::status::ok);
    endpoint local;
    open_loopback(server, local);
    udp_client<> client;
    ASSERT_EQ(client.open(), castle::status::ok);
    size_type bytes = 0U;
    server_type::client_handle handle;
    for (int i = 1; i <= 3; ++i)
    {
        ASSERT_EQ(client.send_to("d", 1U, local, bytes), castle::status::ok);
        ASSERT_EQ(server.process_datagram(handle, 1000), castle::status::ok);
        EXPECT_EQ(last_seen, i);
    }
}

TEST(NetServer, UdpPeerTableOverflowReportsSourceAndRecoversByEviction)
{
    using server_type = udp_server<1U, 16U>;
    server_type server;
    size_type calls = 0U;
    ASSERT_EQ(server.set_default_callback(
        [&calls](server_type::client_handle, endpoint const&, uint8_t const*, size_type) { ++calls; }),
        castle::status::ok);
    endpoint local;
    open_loopback(server, local);

    udp_client<> first;
    udp_client<> second;
    ASSERT_EQ(first.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(second.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    endpoint second_address;
    ASSERT_EQ(second.socket().local_endpoint(second_address), castle::status::ok);
    size_type bytes = 0U;

    server_type::client_handle handle;
    ASSERT_EQ(first.send_to("1", 2U, local, bytes), castle::status::ok);
    ASSERT_EQ(server.process_datagram(handle, 1000), castle::status::ok);
    ASSERT_EQ(calls, 1U);

    endpoint rejected;
    ASSERT_EQ(second.send_to("2", 2U, local, bytes), castle::status::ok);
    EXPECT_EQ(server.process_datagram(handle, 1000, nullptr, &rejected), castle::status::full);
    EXPECT_FALSE(handle.is_valid());
    EXPECT_EQ(rejected, second_address);
    EXPECT_EQ(calls, 1U) << "no callback for a rejected peer";

    const server_type::client_handle victim = server.client_at(0U);
    ASSERT_TRUE(victim.is_valid());
    EXPECT_FALSE(server.client_at(1U).is_valid());
    EXPECT_EQ(server.close_client(victim), castle::status::ok);
    EXPECT_FALSE(server.client_at(0U).is_valid());

    ASSERT_EQ(second.send_to("2", 2U, local, bytes), castle::status::ok);
    ASSERT_EQ(server.process_datagram(handle, 1000, nullptr, &rejected), castle::status::ok);
    EXPECT_EQ(rejected, second_address);
    EXPECT_EQ(calls, 2U);
    EXPECT_NE(handle, victim);
    EXPECT_EQ(server.client_at(0U), handle);
}

TEST(NetServer, UdpOversizedDatagramIsDroppedWithoutRegisteringAPeer)
{
    using server_type = udp_server<2U, 4U>;
    server_type server;
    size_type calls = 0U;
    ASSERT_EQ(server.set_default_callback(
        [&calls](server_type::client_handle, endpoint const&, uint8_t const*, size_type) { ++calls; }),
        castle::status::ok);
    endpoint local;
    open_loopback(server, local);
    udp_client<> client;
    ASSERT_EQ(client.open(), castle::status::ok);
    size_type bytes = 0U;
    ASSERT_EQ(client.send_to("0123456789", 10U, local, bytes), castle::status::ok);
    server_type::client_handle handle;
    EXPECT_EQ(server.process_datagram(handle, 1000), castle::status::data_loss);
    EXPECT_FALSE(handle.is_valid());
    EXPECT_EQ(calls, 0U);
    EXPECT_EQ(server.client_count(), 0U);
}

TEST(NetServer, UdpServersDoNotSilentlyShareAPort)
{
    udp_server<1U> first;
    endpoint local;
    open_loopback(first, local);
    udp_server<1U> second;
    int system_error = 0;
    EXPECT_EQ(second.open(local, 8, true, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, EADDRINUSE);
    EXPECT_FALSE(second.is_open());
    EXPECT_EQ(first.open(local), castle::status::invalid_config);
}

TEST(NetServer, AcceptOnFullServerReportsFullInsteadOfWaiting)
{
    tcp_server<1U> server;
    endpoint local;
    open_loopback(server, local);
    tcp_client<> first;
    tcp_client<> second;
    ASSERT_EQ(first.connect(local, 1000), castle::status::ok);
    ASSERT_EQ(second.connect(local, 1000), castle::status::ok);
    tcp_server<1U>::client_handle handle;
    ASSERT_EQ(server.accept_client(handle, 1000), castle::status::ok);

    const auto started = std::chrono::steady_clock::now();
    tcp_server<1U>::client_handle rejected;
    EXPECT_EQ(server.accept_client(rejected, 5000), castle::status::full);
    EXPECT_LT(std::chrono::steady_clock::now() - started, std::chrono::seconds(2));
    EXPECT_EQ(server.close_client(handle), castle::status::ok);
    EXPECT_EQ(server.accept_client(rejected, 1000), castle::status::ok) << "the queued connection is still accepted";
}

TEST(NetServer, ClientNativeHandleAndSlotIterationSupportExternalPolling)
{
    using server_type = tcp_server<2U>;
    server_type server;
    endpoint local;
    open_loopback(server, local);
    tcp_client<> first;
    tcp_client<> second;
    ASSERT_EQ(first.connect(local, 1000), castle::status::ok);
    ASSERT_EQ(second.connect(local, 1000), castle::status::ok);
    server_type::client_handle a;
    server_type::client_handle b;
    ASSERT_EQ(server.accept_client(a, 1000), castle::status::ok);
    ASSERT_EQ(server.accept_client(b, 1000), castle::status::ok);

    EXPECT_GE(server.client_native_handle(a), 0);
    EXPECT_NE(server.client_native_handle(a), server.client_native_handle(b));
    EXPECT_NE(server.client_native_handle(a), server.native_handle());
    EXPECT_EQ(server.client_at(0U), a);
    EXPECT_EQ(server.client_at(1U), b);
    EXPECT_FALSE(server.client_at(2U).is_valid());

    // One poll over both client descriptors replaces N sequential timeouts.
    castle::io::fd::fd_event_manager manager;
    int ready_fd = -1;
    for (const auto handle : {a, b})
    {
        ASSERT_EQ(manager.add_fd(server.client_native_handle(handle), castle::io::fd::poll_read,
                                 [&ready_fd](int fd, short, void*) { ready_fd = fd; }), castle::status::ok);
    }
    size_type bytes = 0U;
    ASSERT_EQ(second.send_all("x", 1U, bytes), castle::status::ok);
    size_type dispatched = 0U;
    ASSERT_EQ(manager.wait_and_process(1000, dispatched), castle::status::ok);
    EXPECT_EQ(dispatched, 1U);
    EXPECT_EQ(ready_fd, server.client_native_handle(b));

    EXPECT_EQ(server.close_client(a), castle::status::ok);
    EXPECT_EQ(server.client_native_handle(a), -1);
}

TEST(NetServer, UdpClientNativeHandleIsTheSharedSocket)
{
    udp_server<2U> server;
    endpoint local;
    open_loopback(server, local);
    udp_client<> client;
    ASSERT_EQ(client.open(), castle::status::ok);
    size_type bytes = 0U;
    ASSERT_EQ(client.send_to("p", 1U, local, bytes), castle::status::ok);
    udp_server<2U>::client_handle handle;
    ASSERT_EQ(server.process_datagram(handle, 1000), castle::status::ok);
    EXPECT_EQ(server.client_native_handle(handle), server.native_handle());
    EXPECT_EQ(server.client_native_handle(udp_server<2U>::client_handle()), -1);
}

TEST(NetServer, StaleHandleStaysInvalidAfterSlotIsReused)
{
    using server_type = tcp_server<1U>;
    server_type server;
    endpoint local;
    open_loopback(server, local);
    tcp_client<> first;
    ASSERT_EQ(first.connect(local, 1000), castle::status::ok);
    server_type::client_handle old_handle;
    ASSERT_EQ(server.accept_client(old_handle, 1000), castle::status::ok);
    ASSERT_EQ(server.close_client(old_handle), castle::status::ok);

    tcp_client<> second;
    ASSERT_EQ(second.connect(local, 1000), castle::status::ok);
    server_type::client_handle new_handle;
    ASSERT_EQ(server.accept_client(new_handle, 1000), castle::status::ok);
    EXPECT_EQ(new_handle.index, old_handle.index);
    EXPECT_NE(new_handle.generation, old_handle.generation);
    size_type bytes = 0U;
    EXPECT_EQ(server.send_some(old_handle, "x", 1U, bytes), castle::status::not_found);
    EXPECT_EQ(server.process_client(old_handle), castle::status::not_found);
    EXPECT_EQ(server.set_client_callback(old_handle,
        [](server_type::client_handle, endpoint const&, uint8_t const*, size_type) {}), castle::status::not_found);
    EXPECT_EQ(server.send_some(new_handle, "x", 1U, bytes), castle::status::ok);
}

TEST(NetServer, Ipv6TcpServerAcceptsLoopbackClient)
{
    tcp_server<1U> server;
    if (!castle::succeeded(server.open(endpoint::loopback_ipv6(0U))))
    {
        GTEST_SKIP() << "IPv6 loopback is unavailable";
    }
    endpoint local;
    ASSERT_EQ(server.socket().local_endpoint(local), castle::status::ok);
    EXPECT_EQ(local.family(), address_family::ipv6);
    tcp_client<> client;
    ASSERT_EQ(client.connect(local, 1000), castle::status::ok);
    tcp_server<1U>::client_handle handle;
    ASSERT_EQ(server.accept_client(handle, 1000), castle::status::ok);
    endpoint remote;
    ASSERT_EQ(server.client_endpoint(handle, remote), castle::status::ok);
    EXPECT_EQ(remote.family(), address_family::ipv6);
}

TEST(NetServer, FailedOpenLeavesServerClosedAndReusable)
{
    tcp_server<1U> blocker;
    endpoint local;
    open_loopback(blocker, local);
    tcp_server<1U> server;
    EXPECT_EQ(server.open(local), castle::status::system_call_error);
    EXPECT_FALSE(server.is_open());
    EXPECT_EQ(server.open(endpoint::loopback_ipv4(0U)), castle::status::ok);
}

#else

TEST(NetServer, UnsupportedTargetKeepsApiAvailable)
{
    tcp_server<2U> server;
    EXPECT_EQ(server.open(endpoint::loopback_ipv4(0U)), castle::status::not_configured);
    tcp_server<2U>::client_handle handle;
    EXPECT_EQ(server.accept_client(handle), castle::status::not_configured);
}

#endif

} // namespace
