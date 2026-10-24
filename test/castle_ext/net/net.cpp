#include <gtest/gtest.h>

#include "castle/core/traits.hpp"
#include "castle_ext/net/net.hpp"

namespace
{

using namespace castle::net;

static_assert(castle::meta::is_same<tcp_socket, basic_socket<socket_protocol::tcp>>::value, "typed TCP alias");
static_assert(castle::meta::is_same<udp_socket, basic_socket<socket_protocol::udp>>::value, "typed UDP alias");
static_assert(tcp_server<5U>::client_capacity() == 5U, "compile-time TCP capacity");
static_assert(udp_server<7U>::client_capacity() == 7U, "compile-time UDP capacity");

TEST(NetUmbrella, ExposesAllNetworkingPublicTypes)
{
    const endpoint address = endpoint::loopback_ipv4(8080U);
    tcp_socket tcp;
    udp_socket udp;
    tcp_client<> tcp_app;
    udp_client<> udp_app;
    tcp_server<3U> tcp_service;
    udp_server<3U> udp_service;

    EXPECT_EQ(address.family(), address_family::ipv4);
    EXPECT_FALSE(tcp.is_open());
    EXPECT_FALSE(udp.is_open());
    EXPECT_FALSE(tcp_app.is_open());
    EXPECT_FALSE(udp_app.is_open());
    EXPECT_FALSE(tcp_service.is_open());
    EXPECT_FALSE(udp_service.is_open());
    EXPECT_EQ(sockets_available, (CASTLE_USING_POSIX_APIS != 0));
}

} // namespace
