#include <gtest/gtest.h>
#include "castle/container/forward_list.h"

namespace
{
using FL=castle::container::forward_list<int,4>;

TEST(ForwardListTest, EmptyStateAndIteratorBranches)
{
    FL l;
    EXPECT_EQ(l.capacity(),4U);
    EXPECT_EQ(l.max_size(),4U);
    EXPECT_EQ(l.size(),0U);
    EXPECT_EQ(l.available(),4U);
    EXPECT_TRUE(l.empty());
    EXPECT_FALSE(l.full());
    EXPECT_TRUE(l.begin()==l.end());
    EXPECT_TRUE(l.cbegin()==l.cend());
    auto b=l.before_begin();
    EXPECT_TRUE(b!=l.end());
    ++b;
    EXPECT_TRUE(b==l.end());
    EXPECT_EQ(l.pop_front(),castle::status::empty);
    EXPECT_EQ(l.erase_after(l.end()),l.end());
    EXPECT_EQ(l.remove(7),castle::status::ok);
    FL::const_iterator cend{};
    ++cend;
    EXPECT_TRUE(cend == FL::const_iterator{});
}

TEST(ForwardListTest, EmplacePushEraseRemoveReverseAndReuse)
{
    FL l;
    EXPECT_EQ(l.push_front(1),castle::status::ok);
    EXPECT_EQ(l.push_front(2),castle::status::ok);
    EXPECT_EQ(l.emplace_front(3),castle::status::ok);
    EXPECT_EQ(l.front(),3);
    auto p=l.begin();
    EXPECT_EQ(*p,3);
    EXPECT_EQ(*(p.operator->()),3);
    auto before=l.before_begin();
    auto inserted=l.emplace_after(before,4);
    EXPECT_EQ(inserted,castle::status::ok);
    EXPECT_EQ(l.front(),4);
    EXPECT_EQ(l.size(),4U);
    EXPECT_TRUE(l.full());
    EXPECT_EQ(l.push_front(5),castle::status::full);
    auto first=l.begin();
    auto next=l.erase_after(first);
    EXPECT_NE(next,l.end());
    EXPECT_EQ(l.size(),3U);
    EXPECT_EQ(l.pop_front(),castle::status::ok);
    EXPECT_EQ(l.front(),2);
    auto bb=l.before_begin();
    EXPECT_NE(l.erase_after(bb),l.end());
    EXPECT_EQ(l.front(),1);
    EXPECT_EQ(l.emplace_front(1),castle::status::ok);
    EXPECT_EQ(l.emplace_front(1),castle::status::ok);
    EXPECT_EQ(l.remove(1),castle::status::ok);
    EXPECT_EQ(l.find(1),l.end());
    EXPECT_EQ(l.remove(99),castle::status::ok);
    EXPECT_EQ(l.emplace_front(7),castle::status::ok);
    EXPECT_EQ(l.emplace_front(8),castle::status::ok);
    auto e=l.erase_after(l.end());
    EXPECT_TRUE(e==l.end());
    l.reverse();
    int v[3]={};
    size_t i=0;
    for(auto it=l.begin();it!=l.end();++it)
    {
        v[i++]=*it;
    }
    EXPECT_EQ(i,2U);
    const FL& cl=l;
    EXPECT_EQ(cl.front(),v[0]);
    auto cit=cl.find(v[0]);
    EXPECT_TRUE(cit!=cl.end());
    EXPECT_EQ(*cit.operator->(),v[0]);
    l.clear();
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.available(),4U);
    EXPECT_EQ(l.emplace_after(l.end(),1),castle::status::out_of_range);
    EXPECT_EQ(l.push_front(9),castle::status::ok);
    EXPECT_EQ(l.front(),9);
}
}
