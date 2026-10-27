#include <gtest/gtest.h>
#include "castle_ext/protocols/dns/dns.hpp"

TEST(DnsUmbrella, ExposesCodecAndResolverTypes)
{
    uint8_t packet[512U]{};
    castle::size_type size = 0U;
    EXPECT_EQ(castle::protocols::dns::message::encode_query(
                  "example.com", castle::protocols::dns::record_type::aaaa,
                  7U, packet, sizeof(packet), size), castle::status::ok);
    EXPECT_EQ(size, 29U);
    EXPECT_EQ(packet[24U], 0U); // DNS QNAME terminator is in the encoded sequence
}
