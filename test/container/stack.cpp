#include <gtest/gtest.h>
#include "castle/container/stack.h"
namespace
{
TEST(StackTest, LIFOOverflowWrapAndBulk)
{
    using S=castle::container::stack_buffer<int,4>;
    S s;
    EXPECT_EQ(S::capacity(),4U);
    EXPECT_TRUE(s.empty());
    int out=0;
    EXPECT_FALSE(s.pop(out));
    EXPECT_FALSE(s.pop());
    EXPECT_TRUE(s.push(1));
    EXPECT_TRUE(s.push(2));
    int x=3;
    EXPECT_TRUE(s.push(std::move(x)));
    EXPECT_EQ(s.top(),3);
    EXPECT_TRUE(s.peek(0,out));
    EXPECT_EQ(out,1);
    EXPECT_TRUE(s.peek(2,out));
    EXPECT_EQ(out,3);
    EXPECT_FALSE(s.peek(3,out));
    EXPECT_FALSE(s.force_push(4));
    EXPECT_TRUE(s.full());
    EXPECT_TRUE(s.force_push(5));
    EXPECT_EQ(s.top(),5);
    EXPECT_EQ(s.size(),4U);
    EXPECT_TRUE(s.pop(out));
    EXPECT_EQ(out,5);
    EXPECT_TRUE(s.pop());
    EXPECT_EQ(s.top(),2);
    int src[]={6,7,8,9,10};
    EXPECT_EQ(s.push_bulk(nullptr,5),0U);
    EXPECT_EQ(s.push_bulk(src,5),2U);
    EXPECT_TRUE(s.full());
    int dst[8]={};
    EXPECT_EQ(s.pop_bulk(nullptr,8),0U);
    EXPECT_EQ(s.pop_bulk(dst,8),4U);
    EXPECT_EQ(dst[0],7);
    EXPECT_EQ(dst[3],1);
    EXPECT_TRUE(s.empty());
    s.clear();
    s.push(11);
    const S& cs=s;
    EXPECT_EQ(cs.top(),11);
}

TEST(StackTest, NonPowerOfTwoWrap)
{
    castle::container::stack_buffer<int,3> s;
    s.push(1);
    s.push(2);
    s.push(3);
    EXPECT_TRUE(s.force_push(4));
    int out[3]={};
    EXPECT_EQ(s.pop_bulk(out,3),3U);
    EXPECT_EQ(out[0],4);
    EXPECT_EQ(out[2],1);
}
}
