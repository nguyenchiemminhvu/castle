#include <gtest/gtest.h>
#include "castle/container/avl_tree.h"
#include <vector>
#include <initializer_list>

namespace
{
using T=castle::container::avl_tree<int,int,7>;

static void populate(T& t, std::initializer_list<int> ks)
{
    for(int k:ks)
    {
        EXPECT_EQ(t.insert(k,k*10),castle::status::ok);
    }
}

TEST(AvlTreeTest, BasicQueriesBoundsAndIterator)
{
    T t;
    EXPECT_EQ(t.capacity(),7U);
    EXPECT_TRUE(t.empty());
    EXPECT_EQ(t.insert(2,20),castle::status::ok);
    EXPECT_EQ(t.insert(1,10),castle::status::ok);
    EXPECT_EQ(t.insert(3,30),castle::status::ok);
    EXPECT_EQ(t.insert(2,22),castle::status::already_exists);
    EXPECT_TRUE(t.contains(1));
    EXPECT_FALSE(t.contains(9));
    EXPECT_EQ(*t.mapped(3),30);
    EXPECT_EQ(t.mapped(9),nullptr);
    auto lb=t.lower_bound(2);
    EXPECT_EQ(lb->first,2);
    auto ub=t.upper_bound(2);
    EXPECT_EQ(ub->first,3);
    EXPECT_EQ(t.lower_bound(9),t.end());
    EXPECT_EQ(t.upper_bound(9),t.end());
    std::vector<int> keys;
    for(auto it=t.begin();it!=t.end();++it)
    {
        keys.push_back(it->first);
    }
    EXPECT_EQ(keys.size(),3U);
    EXPECT_EQ(keys[0],1);
    EXPECT_EQ(keys[2],3);
    auto e=t.find(3);
    --e;
    EXPECT_EQ(e->first,2);
    auto p=e--;
    EXPECT_EQ(p->first,2);
    EXPECT_EQ(e->first,1);
    auto it2=t.find(2);
    auto after=it2;
    ++after;
    EXPECT_EQ(after->first,3);
    auto before=it2;
    --before;
    EXPECT_EQ(before->first,1);
    const T& ct=t;
    EXPECT_EQ(ct.cbegin()->first,1);
    auto ci=ct.find(2);
    EXPECT_EQ(ci->second,20);
    auto cb=ci;
    --cb;
    EXPECT_EQ(cb->first,1);
    auto ca=ci;
    ++ca;
    EXPECT_EQ(ca->first,3);
}

TEST(AvlTreeTest, RotationsAllShapes)
{
    T a;
    populate(a,{30,20,10});
    EXPECT_EQ(a.begin()->first,10);
    T b;
    populate(b,{10,20,30});
    EXPECT_EQ(b.begin()->first,10);
    auto bi=b.begin();
    ++bi;
    EXPECT_EQ(bi->first,20);
    T c;
    populate(c,{30,10,20});
    EXPECT_EQ(c.find(20)->first,20);
    T d;
    populate(d,{10,30,20});
    EXPECT_EQ(d.find(20)->first,20);
}

TEST(AvlTreeTest, EraseAllStructuralCasesAndCapacity)
{
    T t;
    populate(t,{50,30,70,20,40,60,80});
    EXPECT_EQ(t.erase(999),castle::status::not_found);
    EXPECT_EQ(t.erase(t.end()),t.end());
    auto it=t.find(20);
    auto nxt=t.erase(it);
    EXPECT_EQ(nxt->first,30);
    EXPECT_EQ(t.erase(80),castle::status::ok);
    EXPECT_EQ(t.erase(70),castle::status::ok);
    EXPECT_EQ(t.erase(50),castle::status::ok);
    EXPECT_EQ(t.size(),3U);
    EXPECT_EQ(t.erase(t.find(30))->first,40);
    T full;
    for(int i=0;i<7;++i)
    {
        EXPECT_EQ(full.insert(i,i),castle::status::ok);
    }
    EXPECT_TRUE(full.full());
    EXPECT_EQ(full.insert(99,99),castle::status::full);
    full.clear();
    EXPECT_TRUE(full.empty());
}
}
