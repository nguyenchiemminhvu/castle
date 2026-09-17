#include <gtest/gtest.h>
#include "castle/container/ring_buffer.h"

namespace
{
TEST(RingBufferTest, FIFOOverflowWrapAndBulk)
{
    using R=castle::container::ring_buffer<int,4>;
    R r;
    EXPECT_EQ(R::capacity(),4U);
    EXPECT_TRUE(r.empty());
    EXPECT_FALSE(r.full());
    EXPECT_EQ(r.available(),4U);
    int out=0;
    EXPECT_FALSE(r.pop(out));
    EXPECT_FALSE(r.pop());
    EXPECT_EQ(r.push(1),true);
    EXPECT_EQ(r.push(2),true);
    int x=3;
    EXPECT_TRUE(r.push(std::move(x)));
    EXPECT_EQ(r.front(),1);
    EXPECT_EQ(r.back(),3);
    EXPECT_TRUE(r.peek(1,out));
    EXPECT_EQ(out,2);
    EXPECT_FALSE(r.peek(3,out));
    EXPECT_EQ(r.force_push(4),false);
    EXPECT_EQ(r.force_push(5),true);
    EXPECT_EQ(r.size(),4U);
    EXPECT_EQ(r.front(),2);
    EXPECT_EQ(r.back(),5);
    EXPECT_EQ(r.pop(out),true);
    EXPECT_EQ(out,2);
    int src[]={6,7,8,9,10};
    EXPECT_EQ(r.push_bulk(nullptr,5),0U);
    EXPECT_EQ(r.push_bulk(src,5),1U);
    EXPECT_TRUE(r.full());
    int dst[8]={};
    EXPECT_EQ(r.pop_bulk(nullptr,8),0U);
    EXPECT_EQ(r.pop_bulk(dst,8),4U);
    EXPECT_EQ(dst[0],3);
    EXPECT_EQ(dst[3],6);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(r.pop_bulk(dst,2),0U);
    r.clear();
    EXPECT_TRUE(r.empty());
    r.push(11);
    EXPECT_EQ(r.front(),11);
    const R& cr=r;
    EXPECT_EQ(cr.front(),11);
    EXPECT_EQ(cr.back(),11);
}

TEST(RingBufferTest, NonPowerOfTwoWrap)
{
    castle::container::ring_buffer<int,3> r;
    r.push(1);
    r.push(2);
    r.push(3);
    EXPECT_FALSE(r.push(4));
    EXPECT_TRUE(r.force_push(4));
    int out[3]={};
    EXPECT_EQ(r.pop_bulk(out,3),3U);
    EXPECT_EQ(out[0],2);
    EXPECT_EQ(out[2],4);
}
} // namespace
