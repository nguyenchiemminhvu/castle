#include "sample_support.hpp"

#include "castle_ext/comm/net/client.hpp"

using namespace castle::comm::net;
using castle::size_type;

int main()
{
#if CASTLE_USING_POSIX_APIS
    tcp_socket listener;
    CASTLE_SAMPLE_CHECK(listener.open(address_family::ipv4) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(listener.bind(endpoint::loopback_ipv4(0U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(listener.listen(2) == castle::status::ok);
    endpoint server_address;
    CASTLE_SAMPLE_CHECK(listener.local_endpoint(server_address) == castle::status::ok);

    tcp_client<64U> client;
    size_type callback_count = 0U;
    size_type callback_bytes = 0U;
    CASTLE_SAMPLE_CHECK(client.set_receive_callback(
        [&callback_count, &callback_bytes](endpoint const& peer, uint8_t const* bytes, size_type size)
        {
            if (peer.port() != 0U && size != 0U && bytes[0] == static_cast<uint8_t>('o'))
            {
                ++callback_count;
                callback_bytes = size;
            }
        }) == castle::status::ok);
    // A failed connect closes the socket it opened, so the same client can simply try again.
    CASTLE_SAMPLE_CHECK(client.connect(server_address, 1000) == castle::status::ok);

    CASTLE_SAMPLE_CHECK(listener.wait_readable(1000) == castle::status::ok);
    tcp_socket peer;
    endpoint peer_address;
    CASTLE_SAMPLE_CHECK(listener.accept(peer, peer_address) == castle::status::ok);
    const char incoming[] = "ok";
    size_type transferred = 0U;
    CASTLE_SAMPLE_CHECK(peer.send_all(incoming, sizeof(incoming), transferred) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client.process_once(1000) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(callback_count == 1U && callback_bytes == sizeof(incoming));

    const char outgoing[] = "request";
    CASTLE_SAMPLE_CHECK(client.send_all(outgoing, sizeof(outgoing), transferred) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(peer.wait_readable(1000) == castle::status::ok);
    char received[32] = {};
    size_type received_size = 0U;
    CASTLE_SAMPLE_CHECK(peer.receive_some(received, sizeof(received), received_size) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(received_size == sizeof(outgoing) && received[0] == 'r');
    CASTLE_SAMPLE_CHECK(client.close() == castle::status::ok);

    // UDP client callbacks receive the source endpoint for each datagram. The callback runs in place,
    // so a mutable callable keeps its state, and it may clear or replace itself while it runs.
    udp_socket udp_server;
    CASTLE_SAMPLE_CHECK(udp_server.open(address_family::ipv4) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(udp_server.bind(endpoint::loopback_ipv4(0U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(udp_server.local_endpoint(server_address) == castle::status::ok);

    udp_client<64U> udp;
    size_type datagrams = 0U;
    CASTLE_SAMPLE_CHECK(udp.open(address_family::ipv4) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(udp.set_receive_callback(
        [&datagrams, &udp, seen = 0U](endpoint const& source, uint8_t const* bytes, size_type size) mutable
        {
            if (source.port() != 0U && size == 5U && bytes[0] == static_cast<uint8_t>('d')) { ++datagrams; }
            if (++seen == 2U) { udp.clear_receive_callback(); }
        }) == castle::status::ok);
    const char datagram[] = "data";
    for (size_type i = 0U; i < 3U; ++i)
    {
        CASTLE_SAMPLE_CHECK(udp.send_to(datagram, sizeof(datagram), server_address, transferred) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(udp_server.wait_readable(1000) == castle::status::ok);
        endpoint source;
        CASTLE_SAMPLE_CHECK(udp_server.receive_from(received, sizeof(received), received_size, source)
                            == castle::status::ok);
        const char reply[] = "done";
        CASTLE_SAMPLE_CHECK(udp_server.send_to(reply, sizeof(reply), source, transferred) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(udp.process_once(1000) == castle::status::ok);
    }
    CASTLE_SAMPLE_CHECK(datagrams == 2U); // the callback cleared itself during the second datagram
    CASTLE_SAMPLE_CHECK(udp.close() == castle::status::ok);
#else
    tcp_client<> client;
    CASTLE_SAMPLE_CHECK(client.open() == castle::status::not_configured);
#endif
    return 0;
}
