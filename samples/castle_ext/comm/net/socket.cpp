#include "sample_support.hpp"

#include "castle_ext/comm/net/socket.hpp"

using namespace castle::comm::net;
using castle::size_type;

int main()
{
#if CASTLE_USING_POSIX_APIS
    tcp_socket listener;
    CASTLE_SAMPLE_CHECK(listener.open(address_family::ipv4, true) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(listener.set_reuse_address() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(listener.bind(endpoint::loopback_ipv4(0U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(listener.listen(2) == castle::status::ok);
    endpoint server_address;
    CASTLE_SAMPLE_CHECK(listener.local_endpoint(server_address) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server_address.port() != 0U);

    // A non-blocking connect is `time_out` while pending; finish_connect() waits for it to complete.
    tcp_socket client;
    CASTLE_SAMPLE_CHECK(client.open(address_family::ipv4, true) == castle::status::ok);
    castle::status connected = client.connect(server_address);
    if (connected == castle::status::time_out)
    {
        connected = client.finish_connect(1000);
    }
    CASTLE_SAMPLE_CHECK(connected == castle::status::ok);

    CASTLE_SAMPLE_CHECK(listener.wait_readable(1000) == castle::status::ok);
    tcp_socket peer;
    endpoint peer_address;
    CASTLE_SAMPLE_CHECK(listener.accept(peer, peer_address) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(peer_address.port() != 0U);
    CASTLE_SAMPLE_CHECK(peer.set_no_delay() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(peer.set_keep_alive() == castle::status::ok);

    const char request[] = "ping";
    size_type transferred = 0U;
    CASTLE_SAMPLE_CHECK(client.send_all(request, sizeof(request), transferred) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(transferred == sizeof(request));
    CASTLE_SAMPLE_CHECK(peer.wait_readable(1000) == castle::status::ok);
    char received[16] = {};
    size_type received_size = 0U;
    CASTLE_SAMPLE_CHECK(peer.receive_some(received, sizeof(received), received_size) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(received_size == sizeof(request));
    CASTLE_SAMPLE_CHECK(received[0] == 'p' && received[3] == 'g');

    const char response[] = "pong";
    CASTLE_SAMPLE_CHECK(peer.send_all(response, sizeof(response), transferred) == castle::status::ok);
    short ready = 0;
    CASTLE_SAMPLE_CHECK(client.wait(castle::io::fd::poll_read, 1000, ready) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client.receive_some(received, sizeof(received), received_size) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(received_size == sizeof(response) && received[1] == 'o');
    endpoint remote;
    CASTLE_SAMPLE_CHECK(client.peer_endpoint(remote) == castle::status::ok);

    // A TCP end-of-file is `empty` with `peer_closed` set, so it is not mistaken for would-block.
    CASTLE_SAMPLE_CHECK(peer.shutdown(shutdown_mode::send) == castle::status::ok);
    bool peer_closed = false;
    CASTLE_SAMPLE_CHECK(client.wait_readable(1000) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client.receive_some(received, sizeof(received), received_size, nullptr, &peer_closed)
                        == castle::status::empty);
    CASTLE_SAMPLE_CHECK(peer_closed);

    // UDP sends and receives one complete datagram with source endpoint metadata.
    udp_socket datagram_server;
    udp_socket datagram_client;
    CASTLE_SAMPLE_CHECK(datagram_server.open(address_family::ipv4) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(datagram_server.bind(endpoint::loopback_ipv4(0U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(datagram_server.local_endpoint(server_address) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(datagram_client.open(address_family::ipv4) == castle::status::ok);
    const char message[] = "udp";
    CASTLE_SAMPLE_CHECK(datagram_client.send_to(message, sizeof(message), server_address, transferred)
                        == castle::status::ok);
    CASTLE_SAMPLE_CHECK(datagram_server.wait_readable(1000) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(datagram_server.receive_from(received, sizeof(received), received_size, remote)
                        == castle::status::ok);
    CASTLE_SAMPLE_CHECK(received_size == sizeof(message) && remote.port() != 0U);

    // A datagram larger than the buffer is cut and reported as data_loss; the excess is dropped.
    CASTLE_SAMPLE_CHECK(datagram_client.send_to("0123456789", 10U, server_address, transferred) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(datagram_server.wait_readable(1000) == castle::status::ok);
    char small[4] = {};
    CASTLE_SAMPLE_CHECK(datagram_server.receive_from(small, sizeof(small), received_size, remote)
                        == castle::status::data_loss);
    CASTLE_SAMPLE_CHECK(received_size == sizeof(small) && small[3] == '3');

    CASTLE_SAMPLE_CHECK(client.close() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client.close() == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(listener.close() == castle::status::ok);
#else
    tcp_socket socket;
    CASTLE_SAMPLE_CHECK(socket.open() == castle::status::not_configured);
#endif
    return 0;
}
