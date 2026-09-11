#include <gtest/gtest.h>
#include "castle/container/heap.h"

namespace
{
TEST(HeapTest, EmptyAndPushPopBranches)
{
    castle::container::min_heap<int,4> h;
    static_assert(noexcept(h.push(1)),"noexcept");
    EXPECT_EQ(h.capacity(),4U);
    EXPECT_EQ(h.available(),4U);
    EXPECT_TRUE(h.empty());
    EXPECT_FALSE(h.full());
    EXPECT_EQ(h.pop(),castle::status::empty);
    int out=0;
    EXPECT_EQ(h.pop(out),castle::status::empty);
    EXPECT_EQ(h.begin(),nullptr);
    EXPECT_EQ(h.end(),nullptr);
    EXPECT_EQ(h.push(3),castle::status::ok);
    EXPECT_EQ(h.push(1),castle::status::ok);
    EXPECT_EQ(h.push(2),castle::status::ok);
    EXPECT_EQ(h.emplace(0),castle::status::ok);
    EXPECT_TRUE(h.full());
    EXPECT_EQ(h.push(5),castle::status::full);
    EXPECT_EQ(h.top(),0);
    EXPECT_EQ(h.pop(out),castle::status::ok);
    EXPECT_EQ(out,0);
    EXPECT_EQ(h.top(),1);
    EXPECT_EQ(h.pop(),castle::status::ok);
    EXPECT_EQ(h.pop(),castle::status::ok);
    EXPECT_EQ(h.pop(),castle::status::ok);
    EXPECT_TRUE(h.empty());
}

TEST(HeapTest, RemoveAtSiftsUpWhenReplacementBeatsParent)
{
    castle::container::min_heap<int,6> h;
    for(int x : {0,100,1,101,102,50})
    {
        EXPECT_EQ(h.push(x),castle::status::ok);
    }
    EXPECT_EQ(h.remove_at(3),castle::status::ok);
    EXPECT_EQ(h.size(),5U);
    EXPECT_EQ(h.top(),0);
}

TEST(HeapTest, RemoveReplaceClearAndMaxHeap)
{
    castle::container::min_heap<int,6> h;
    for(int x:{5,1,4,2,3})
    {
        EXPECT_EQ(h.push(x),castle::status::ok);
    }
    EXPECT_EQ(h.remove_at(99),castle::status::out_of_range);
    EXPECT_EQ(h.replace_top(9),castle::status::ok);
    EXPECT_EQ(h.top(),2);
    EXPECT_EQ(h.replace_top(0),castle::status::ok);
    EXPECT_EQ(h.top(),0);
    EXPECT_EQ(h.remove_at(0),castle::status::ok);
    EXPECT_EQ(h.size(),4U);
    EXPECT_EQ(h.remove_at(h.size()-1),castle::status::ok);
    EXPECT_EQ(h.size(),3U);
    h.clear();
    EXPECT_TRUE(h.empty());
    EXPECT_EQ(h.replace_top(1),castle::status::empty);
    castle::container::max_heap<int,3> m;
    m.push(1);
    m.push(3);
    m.push(2);
    EXPECT_EQ(m.top(),3);
    const auto& cm=m;
    EXPECT_EQ(cm.top(),3);
    EXPECT_NE(m.begin(),nullptr);
    EXPECT_NE(m.end(),nullptr);
}
} // namespace
