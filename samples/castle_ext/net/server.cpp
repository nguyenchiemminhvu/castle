#include "sample_support.hpp"

#include "castle_ext/net/server.hpp"
#include "castle_ext/net/client.hpp"

using namespace castle::net;
using castle::size_type;

int main()
{
#if CASTLE_USING_POSIX_APIS
    tcp_server<2U, 64U> server;
    size_type tcp_received = 0U;
    CASTLE_SAMPLE_CHECK(server.set_default_callback(
        [&tcp_received](tcp_server<2U, 64U>::client_handle, endpoint const& peer,
                        uint8_t const* data, size_type size)
        {
            if (peer.port() != 0U && size == 6U && data[0] == static_cast<uint8_t>('h')) { ++tcp_received; }
        }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server.open(endpoint::loopback_ipv4(0U), 2) == castle::status::ok);
    endpoint local;
    CASTLE_SAMPLE_CHECK(server.socket().local_endpoint(local) == castle::status::ok);

    tcp_client<64U> client;
    CASTLE_SAMPLE_CHECK(client.connect(local, 1000) == castle::status::ok);
    tcp_server<2U, 64U>::client_handle accepted;
    CASTLE_SAMPLE_CHECK(server.accept_client(accepted, 1000) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(accepted.is_valid() && server.client_count() == 1U);
    const char hello[] = "hello";
    size_type transferred = 0U;
    CASTLE_SAMPLE_CHECK(client.send_all(hello, sizeof(hello), transferred) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server.process_client(accepted, 1000) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tcp_received == 1U);
    const char world[] = "world";
    CASTLE_SAMPLE_CHECK(server.send_all(accepted, world, sizeof(world), transferred) == castle::status::ok);
    size_type client_received = 0U;
    CASTLE_SAMPLE_CHECK(client.set_receive_callback(
        [&client_received](endpoint const&, uint8_t const* bytes, size_type size)
        {
            if (size == 6U && bytes[0] == static_cast<uint8_t>('w')) { client_received = size; }
        }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client.process_once(1000) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client_received == sizeof(world));

    // Every client's descriptor can join one event manager, so waiting on N clients is a single poll().
    tcp_client<64U> second;
    CASTLE_SAMPLE_CHECK(second.connect(local, 1000) == castle::status::ok);
    tcp_server<2U, 64U>::client_handle second_accepted;
    CASTLE_SAMPLE_CHECK(server.accept_client(second_accepted, 1000) == castle::status::ok);
    tcp_server<2U, 64U>::client_handle third_rejected;
    CASTLE_SAMPLE_CHECK(server.accept_client(third_rejected) == castle::status::full);

    castle::io::fd::fd_event_manager manager;
    int ready_fd = -1;
    for (size_type slot = 0U; slot < server.client_capacity(); ++slot)
    {
        const int descriptor = server.client_native_handle(server.client_at(slot));
        CASTLE_SAMPLE_CHECK(manager.add_fd(descriptor, castle::io::fd::poll_read,
                                           [&ready_fd](int fd, short, void*) { ready_fd = fd; })
                            == castle::status::ok);
    }
    CASTLE_SAMPLE_CHECK(second.send_all(hello, sizeof(hello), transferred) == castle::status::ok);
    size_type dispatched = 0U;
    CASTLE_SAMPLE_CHECK(manager.wait_and_process(1000, dispatched) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(dispatched == 1U && ready_fd == server.client_native_handle(second_accepted));
    CASTLE_SAMPLE_CHECK(server.process_client(second_accepted, 1000) == castle::status::ok);
    manager.clear();

    CASTLE_SAMPLE_CHECK(server.close_client(accepted) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server.close_client(accepted) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(server.close() == castle::status::ok);

    // UDP peers are learned from datagram sources. A full table reports who was turned away, and
    // client_at() lets the application pick a peer to evict.
    udp_server<1U, 64U> datagram_server;
    size_type udp_received = 0U;
    CASTLE_SAMPLE_CHECK(datagram_server.set_default_callback(
        [&udp_received](udp_server<1U, 64U>::client_handle, endpoint const& peer,
                       uint8_t const* data, size_type size)
        {
            if (peer.port() != 0U && size == 5U && data[0] == static_cast<uint8_t>('p')) { ++udp_received; }
        }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(datagram_server.open(endpoint::loopback_ipv4(0U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(datagram_server.socket().local_endpoint(local) == castle::status::ok);

    udp_client<64U> udp;
    udp_client<64U> latecomer;
    CASTLE_SAMPLE_CHECK(udp.bind(endpoint::loopback_ipv4(0U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(latecomer.bind(endpoint::loopback_ipv4(0U)) == castle::status::ok);
    const char packet[] = "ping";
    CASTLE_SAMPLE_CHECK(udp.send_to(packet, sizeof(packet), local, transferred) == castle::status::ok);
    udp_server<1U, 64U>::client_handle remote;
    CASTLE_SAMPLE_CHECK(datagram_server.process_datagram(remote, 1000) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(remote.is_valid() && datagram_server.client_count() == 1U);
    CASTLE_SAMPLE_CHECK(udp_received == 1U);
    const char pong[] = "pong";
    CASTLE_SAMPLE_CHECK(datagram_server.send_some(remote, pong, sizeof(pong), transferred) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(udp.process_once(1000) == castle::status::ok);

    endpoint turned_away;
    udp_server<1U, 64U>::client_handle none;
    CASTLE_SAMPLE_CHECK(latecomer.send_to(packet, sizeof(packet), local, transferred) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(datagram_server.process_datagram(none, 1000, nullptr, &turned_away) == castle::status::full);
    CASTLE_SAMPLE_CHECK(!none.is_valid() && turned_away.port() != 0U && udp_received == 1U);
    CASTLE_SAMPLE_CHECK(datagram_server.close_client(datagram_server.client_at(0U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(datagram_server.client_count() == 0U);
    CASTLE_SAMPLE_CHECK(datagram_server.close() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(udp.close() == castle::status::ok);
#else
    tcp_server<2U> server;
    CASTLE_SAMPLE_CHECK(server.open(endpoint::loopback_ipv4(0U)) == castle::status::not_configured);
#endif
    return 0;
}
