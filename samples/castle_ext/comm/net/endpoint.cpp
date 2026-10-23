#include "sample_support.hpp"

#include "castle_ext/comm/net/endpoint.hpp"

using namespace castle::comm::net;

// Endpoints are literal values, so they can be built and compared at compile time.
static_assert(endpoint::loopback_ipv4(8080U).port() == 8080U, "constexpr IPv4 endpoint");
static_assert(endpoint::any_ipv6(80U).is_any(), "constexpr IPv6 endpoint");
static_assert(endpoint::ipv4(10U, 0U, 0U, 1U, 80U) != endpoint::ipv4(10U, 0U, 0U, 2U, 80U), "constexpr comparison");

int main()
{
    const endpoint default_endpoint;
    CASTLE_SAMPLE_CHECK(default_endpoint.family() == address_family::ipv4);
    CASTLE_SAMPLE_CHECK(default_endpoint.is_any());
    CASTLE_SAMPLE_CHECK(default_endpoint.port() == 0U);

    const endpoint gateway = endpoint::ipv4(192U, 168U, 1U, 1U, 8080U);
    CASTLE_SAMPLE_CHECK(gateway.family() == address_family::ipv4);
    CASTLE_SAMPLE_CHECK(gateway.address_size() == 4U);
    CASTLE_SAMPLE_CHECK(gateway.address_bytes()[0] == 192U);
    CASTLE_SAMPLE_CHECK(gateway.address_bytes()[3] == 1U);
    CASTLE_SAMPLE_CHECK(gateway.port() == 8080U);
    CASTLE_SAMPLE_CHECK(gateway == endpoint::ipv4(192U, 168U, 1U, 1U, 8080U));
    CASTLE_SAMPLE_CHECK(gateway != endpoint::ipv4(192U, 168U, 1U, 2U, 8080U));

    uint8_t ipv6_bytes[16] = {};
    ipv6_bytes[0] = 0xFEU;
    ipv6_bytes[1] = 0x80U;
    ipv6_bytes[15] = 1U;
    const endpoint link_local = endpoint::ipv6(ipv6_bytes, 443U, 3U);
    CASTLE_SAMPLE_CHECK(link_local.family() == address_family::ipv6);
    CASTLE_SAMPLE_CHECK(link_local.address_size() == 16U);
    CASTLE_SAMPLE_CHECK(link_local.scope_id() == 3U);
    CASTLE_SAMPLE_CHECK(link_local.port() == 443U);
    CASTLE_SAMPLE_CHECK(!link_local.is_any());
    CASTLE_SAMPLE_CHECK(endpoint::any_ipv6(80U).is_any());
    CASTLE_SAMPLE_CHECK(endpoint::loopback_ipv6(80U).address_bytes()[15] == 1U);

    return 0;
}
