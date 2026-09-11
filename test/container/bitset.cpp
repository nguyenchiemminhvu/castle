#include <gtest/gtest.h>
#include "castle/container/bitset.h"

namespace
{
TEST(BitsetTest, SetResetFlipCountAndConversion)
{
    using B=castle::container::bitset<40>;
    B b;
    EXPECT_EQ(b.size(),40U);
    EXPECT_TRUE(b.none());
    EXPECT_FALSE(b.any());
    EXPECT_FALSE(b.all());
    EXPECT_EQ(b.to_uint32(),0U);
    EXPECT_EQ(b.to_uint64(),0U);
    EXPECT_EQ(b.set(0),castle::status::ok);
    EXPECT_EQ(b.set(31),castle::status::ok);
    EXPECT_EQ(b.set(32),castle::status::ok);
    EXPECT_EQ(b.count(),3U);
    EXPECT_TRUE(b[0]);
    EXPECT_TRUE(static_cast<const B&>(b)[32]);
    EXPECT_EQ(b.set(40),castle::status::out_of_range);
    EXPECT_FALSE(b.test(40));
    EXPECT_EQ(b.reset(40),castle::status::out_of_range);
    EXPECT_EQ(b.flip(40),castle::status::out_of_range);
    b[1]=true;
    EXPECT_TRUE(b[1]);
    b[1]=false;
    EXPECT_FALSE(b[1]);
    auto r=b[0]; r.flip();
    EXPECT_FALSE(b[0]);
    r=true;
    EXPECT_TRUE(b[0]);
    auto r2=b[31];
    r2=b[0];
    EXPECT_TRUE(b[31]);
    b.flip(32);
    EXPECT_FALSE(b[32]);
    b.flip(32);
    EXPECT_TRUE(b[32]);
    uint64_t raw=(uint64_t(1)<<0)|(uint64_t(1)<<31)|(uint64_t(1)<<32);
    EXPECT_EQ(b.to_uint64(),raw);
    char text[41]{};
    EXPECT_EQ(b.to_string(text,41),castle::status::ok);
    EXPECT_EQ(text[40],'\0');
    EXPECT_EQ(b.to_string(nullptr,41),castle::status::invalid_argument);
    EXPECT_EQ(b.to_string(text,40),castle::status::invalid_argument);
    b.set();
    EXPECT_TRUE(b.all());
    EXPECT_EQ(b.count(),40U);
    EXPECT_EQ(b.to_uint64(),0xFFFFFFFFFFULL);
    b.reset();
    EXPECT_TRUE(b.none());
}

TEST(BitsetTest, BitwiseOperatorsAndExactWordBoundary)
{
    castle::container::bitset<64> a(uint64_t(0xF0F0F0F0F0F0F0F0ULL)), b(uint64_t(0x0F0F0F0F0F0F0F0FULL)); EXPECT_EQ((a&b).count(),0U);
    EXPECT_EQ((a|b).count(),64U);
    EXPECT_EQ((a^b).count(),64U);
    auto n=~a;
    EXPECT_EQ(n.to_uint64(),0x0F0F0F0F0F0F0F0FULL);
    a&=b;
    EXPECT_TRUE(a.none());
    a|=b;
    EXPECT_EQ(a.to_uint64(),b.to_uint64());
    a^=b;
    EXPECT_TRUE(a.none());
}
}
