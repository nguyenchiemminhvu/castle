#include <gtest/gtest.h>

#include "castle_ext/comm/net/socket.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <errno.h>
    #include <fcntl.h>
    #include <netinet/in.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

namespace
{

using namespace castle::comm::net;
using castle::size_type;

#if CASTLE_USING_POSIX_APIS

castle::status complete_connect(tcp_socket& client, endpoint const& remote)
{
    castle::status result = client.connect(remote);
    if (result == castle::status::time_out)
    {
        result = client.finish_connect(1000);
    }
    return result;
}

TEST(NetSocket, CreatesTypedSocketsAndRejectsDuplicateOpen)
{
    tcp_socket tcp;
    udp_socket udp;
    EXPECT_FALSE(tcp.is_open());
    EXPECT_FALSE(udp.is_open());
    EXPECT_EQ(tcp.open(), castle::status::ok);
    EXPECT_EQ(tcp.open(), castle::status::invalid_config);
    EXPECT_TRUE(tcp.is_open());
    EXPECT_TRUE(tcp.is_nonblocking());
    EXPECT_EQ(tcp.close(), castle::status::ok);
    EXPECT_EQ(udp.open(address_family::ipv4, false), castle::status::ok);
    EXPECT_FALSE(udp.is_nonblocking());
    EXPECT_EQ(udp.close(), castle::status::ok);
}

TEST(NetSocket, ClosedOperationsReturnNotConfiguredAndClearOutputs)
{
    tcp_socket socket;
    size_type transferred = 42U;
    int system_error = 99;
    EXPECT_EQ(socket.close(&system_error), castle::status::not_configured);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(socket.send_some("x", 1U, transferred, &system_error), castle::status::not_configured);
    EXPECT_EQ(transferred, 0U);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(socket.receive_some(nullptr, 0U, transferred, &system_error), castle::status::not_configured);
    EXPECT_EQ(transferred, 0U);
    EXPECT_EQ(socket.bind(endpoint::loopback_ipv4(0U)), castle::status::not_configured);
    short events = 99;
    EXPECT_EQ(socket.wait(castle::io::fd::poll_read, 0, events), castle::status::not_configured);
    EXPECT_EQ(events, 0);
    endpoint local;
    EXPECT_EQ(socket.local_endpoint(local), castle::status::not_configured);
}

TEST(NetSocket, RejectsInvalidBuffersAndProtocolSpecificOperations)
{
    tcp_socket tcp;
    udp_socket udp;
    ASSERT_EQ(tcp.open(), castle::status::ok);
    ASSERT_EQ(udp.open(), castle::status::ok);
    size_type transferred = 0U;
    endpoint remote;
    EXPECT_EQ(tcp.send_some(nullptr, 1U, transferred), castle::status::invalid_argument);
    EXPECT_EQ(tcp.receive_some(nullptr, 1U, transferred), castle::status::invalid_argument);
    EXPECT_EQ(tcp.send_to("x", 1U, endpoint::loopback_ipv4(1U), transferred), castle::status::invalid_config);
    EXPECT_EQ(tcp.listen(1), castle::status::ok); // Empty TCP sockets may listen before bind; OS accepts or rejects.
    EXPECT_EQ(tcp.set_no_delay(), castle::status::ok);
    EXPECT_EQ(tcp.set_keep_alive(), castle::status::ok);
    EXPECT_EQ(udp.set_no_delay(), castle::status::invalid_config);
    EXPECT_EQ(udp.set_keep_alive(), castle::status::invalid_config);
    EXPECT_EQ(udp.listen(), castle::status::invalid_config);
    EXPECT_EQ(udp.finish_connect(0), castle::status::invalid_config);
    EXPECT_EQ(udp.accept(tcp, remote), castle::status::invalid_config);
    EXPECT_EQ(tcp.close(), castle::status::ok);
    EXPECT_EQ(udp.close(), castle::status::ok);
}

TEST(NetSocket, TcpListenConnectAcceptAndExchange)
{
    tcp_socket listener;
    ASSERT_EQ(listener.open(), castle::status::ok);
    ASSERT_EQ(listener.set_reuse_address(), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(4), castle::status::ok);
    endpoint local;
    ASSERT_EQ(listener.local_endpoint(local), castle::status::ok);
    ASSERT_NE(local.port(), 0U);

    tcp_socket client;
    ASSERT_EQ(client.open(), castle::status::ok);
    ASSERT_EQ(complete_connect(client, local), castle::status::ok);
    short ready = 0;
    ASSERT_EQ(listener.wait(castle::io::fd::poll_read, 1000, ready), castle::status::ok);
    EXPECT_NE(ready & castle::io::fd::poll_read, 0);

    tcp_socket accepted;
    endpoint peer;
    ASSERT_EQ(listener.accept(accepted, peer), castle::status::ok);
    EXPECT_TRUE(accepted.is_open());
    EXPECT_TRUE(accepted.is_nonblocking());
    EXPECT_NE(peer.port(), 0U);
    endpoint accepted_local;
    ASSERT_EQ(accepted.local_endpoint(accepted_local), castle::status::ok);
    EXPECT_EQ(accepted_local.port(), local.port());
    endpoint client_peer;
    ASSERT_EQ(client.peer_endpoint(client_peer), castle::status::ok);
    EXPECT_EQ(client_peer.port(), local.port());

    const char request[] = "socket-test";
    size_type bytes = 0U;
    ASSERT_EQ(client.send_all(request, sizeof(request), bytes), castle::status::ok);
    EXPECT_EQ(bytes, sizeof(request));
    ASSERT_EQ(accepted.wait(castle::io::fd::poll_read, 1000, ready), castle::status::ok);
    char buffer[32] = {};
    ASSERT_EQ(accepted.receive_some(buffer, sizeof(buffer), bytes), castle::status::ok);
    EXPECT_EQ(bytes, sizeof(request));
    EXPECT_STREQ(buffer, request);

    EXPECT_EQ(listener.accept(accepted, peer), castle::status::invalid_config); // output is already open
    EXPECT_EQ(client.set_nonblocking(false), castle::status::ok);
    EXPECT_FALSE(client.is_nonblocking());
    EXPECT_EQ(client.set_nonblocking(true), castle::status::ok);
    EXPECT_TRUE(client.is_nonblocking());
    EXPECT_EQ(client.shutdown(shutdown_mode::send), castle::status::ok);

    EXPECT_EQ(accepted.close(), castle::status::ok);
    EXPECT_EQ(client.close(), castle::status::ok);
    EXPECT_EQ(listener.close(), castle::status::ok);
}

TEST(NetSocket, TcpReceiveDistinguishesWouldBlockFromPeerClose)
{
    tcp_socket listener;
    ASSERT_EQ(listener.open(), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(1), castle::status::ok);
    endpoint local;
    ASSERT_EQ(listener.local_endpoint(local), castle::status::ok);

    tcp_socket client;
    ASSERT_EQ(client.open(), castle::status::ok);
    ASSERT_EQ(complete_connect(client, local), castle::status::ok);
    tcp_socket accepted;
    endpoint remote;
    ASSERT_EQ(listener.accept(accepted, remote), castle::status::ok);

    char buffer[8] = {};
    size_type bytes = 99U;
    bool peer_closed = true;
    EXPECT_EQ(accepted.receive_some(buffer, sizeof(buffer), bytes, nullptr, &peer_closed),
              castle::status::empty);
    EXPECT_EQ(bytes, 0U);
    EXPECT_FALSE(peer_closed);

    ASSERT_EQ(client.shutdown(shutdown_mode::send), castle::status::ok);
    short ready = 0;
    ASSERT_EQ(accepted.wait(castle::io::fd::poll_read, 1000, ready), castle::status::ok);
    EXPECT_EQ(accepted.receive_some(buffer, sizeof(buffer), bytes, nullptr, &peer_closed),
              castle::status::empty);
    EXPECT_EQ(bytes, 0U);
    EXPECT_TRUE(peer_closed);
}

TEST(NetSocket, NonblockingTcpAcceptAndWaitReportEmptyAndTimeout)
{
    tcp_socket listener;
    ASSERT_EQ(listener.open(), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(1), castle::status::ok);
    tcp_socket accepted;
    endpoint peer;
    EXPECT_EQ(listener.accept(accepted, peer), castle::status::empty);
    short ready = 0;
    EXPECT_EQ(listener.wait(castle::io::fd::poll_read, 0, ready), castle::status::time_out);
    EXPECT_EQ(ready, 0);
}

TEST(NetSocket, UdpDatagramsPreserveSourceEndpointAndZeroLengthPackets)
{
    udp_socket server;
    udp_socket client;
    ASSERT_EQ(server.open(), castle::status::ok);
    ASSERT_EQ(client.open(), castle::status::ok);
    ASSERT_EQ(server.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    endpoint local;
    ASSERT_EQ(server.local_endpoint(local), castle::status::ok);

    const char packet[] = "datagram";
    size_type bytes = 0U;
    ASSERT_EQ(client.send_to(packet, sizeof(packet), local, bytes), castle::status::ok);
    EXPECT_EQ(bytes, sizeof(packet));
    short ready = 0;
    ASSERT_EQ(server.wait(castle::io::fd::poll_read, 1000, ready), castle::status::ok);
    char buffer[32] = {};
    endpoint source;
    ASSERT_EQ(server.receive_from(buffer, sizeof(buffer), bytes, source), castle::status::ok);
    EXPECT_EQ(bytes, sizeof(packet));
    EXPECT_STREQ(buffer, packet);
    EXPECT_NE(source.port(), 0U);

    ASSERT_EQ(client.send_to(nullptr, 0U, local, bytes), castle::status::ok);
    ASSERT_EQ(server.wait(castle::io::fd::poll_read, 1000, ready), castle::status::ok);
    ASSERT_EQ(server.receive_from(buffer, sizeof(buffer), bytes, source), castle::status::ok);
    EXPECT_EQ(bytes, 0U);

    udp_socket connected_client;
    ASSERT_EQ(connected_client.open(), castle::status::ok);
    ASSERT_EQ(connected_client.connect(local), castle::status::ok);
    ASSERT_EQ(connected_client.send_some(nullptr, 0U, bytes), castle::status::ok);
    ASSERT_EQ(server.wait(castle::io::fd::poll_read, 1000, ready), castle::status::ok);
    ASSERT_EQ(server.receive_from(buffer, sizeof(buffer), bytes, source), castle::status::ok);
    EXPECT_EQ(bytes, 0U);
    EXPECT_EQ(connected_client.close(), castle::status::ok);

    EXPECT_EQ(server.send_to(nullptr, 1U, source, bytes), castle::status::invalid_argument);
    EXPECT_EQ(client.close(), castle::status::ok);
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(NetSocket, UdpOversizedDatagramIsReportedAsDataLoss)
{
    udp_socket server;
    udp_socket client;
    ASSERT_EQ(server.open(), castle::status::ok);
    ASSERT_EQ(client.open(), castle::status::ok);
    ASSERT_EQ(server.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    endpoint local;
    ASSERT_EQ(server.local_endpoint(local), castle::status::ok);
    const uint8_t payload[8] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U};
    size_type bytes = 0U;
    ASSERT_EQ(client.send_to(payload, sizeof(payload), local, bytes), castle::status::ok);
    short ready = 0;
    ASSERT_EQ(server.wait(castle::io::fd::poll_read, 1000, ready), castle::status::ok);
    uint8_t small[2] = {};
    endpoint source;
    EXPECT_EQ(server.receive_from(small, sizeof(small), bytes, source), castle::status::data_loss);
    EXPECT_EQ(bytes, sizeof(small));
    EXPECT_EQ(small[0], 0U);
    EXPECT_EQ(small[1], 1U);
}

TEST(NetSocket, FamilyMismatchIsRejected)
{
    tcp_socket socket;
    ASSERT_EQ(socket.open(address_family::ipv4), castle::status::ok);
    EXPECT_EQ(socket.bind(endpoint::loopback_ipv6(0U)), castle::status::invalid_argument);
    EXPECT_EQ(socket.connect(endpoint::loopback_ipv6(1U)), castle::status::invalid_argument);
}

void connect_pair(tcp_socket& listener, tcp_socket& client, tcp_socket& accepted)
{
    ASSERT_EQ(listener.open(), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(2), castle::status::ok);
    endpoint local;
    ASSERT_EQ(listener.local_endpoint(local), castle::status::ok);
    ASSERT_EQ(client.open(), castle::status::ok);
    ASSERT_EQ(complete_connect(client, local), castle::status::ok);
    ASSERT_EQ(listener.wait_readable(1000), castle::status::ok);
    endpoint remote;
    ASSERT_EQ(listener.accept(accepted, remote), castle::status::ok);
}

bool is_close_on_exec(int descriptor)
{
    return (::fcntl(descriptor, F_GETFD) & FD_CLOEXEC) != 0;
}

bool is_os_nonblocking(int descriptor)
{
    return (::fcntl(descriptor, F_GETFL) & O_NONBLOCK) != 0;
}

bool ipv6_loopback_available()
{
    udp_socket probe;
    return castle::succeeded(probe.open(address_family::ipv6)) &&
           castle::succeeded(probe.bind(endpoint::loopback_ipv6(0U)));
}

TEST(NetSocket, SocketsAreCloseOnExecAndHonourBlockingMode)
{
    tcp_socket blocking;
    udp_socket datagram;
    ASSERT_EQ(blocking.open(address_family::ipv4, false), castle::status::ok);
    ASSERT_EQ(datagram.open(address_family::ipv4, true), castle::status::ok);
    EXPECT_TRUE(is_close_on_exec(blocking.native_handle()));
    EXPECT_TRUE(is_close_on_exec(datagram.native_handle()));
    EXPECT_FALSE(is_os_nonblocking(blocking.native_handle()));
    EXPECT_TRUE(is_os_nonblocking(datagram.native_handle()));

    tcp_socket listener;
    tcp_socket client;
    tcp_socket accepted;
    connect_pair(listener, client, accepted);
    EXPECT_TRUE(is_close_on_exec(accepted.native_handle()));
    EXPECT_TRUE(is_os_nonblocking(accepted.native_handle()));
}

TEST(NetSocket, AcceptedSocketInheritsBlockingModeFromListener)
{
    tcp_socket listener;
    ASSERT_EQ(listener.open(address_family::ipv4, false), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(1), castle::status::ok);
    endpoint local;
    ASSERT_EQ(listener.local_endpoint(local), castle::status::ok);
    tcp_socket client;
    ASSERT_EQ(client.open(), castle::status::ok);
    ASSERT_EQ(complete_connect(client, local), castle::status::ok);

    tcp_socket accepted;
    endpoint remote;
    ASSERT_EQ(listener.accept(accepted, remote), castle::status::ok);
    EXPECT_FALSE(accepted.is_nonblocking());
    EXPECT_FALSE(is_os_nonblocking(accepted.native_handle()));
    EXPECT_TRUE(is_close_on_exec(accepted.native_handle()));
}

TEST(NetSocket, Ipv6LoopbackBindConnectAndExchange)
{
    if (!ipv6_loopback_available()) { GTEST_SKIP() << "IPv6 loopback is unavailable"; }
    tcp_socket listener;
    ASSERT_EQ(listener.open(address_family::ipv6), castle::status::ok);
    ASSERT_EQ(listener.bind(endpoint::loopback_ipv6(0U)), castle::status::ok);
    ASSERT_EQ(listener.listen(1), castle::status::ok);
    endpoint local;
    ASSERT_EQ(listener.local_endpoint(local), castle::status::ok);
    EXPECT_EQ(local.family(), address_family::ipv6);
    EXPECT_EQ(local, endpoint::loopback_ipv6(local.port()));

    tcp_socket client;
    ASSERT_EQ(client.open(address_family::ipv6), castle::status::ok);
    ASSERT_EQ(complete_connect(client, local), castle::status::ok);
    ASSERT_EQ(listener.wait_readable(1000), castle::status::ok);
    tcp_socket accepted;
    endpoint remote;
    ASSERT_EQ(listener.accept(accepted, remote), castle::status::ok);
    EXPECT_EQ(remote.family(), address_family::ipv6);
    EXPECT_EQ(remote.address_bytes()[15], 1U);

    size_type bytes = 0U;
    ASSERT_EQ(client.send_all("v6", 3U, bytes), castle::status::ok);
    ASSERT_EQ(accepted.wait_readable(1000), castle::status::ok);
    char buffer[8] = {};
    ASSERT_EQ(accepted.receive_some(buffer, sizeof(buffer), bytes), castle::status::ok);
    EXPECT_STREQ(buffer, "v6");
}

TEST(NetSocket, NativeEndpointConversionRoundTripsAddressPortAndScope)
{
    uint8_t bytes[16] = {0xFEU, 0x80U};
    bytes[15] = 0x42U;
    const endpoint original = endpoint::ipv6(bytes, 4433U, 7U);
    struct sockaddr_storage storage;
    socklen_t length = 0;
    ASSERT_EQ(detail::to_native_endpoint(original, storage, length), castle::status::ok);
    ASSERT_EQ(length, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    struct sockaddr_in6 native;
    ::memcpy(&native, &storage, sizeof(native));
    EXPECT_EQ(native.sin6_port, htons(4433U));
    EXPECT_EQ(native.sin6_scope_id, 7U);
    EXPECT_EQ(native.sin6_addr.s6_addr[15], 0x42U);

    endpoint restored;
    ASSERT_EQ(detail::from_native_endpoint(storage, length, restored), castle::status::ok);
    EXPECT_EQ(restored, original);
    EXPECT_EQ(detail::from_native_endpoint(storage, length - 1, restored), castle::status::invalid_argument);

    const endpoint v4 = endpoint::ipv4(192U, 168U, 7U, 9U, 0x1234U);
    ASSERT_EQ(detail::to_native_endpoint(v4, storage, length), castle::status::ok);
    ASSERT_EQ(length, static_cast<socklen_t>(sizeof(struct sockaddr_in)));
    struct sockaddr_in native_v4;
    ::memcpy(&native_v4, &storage, sizeof(native_v4));
    EXPECT_EQ(native_v4.sin_port, htons(0x1234U));
    ASSERT_EQ(detail::from_native_endpoint(storage, length, restored), castle::status::ok);
    EXPECT_EQ(restored, v4);
}

TEST(NetSocket, TcpSendReportsFullWhenPeerDoesNotRead)
{
    tcp_socket listener;
    tcp_socket client;
    tcp_socket accepted;
    connect_pair(listener, client, accepted);

    static uint8_t chunk[65536];
    castle::status result = castle::status::ok;
    size_type total = 0U;
    int system_error = 0;
    for (int i = 0; i < 4096 && castle::succeeded(result); ++i)
    {
        size_type sent = 0U;
        result = client.send_some(chunk, sizeof(chunk), sent, &system_error);
        total += sent;
    }
    EXPECT_EQ(result, castle::status::full);
    EXPECT_TRUE(system_error == EAGAIN || system_error == EWOULDBLOCK);
    EXPECT_GT(total, 0U);

    size_type bytes = 99U;
    EXPECT_EQ(client.send_all(chunk, sizeof(chunk), bytes), castle::status::full);
    EXPECT_LT(bytes, sizeof(chunk));
}

TEST(NetSocket, SendToClosedPeerFailsWithoutRaisingSigpipe)
{
    tcp_socket listener;
    tcp_socket client;
    tcp_socket accepted;
    connect_pair(listener, client, accepted);
    ASSERT_EQ(accepted.close(), castle::status::ok);

    castle::status result = castle::status::ok;
    int system_error = 0;
    for (int i = 0; i < 100 && castle::succeeded(result); ++i)
    {
        size_type sent = 0U;
        result = client.send_some("x", 1U, sent, &system_error);
        ::usleep(1000);
    }
    EXPECT_EQ(result, castle::status::system_call_error);
    EXPECT_TRUE(system_error == EPIPE || system_error == ECONNRESET) << system_error;
}

TEST(NetSocket, ConnectToClosedPortReportsRefusal)
{
    endpoint refused;
    {
        tcp_socket probe;
        ASSERT_EQ(probe.open(), castle::status::ok);
        ASSERT_EQ(probe.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
        ASSERT_EQ(probe.local_endpoint(refused), castle::status::ok);
    }
    tcp_socket client;
    ASSERT_EQ(client.open(), castle::status::ok);
    int system_error = 0;
    castle::status result = client.connect(refused, &system_error);
    if (result == castle::status::time_out)
    {
        EXPECT_TRUE(system_error == EINPROGRESS || system_error == EALREADY);
        result = client.finish_connect(1000, &system_error);
    }
    EXPECT_EQ(result, castle::status::system_call_error);
    EXPECT_EQ(system_error, ECONNREFUSED);
}

TEST(NetSocket, FinishConnectRejectsSocketThatNeverConnected)
{
    tcp_socket client;
    ASSERT_EQ(client.open(), castle::status::ok);
    int system_error = 0;
    EXPECT_EQ(client.finish_connect(0, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, ENOTCONN);
}

TEST(NetSocket, SendAllOfNothingClearsSystemError)
{
    tcp_socket listener;
    tcp_socket client;
    tcp_socket accepted;
    connect_pair(listener, client, accepted);
    int system_error = 99;
    size_type bytes = 5U;
    EXPECT_EQ(client.send_all(nullptr, 0U, bytes, &system_error), castle::status::ok);
    EXPECT_EQ(bytes, 0U);
    EXPECT_EQ(system_error, 0);
}

TEST(NetSocket, UdpReceiveSomeReportsTruncatedDatagramAsDataLoss)
{
    udp_socket server;
    udp_socket client;
    ASSERT_EQ(server.open(), castle::status::ok);
    ASSERT_EQ(server.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    endpoint local;
    ASSERT_EQ(server.local_endpoint(local), castle::status::ok);
    ASSERT_EQ(client.open(), castle::status::ok);
    ASSERT_EQ(client.connect(local), castle::status::ok);

    const uint8_t payload[8] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U};
    size_type bytes = 0U;
    ASSERT_EQ(client.send_some(payload, sizeof(payload), bytes), castle::status::ok);
    ASSERT_EQ(server.wait_readable(1000), castle::status::ok);
    uint8_t small[2] = {};
    EXPECT_EQ(server.receive_some(small, sizeof(small), bytes), castle::status::data_loss);
    EXPECT_EQ(bytes, sizeof(small));
    EXPECT_EQ(small[1], 1U);
    EXPECT_EQ(server.receive_some(small, sizeof(small), bytes), castle::status::empty); // the rest was dropped

    ASSERT_EQ(client.send_some(payload, 2U, bytes), castle::status::ok);
    ASSERT_EQ(server.wait_readable(1000), castle::status::ok);
    EXPECT_EQ(server.receive_some(small, sizeof(small), bytes), castle::status::ok);
    EXPECT_EQ(bytes, 2U);
}

TEST(NetSocket, ShutdownRejectsUnknownModeAndZeroCapacityReceiveIsOk)
{
    tcp_socket listener;
    tcp_socket client;
    tcp_socket accepted;
    connect_pair(listener, client, accepted);
    EXPECT_EQ(client.shutdown(static_cast<shutdown_mode>(9)), castle::status::invalid_argument);
    size_type bytes = 7U;
    EXPECT_EQ(accepted.receive_some(nullptr, 0U, bytes), castle::status::ok);
    EXPECT_EQ(bytes, 0U);
}

TEST(NetSocket, WaitReadableReportsTimeoutReadinessAndClosedSocket)
{
    udp_socket server;
    EXPECT_EQ(server.wait_readable(0), castle::status::not_configured);
    ASSERT_EQ(server.open(), castle::status::ok);
    ASSERT_EQ(server.bind(endpoint::loopback_ipv4(0U)), castle::status::ok);
    EXPECT_EQ(server.wait_readable(0), castle::status::time_out);
    endpoint local;
    ASSERT_EQ(server.local_endpoint(local), castle::status::ok);
    udp_socket client;
    ASSERT_EQ(client.open(), castle::status::ok);
    size_type bytes = 0U;
    ASSERT_EQ(client.send_to("x", 1U, local, bytes), castle::status::ok);
    EXPECT_EQ(server.wait_readable(1000), castle::status::ok);
}

#else

TEST(NetSocket, UnsupportedTargetReportsNotConfigured)
{
    tcp_socket socket;
    EXPECT_FALSE(sockets_available);
    EXPECT_EQ(socket.open(), castle::status::not_configured);
}

#endif

} // namespace
