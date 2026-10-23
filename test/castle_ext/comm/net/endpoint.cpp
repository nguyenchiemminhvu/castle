#include <gtest/gtest.h>

#include "castle/core/traits.hpp"
#include "castle_ext/comm/net/endpoint.hpp"

namespace
{

using castle::comm::net::address_family;
using castle::comm::net::endpoint;

static_assert(castle::meta::is_trivially_copyable<endpoint>::value, "endpoint should be a plain fixed-size value");
static_assert(castle::meta::is_standard_layout<endpoint>::value, "endpoint should have standard layout");

constexpr uint8_t kLinkLocal[16] = {0xFEU, 0x80U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U};
constexpr endpoint kConstexprV4 = endpoint::ipv4(10U, 0U, 0U, 1U, 80U);
constexpr endpoint kConstexprV6 = endpoint::ipv6(kLinkLocal, 443U, 3U);

static_assert(kConstexprV4.port() == 80U && kConstexprV4.family() == address_family::ipv4, "IPv4 is constexpr");
static_assert(kConstexprV4.address_bytes()[3] == 1U && kConstexprV4.address_size() == 4U, "IPv4 bytes are constexpr");
static_assert(kConstexprV6.scope_id() == 3U && kConstexprV6.address_size() == 16U, "IPv6 is constexpr");
static_assert(endpoint::any_ipv6(1U).is_any() && !endpoint::loopback_ipv6(1U).is_any(), "IPv6 factories are constexpr");
static_assert(kConstexprV4 == endpoint::ipv4(10U, 0U, 0U, 1U, 80U), "equality is constexpr");
static_assert(kConstexprV4 != endpoint::ipv4(10U, 0U, 0U, 1U, 81U), "inequality is constexpr");

TEST(NetEndpoint, DefaultIsIpv4AnyAddressAtPortZero)
{
    const endpoint value;
    EXPECT_EQ(value.family(), address_family::ipv4);
    EXPECT_EQ(value.port(), 0U);
    EXPECT_EQ(value.scope_id(), 0U);
    EXPECT_EQ(value.address_size(), 4U);
    EXPECT_TRUE(value.is_any());
}

TEST(NetEndpoint, BuildsIpv4FromOctetsAndHostOrderPort)
{
    const endpoint value = endpoint::ipv4(192U, 168U, 2U, 17U, 0x1234U);
    EXPECT_EQ(value.family(), address_family::ipv4);
    EXPECT_EQ(value.address_bytes()[0], 192U);
    EXPECT_EQ(value.address_bytes()[1], 168U);
    EXPECT_EQ(value.address_bytes()[2], 2U);
    EXPECT_EQ(value.address_bytes()[3], 17U);
    EXPECT_EQ(value.port(), 0x1234U);
    EXPECT_FALSE(value.is_any());
}

TEST(NetEndpoint, Ipv4WildcardAndLoopback)
{
    const endpoint any = endpoint::any_ipv4(80U);
    const endpoint loopback = endpoint::loopback_ipv4(443U);
    EXPECT_TRUE(any.is_any());
    EXPECT_EQ(any.port(), 80U);
    EXPECT_FALSE(loopback.is_any());
    EXPECT_EQ(loopback.address_bytes()[0], 127U);
    EXPECT_EQ(loopback.address_bytes()[3], 1U);
    EXPECT_EQ(loopback.port(), 443U);
}

TEST(NetEndpoint, BuildsIpv6AndRetainsScope)
{
    uint8_t bytes[16] = {};
    bytes[0] = 0xFEU;
    bytes[1] = 0x80U;
    bytes[15] = 0xA5U;
    const endpoint value = endpoint::ipv6(bytes, 53U, 9U);
    EXPECT_EQ(value.family(), address_family::ipv6);
    EXPECT_EQ(value.address_size(), 16U);
    EXPECT_EQ(value.address_bytes()[0], 0xFEU);
    EXPECT_EQ(value.address_bytes()[15], 0xA5U);
    EXPECT_EQ(value.port(), 53U);
    EXPECT_EQ(value.scope_id(), 9U);
    EXPECT_FALSE(value.is_any());
}

TEST(NetEndpoint, Ipv6AnyAcceptsNullAddressPointer)
{
    const endpoint value = endpoint::ipv6(nullptr, 123U, 4U);
    EXPECT_EQ(value.family(), address_family::ipv6);
    EXPECT_TRUE(value.is_any());
    EXPECT_EQ(value.scope_id(), 4U);
    EXPECT_EQ(value.port(), 123U);
    EXPECT_TRUE(endpoint::any_ipv6(0U).is_any());
}

TEST(NetEndpoint, Ipv6LoopbackUsesFinalOctet)
{
    const endpoint value = endpoint::loopback_ipv6(8080U);
    EXPECT_EQ(value.family(), address_family::ipv6);
    EXPECT_EQ(value.address_bytes()[15], 1U);
    for (castle::size_type i = 0U; i < 15U; ++i) { EXPECT_EQ(value.address_bytes()[i], 0U); }
    EXPECT_EQ(value.port(), 8080U);
}

TEST(NetEndpoint, EqualityComparesEveryIdentityField)
{
    const endpoint first = endpoint::ipv4(10U, 0U, 0U, 1U, 80U);
    EXPECT_EQ(first, endpoint::ipv4(10U, 0U, 0U, 1U, 80U));
    EXPECT_NE(first, endpoint::ipv4(10U, 0U, 0U, 2U, 80U));
    EXPECT_NE(first, endpoint::ipv4(10U, 0U, 0U, 1U, 81U));
    EXPECT_NE(first, endpoint::ipv6(nullptr, 80U));

    uint8_t bytes[16] = {};
    bytes[0] = 0xFEU;
    const endpoint scoped = endpoint::ipv6(bytes, 10U, 1U);
    EXPECT_NE(scoped, endpoint::ipv6(bytes, 10U, 2U));
    EXPECT_EQ(scoped, endpoint::ipv6(bytes, 10U, 1U));
}

TEST(NetEndpoint, Ipv6ComparisonAndAnyCoverEverySixteenBytes)
{
    for (castle::size_type i = 0U; i < 16U; ++i)
    {
        uint8_t bytes[16] = {};
        bytes[i] = 1U;
        const endpoint single_bit = endpoint::ipv6(bytes, 9U);
        EXPECT_FALSE(single_bit.is_any()) << "byte " << i;
        EXPECT_NE(single_bit, endpoint::any_ipv6(9U)) << "byte " << i;
        EXPECT_EQ(single_bit, endpoint::ipv6(bytes, 9U)) << "byte " << i;
    }
}

TEST(NetEndpoint, Ipv4ComparisonUsesAllFourBytes)
{
    EXPECT_EQ(endpoint::ipv4(1U, 2U, 3U, 4U, 5U), endpoint::ipv4(1U, 2U, 3U, 4U, 5U));
    EXPECT_NE(endpoint::ipv4(1U, 2U, 3U, 4U, 5U), endpoint::ipv4(1U, 2U, 3U, 5U, 5U));
}

} // namespace
