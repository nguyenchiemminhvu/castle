#include "sample_support.hpp"

#include "castle_ext/net/net.hpp"

#include "castle/core/traits.hpp"

using namespace castle::net;

int main()
{
    static_assert(castle::meta::is_same<tcp_socket, basic_socket<socket_protocol::tcp>>::value,
                  "TCP socket alias has a compile-time protocol");
    static_assert(castle::meta::is_same<udp_socket, basic_socket<socket_protocol::udp>>::value,
                  "UDP socket alias has a compile-time protocol");
    static_assert(tcp_server<4U>::client_capacity() == 4U, "TCP capacity is compile time");
    static_assert(udp_server<3U>::client_capacity() == 3U, "UDP peer capacity is compile time");

    const endpoint endpoint_v4 = endpoint::loopback_ipv4(8080U);
    const endpoint endpoint_v6 = endpoint::loopback_ipv6(8080U);
    CASTLE_SAMPLE_CHECK(endpoint_v4.family() == address_family::ipv4);
    CASTLE_SAMPLE_CHECK(endpoint_v6.family() == address_family::ipv6);
    CASTLE_SAMPLE_CHECK(castle::sockets_available == (CASTLE_USING_POSIX_APIS != 0));

    tcp_client<> tcp;
    udp_client<> udp;
    tcp_server<4U> tcp_srv;
    udp_server<4U> udp_srv;
    CASTLE_SAMPLE_CHECK(!tcp.is_open() && !udp.is_open());
    CASTLE_SAMPLE_CHECK(!tcp_srv.is_open() && !udp_srv.is_open());
    return 0;
}
