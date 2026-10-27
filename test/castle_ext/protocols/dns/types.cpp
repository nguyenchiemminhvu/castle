#include <gtest/gtest.h>
#include "castle_ext/protocols/dns/types.hpp"

TEST(DnsTypes, ResponseDefaultsAndClear)
{
    castle::protocols::dns::response<3U> value;
    EXPECT_EQ(value.record_count, 0U);
    EXPECT_EQ(value.rcode, castle::protocols::dns::response_code::no_error);
    value.transaction_id = 0x1234U;
    value.record_count = 2U;
    value.records[0U].ttl = 99U;
    value.clear();
    EXPECT_EQ(value.transaction_id, 0U);
    EXPECT_EQ(value.record_count, 0U);
    EXPECT_EQ(value.records[0U].ttl, 99U); // clear resets logical size, not every unused slot
    EXPECT_FALSE(value.truncated);
}

TEST(DnsTypes, QueryEnumsMatchWireValues)
{
    using castle::protocols::dns::record_type;
    EXPECT_EQ(static_cast<uint16_t>(record_type::a), 1U);
    EXPECT_EQ(static_cast<uint16_t>(record_type::ns), 2U);
    EXPECT_EQ(static_cast<uint16_t>(record_type::cname), 5U);
    EXPECT_EQ(static_cast<uint16_t>(record_type::mx), 15U);
    EXPECT_EQ(static_cast<uint16_t>(record_type::aaaa), 28U);
}
