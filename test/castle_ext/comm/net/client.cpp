#include <gtest/gtest.h>

#include "castle_ext/comm/net/client.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <errno.h>
    #include <string.h>
    #include <sys/socket.h>
#endif

namespace
{

using namespace castle::comm::net;
using castle::size_type;

#if CASTLE_USING_POSIX_APIS

template <size_type ReceiveBufferSize>
castle::status connect_client(tcp_client<ReceiveBufferSize>& client, endpoint const& peer)
{
    return client.connect(peer, 1000);
}

TEST(NetClient, TcpClientConnectSendAndCallbackReceive)
{
    tcp_socket listener;
    ASSERT_EQ(listener.open(), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(2), castle::status::ok);
    endpoint local;
    ASSERT_EQ(listener.local_endpoint(local), castle::status::ok);

    tcp_client<64U> client;
    size_type callbacks = 0U;
    size_type callback_size = 0U;
    endpoint callback_peer;
    ASSERT_EQ(client.set_receive_callback(
        [&callbacks, &callback_size, &callback_peer](endpoint const& peer, uint8_t const* data, size_type size)
        {
            ++callbacks;
            callback_size = size;
            callback_peer = peer;
            EXPECT_GT(size, 0U);
            EXPECT_EQ(data[0], static_cast<uint8_t>('r'));
        }), castle::status::ok);
    ASSERT_EQ(connect_client(client, local), castle::status::ok);

    tcp_socket peer;
    endpoint peer_address;
    ASSERT_EQ(listener.accept(peer, peer_address), castle::status::ok);
    const char response[] = "reply";
    size_type bytes = 0U;
    ASSERT_EQ(peer.send_all(response, sizeof(response), bytes), castle::status::ok);
    ASSERT_EQ(client.process_once(1000), castle::status::ok);
    EXPECT_EQ(callbacks, 1U);
    EXPECT_EQ(callback_size, sizeof(response));
    EXPECT_EQ(callback_peer.port(), local.port());

    const char request[] = "query";
    ASSERT_EQ(client.send_all(request, sizeof(request), bytes), castle::status::ok);
    short ready = 0;
    ASSERT_EQ(peer.wait(castle::io::fd::poll_read, 1000, ready), castle::status::ok);
    char buffer[32] = {};
    ASSERT_EQ(peer.receive_some(buffer, sizeof(buffer), bytes), castle::status::ok);
    EXPECT_STREQ(buffer, request);

    client.clear_receive_callback();
    ASSERT_EQ(peer.send_all(response, sizeof(response), bytes), castle::status::ok);
    EXPECT_EQ(client.process_once(1000), castle::status::ok);
    EXPECT_EQ(callbacks, 1U);

    EXPECT_EQ(client.close(), castle::status::ok);
    EXPECT_EQ(client.close(), castle::status::not_configured);
}

TEST(NetClient, TcpPeerEofClosesClient)
{
    tcp_socket listener;
    ASSERT_EQ(listener.open(), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(1), castle::status::ok);
    endpoint local;
    ASSERT_EQ(listener.local_endpoint(local), castle::status::ok);
    tcp_client<> client;
    ASSERT_EQ(client.connect(local, 1000), castle::status::ok);
    tcp_socket peer;
    endpoint remote;
    ASSERT_EQ(listener.accept(peer, remote), castle::status::ok);
    ASSERT_EQ(peer.shutdown(shutdown_mode::send), castle::status::ok);
    EXPECT_EQ(client.process_once(1000), castle::status::not_configured);
    EXPECT_FALSE(client.is_open());
}

TEST(NetClient, UdpClientReceivesCallbackWithSourceAndSupportsSendTo)
{
    udp_socket server;
    ASSERT_EQ(server.open(), castle::status::ok);
    ASSERT_EQ(server.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    endpoint local;
    ASSERT_EQ(server.local_endpoint(local), castle::status::ok);

    udp_client<32U> client;
    size_type callback_count = 0U;
    size_type callback_size = 0U;
    ASSERT_EQ(client.open(), castle::status::ok);
    ASSERT_EQ(client.set_receive_callback(
        [&callback_count, &callback_size](endpoint const& source, uint8_t const* data, size_type size)
        {
            if (source.port() != 0U) { ++callback_count; }
            callback_size = size;
            EXPECT_EQ(data[0], static_cast<uint8_t>('a'));
        }), castle::status::ok);

    const char outgoing[] = "out";
    size_type bytes = 0U;
    ASSERT_EQ(client.send_to(outgoing, sizeof(outgoing), local, bytes), castle::status::ok);
    short ready = 0;
    ASSERT_EQ(server.wait(castle::io::fd::poll_read, 1000, ready), castle::status::ok);
    char buffer[16] = {};
    endpoint source;
    ASSERT_EQ(server.receive_from(buffer, sizeof(buffer), bytes, source), castle::status::ok);
    EXPECT_STREQ(buffer, outgoing);
    const char response[] = "ack";
    ASSERT_EQ(server.send_to(response, sizeof(response), source, bytes), castle::status::ok);
    ASSERT_EQ(client.process_once(1000), castle::status::ok);
    EXPECT_EQ(callback_count, 1U);
    EXPECT_EQ(callback_size, sizeof(response));
}

TEST(NetClient, OpenAndOperationStatusAreForwarded)
{
    tcp_client<> client;
    EXPECT_FALSE(client.is_open());
    EXPECT_EQ(client.process_once(), castle::status::not_configured);
    EXPECT_EQ(client.open(address_family::ipv4), castle::status::ok);
    EXPECT_EQ(client.open(address_family::ipv4), castle::status::invalid_config);
    size_type bytes = 99U;
    EXPECT_EQ(client.send_to("x", 1U, endpoint::loopback_ipv4(1U), bytes), castle::status::invalid_config);
    EXPECT_EQ(client.close(), castle::status::ok);

    udp_client<> datagram_client;
    ASSERT_EQ(datagram_client.open(), castle::status::ok);
    EXPECT_EQ(datagram_client.process_once(0), castle::status::time_out);
    EXPECT_EQ(datagram_client.close(), castle::status::ok);
}

TEST(NetClient, UdpDatagramLargerThanBufferIsDroppedAsDataLoss)
{
    udp_socket server;
    ASSERT_EQ(server.open(), castle::status::ok);
    ASSERT_EQ(server.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);

    udp_client<4U> client;
    ASSERT_EQ(client.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    endpoint client_address;
    ASSERT_EQ(client.socket().local_endpoint(client_address), castle::status::ok);
    size_type calls = 0U;
    ASSERT_EQ(client.set_receive_callback([&calls](endpoint const&, uint8_t const*, size_type) { ++calls; }),
              castle::status::ok);
    size_type bytes = 0U;
    ASSERT_EQ(server.send_to("0123456789", 10U, client_address, bytes), castle::status::ok);
    EXPECT_EQ(client.process_once(1000), castle::status::data_loss);
    EXPECT_EQ(calls, 0U);
    EXPECT_EQ(client.process_once(0), castle::status::time_out);
}

TEST(NetClient, CallbackKeepsMutableStateBetweenDatagrams)
{
    udp_socket server;
    ASSERT_EQ(server.open(), castle::status::ok);
    udp_client<16U> client;
    ASSERT_EQ(client.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    endpoint client_address;
    ASSERT_EQ(client.socket().local_endpoint(client_address), castle::status::ok);

    int last_seen = 0;
    ASSERT_EQ(client.set_receive_callback(
        [&last_seen, count = 0](endpoint const&, uint8_t const*, size_type) mutable { last_seen = ++count; }),
        castle::status::ok);
    size_type bytes = 0U;
    for (int i = 1; i <= 3; ++i)
    {
        ASSERT_EQ(server.send_to("d", 1U, client_address, bytes), castle::status::ok);
        ASSERT_EQ(client.process_once(1000), castle::status::ok);
        EXPECT_EQ(last_seen, i);
    }
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

TEST(NetClient, CallbackMayClearAndReplaceItselfWhileRunning)
{
    udp_socket server;
    ASSERT_EQ(server.open(), castle::status::ok);
    udp_client<16U> client;
    ASSERT_EQ(client.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    endpoint client_address;
    ASSERT_EQ(client.socket().local_endpoint(client_address), castle::status::ok);
    size_type bytes = 0U;

    int live = 0;
    int live_inside_after_clear = -1;
    int replacement_calls = 0;
    {
        live_capture capture(&live);
        ASSERT_EQ(client.set_receive_callback(
            [capture, &client, &live_inside_after_clear](endpoint const&, uint8_t const*, size_type)
            {
                client.clear_receive_callback();
                live_inside_after_clear = *capture.live;
            }), castle::status::ok);
    }
    EXPECT_EQ(live, 1);
    ASSERT_EQ(server.send_to("a", 1U, client_address, bytes), castle::status::ok);
    ASSERT_EQ(client.process_once(1000), castle::status::ok);
    EXPECT_EQ(live_inside_after_clear, 1) << "the running callable must outlive its own clear";
    EXPECT_EQ(live, 0) << "the cleared callable is destroyed once it returns";

    ASSERT_EQ(client.set_receive_callback(
        [&client, &replacement_calls](endpoint const&, uint8_t const*, size_type)
        {
            EXPECT_EQ(client.set_receive_callback(
                [&replacement_calls](endpoint const&, uint8_t const*, size_type) { replacement_calls += 10; }),
                castle::status::ok);
            replacement_calls += 1;
        }), castle::status::ok);
    for (int i = 0; i < 2; ++i)
    {
        ASSERT_EQ(server.send_to("b", 1U, client_address, bytes), castle::status::ok);
        ASSERT_EQ(client.process_once(1000), castle::status::ok);
    }
    EXPECT_EQ(replacement_calls, 11);
}

TEST(NetClient, CallbackCannotReenterProcessOnceButMayClose)
{
    udp_socket server;
    ASSERT_EQ(server.open(), castle::status::ok);
    udp_client<16U> client;
    ASSERT_EQ(client.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    endpoint client_address;
    ASSERT_EQ(client.socket().local_endpoint(client_address), castle::status::ok);

    castle::status nested = castle::status::ok;
    ASSERT_EQ(client.set_receive_callback(
        [&client, &nested](endpoint const&, uint8_t const*, size_type)
        {
            nested = client.process_once(0);
            (void)client.close();
        }), castle::status::ok);
    size_type bytes = 0U;
    ASSERT_EQ(server.send_to("c", 1U, client_address, bytes), castle::status::ok);
    EXPECT_EQ(client.process_once(1000), castle::status::ok);
    EXPECT_EQ(nested, castle::status::invalid_config);
    EXPECT_FALSE(client.is_open());
}

TEST(NetClient, NullFunctionPointerCallbackIsRejected)
{
    tcp_client<> client;
    void (*none)(endpoint const&, uint8_t const*, size_type) = nullptr;
    EXPECT_EQ(client.set_receive_callback(none), castle::status::invalid_callback);
}

TEST(NetClient, TcpDataIsDeliveredEvenWhenPeerResetsRightAfterSending)
{
    tcp_socket listener;
    ASSERT_EQ(listener.open(), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(1), castle::status::ok);
    endpoint local;
    ASSERT_EQ(listener.local_endpoint(local), castle::status::ok);

    tcp_client<16U> client;
    char received[8] = {};
    endpoint reported_peer;
    ASSERT_EQ(client.set_receive_callback(
        [&received, &reported_peer](endpoint const& peer, uint8_t const* data, size_type size)
        {
            reported_peer = peer;
            ::memcpy(received, data, size < sizeof(received) ? size : sizeof(received));
        }), castle::status::ok);
    ASSERT_EQ(client.connect(local, 1000), castle::status::ok);
    ASSERT_EQ(listener.wait_readable(1000), castle::status::ok);
    {
        tcp_socket peer;
        endpoint remote;
        ASSERT_EQ(listener.accept(peer, remote), castle::status::ok);
        size_type bytes = 0U;
        ASSERT_EQ(peer.send_all("bye", 4U, bytes), castle::status::ok);
        struct linger abortive = {1, 0};
        ASSERT_EQ(::setsockopt(peer.native_handle(), SOL_SOCKET, SO_LINGER, &abortive, sizeof(abortive)), 0);
    } // closing with a zero linger sends RST right behind the data
    EXPECT_EQ(client.process_once(1000), castle::status::ok);
    EXPECT_STREQ(received, "bye");
    EXPECT_EQ(reported_peer, local);
}

TEST(NetClient, FailedConnectClosesItsSocketSoRetryCanChooseAnyFamily)
{
    endpoint refused;
    {
        tcp_socket probe;
        ASSERT_EQ(probe.open(), castle::status::ok);
        ASSERT_EQ(probe.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
        ASSERT_EQ(probe.local_endpoint(refused), castle::status::ok);
    }
    tcp_socket listener;
    ASSERT_EQ(listener.open(), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(1), castle::status::ok);
    endpoint reachable;
    ASSERT_EQ(listener.local_endpoint(reachable), castle::status::ok);

    tcp_client<> client;
    int system_error = 0;
    EXPECT_EQ(client.connect(refused, 1000, true, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, ECONNREFUSED);
    EXPECT_FALSE(client.is_open());
    EXPECT_EQ(client.connect(reachable, 1000), castle::status::ok);
    EXPECT_TRUE(client.is_open());
}

TEST(NetClient, ConnectWithoutWaitLeavesPendingSocketOpenAndFinishesLater)
{
    tcp_socket listener;
    ASSERT_EQ(listener.open(), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(1), castle::status::ok);
    endpoint local;
    ASSERT_EQ(listener.local_endpoint(local), castle::status::ok);

    tcp_client<> client;
    const castle::status result = client.connect(local); // default: do not wait
    EXPECT_TRUE(result == castle::status::ok || result == castle::status::time_out);
    EXPECT_TRUE(client.is_open());
    if (result == castle::status::time_out)
    {
        EXPECT_EQ(client.socket().finish_connect(1000), castle::status::ok);
    }
}

TEST(NetClient, MoveTransfersSocketPeerAndCallback)
{
    tcp_socket listener;
    ASSERT_EQ(listener.open(), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(1), castle::status::ok);
    endpoint local;
    ASSERT_EQ(listener.local_endpoint(local), castle::status::ok);

    tcp_client<16U> original;
    size_type calls = 0U;
    endpoint reported_peer;
    ASSERT_EQ(original.set_receive_callback(
        [&calls, &reported_peer](endpoint const& peer, uint8_t const*, size_type) { ++calls; reported_peer = peer; }),
        castle::status::ok);
    ASSERT_EQ(original.connect(local, 1000), castle::status::ok);

    tcp_client<16U> moved(castle::move(original));
    EXPECT_FALSE(original.is_open());
    EXPECT_TRUE(moved.is_open());

    ASSERT_EQ(listener.wait_readable(1000), castle::status::ok);
    tcp_socket peer;
    endpoint remote;
    ASSERT_EQ(listener.accept(peer, remote), castle::status::ok);
    size_type bytes = 0U;
    ASSERT_EQ(peer.send_all("m", 1U, bytes), castle::status::ok);
    EXPECT_EQ(moved.process_once(1000), castle::status::ok);
    EXPECT_EQ(calls, 1U);
    EXPECT_EQ(reported_peer, local);

    tcp_client<16U> assigned;
    assigned = castle::move(moved);
    EXPECT_FALSE(moved.is_open());
    EXPECT_TRUE(assigned.is_open());
    EXPECT_EQ(assigned.close(), castle::status::ok);
}

#else

TEST(NetClient, UnsupportedTargetKeepsApiAvailable)
{
    tcp_client<> client;
    EXPECT_EQ(client.open(), castle::status::not_configured);
    EXPECT_EQ(client.process_once(), castle::status::not_configured);
}

#endif

} // namespace
