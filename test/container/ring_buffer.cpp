#include <gtest/gtest.h>
#include "castle/container/ring_buffer.hpp"

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

TEST(RingBufferTest, InitializerListConstructor)
{
    castle::container::ring_buffer<int,4> r{1,2,3};
    EXPECT_EQ(r.size(),3U);
    EXPECT_EQ(r.front(),1);
    EXPECT_EQ(r.back(),3);
    int out=0;
    EXPECT_TRUE(r.pop(out));
    EXPECT_EQ(out,1);
    EXPECT_TRUE(r.pop(out));
    EXPECT_EQ(out,2);
    EXPECT_TRUE(r.pop(out));
    EXPECT_EQ(out,3);
    EXPECT_TRUE(r.empty());

    castle::container::ring_buffer<int,3> full{1,2,3};
    EXPECT_TRUE(full.full());
    EXPECT_FALSE(full.push(4));
    while (!full.empty())
    {
        int temp;
        EXPECT_TRUE(full.pop(temp));
    }
    EXPECT_FALSE(full.pop(out));
}

TEST(RingBufferTest, InitializerListConstructorOverflowBreak)
{
    using R = castle::container::ring_buffer<int, 3>;

    // List contains more elements than capacity.
    // Constructor should stop when buffer becomes full.
    R r{1, 2, 3, 4, 5, 6};

    EXPECT_TRUE(r.full());
    EXPECT_EQ(r.size(), 3U);

    int out = 0;

    EXPECT_TRUE(r.pop(out));
    EXPECT_EQ(out, 1);

    EXPECT_TRUE(r.pop(out));
    EXPECT_EQ(out, 2);

    EXPECT_TRUE(r.pop(out));
    EXPECT_EQ(out, 3);

    EXPECT_TRUE(r.empty());
    EXPECT_FALSE(r.pop(out));
}

TEST(RingBufferTest, PushReturnsFalseWhenFull)
{
    castle::container::ring_buffer<int, 3> r;

    EXPECT_TRUE(r.push(1));
    EXPECT_TRUE(r.push(2));
    EXPECT_TRUE(r.push(3));

    EXPECT_TRUE(r.full());
    EXPECT_FALSE(r.push(4)); // cover return false branch

    EXPECT_EQ(r.size(), 3U);
    EXPECT_EQ(r.front(), 1);
    EXPECT_EQ(r.back(), 3);
}

TEST(RingBufferTest, PushOnFullBufferDoesNotChangeContents)
{
    castle::container::ring_buffer<int, 2> r;
    const int val1 = 10;
    const int val2 = 20;
    const int val3 = 30;

    EXPECT_TRUE(r.push(val1));
    EXPECT_TRUE(r.push(val2));

    EXPECT_FALSE(r.push(val3));

    int out = 0;
    EXPECT_TRUE(r.pop(out));
    EXPECT_EQ(out, val1);

    EXPECT_TRUE(r.pop(out));
    EXPECT_EQ(out, val2);

    EXPECT_TRUE(r.empty());
}

TEST(RingBufferTest, PopWithoutOutputParameterSuccessPath)
{
    castle::container::ring_buffer<int, 3> r;

    EXPECT_TRUE(r.push(10));
    EXPECT_TRUE(r.push(20));

    EXPECT_FALSE(r.empty());
    EXPECT_EQ(r.size(), 2U);

    EXPECT_TRUE(r.pop()); // cover non-empty branch

    EXPECT_EQ(r.size(), 1U);
    EXPECT_EQ(r.front(), 20);

    EXPECT_TRUE(r.pop()); // cover again

    EXPECT_TRUE(r.empty());
    EXPECT_EQ(r.size(), 0U);

    EXPECT_FALSE(r.pop()); // existing empty path
}

} // namespace
