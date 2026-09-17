#include <gtest/gtest.h>
#include "castle/container/hash_table.h"

namespace
{
struct ConstantHash
{
    size_t operator()(int) const noexcept
    {
        return 0U;
    }
};

TEST(HashTableTest, BasicLookupAssignmentEmplaceAndIterators)
{
    using H=castle::container::hash_table<int,int,4,ConstantHash>;
    H h;
    static_assert(H::capacity()==4U,"capacity");
    EXPECT_TRUE(h.empty()); EXPECT_TRUE(h.begin()==h.end());
    EXPECT_EQ(h.insert(1,10),castle::status::ok);
    EXPECT_EQ(h.insert(1,11),castle::status::already_exists);
    EXPECT_EQ(h.emplace(2,20),castle::status::ok);
    EXPECT_EQ(h.emplace(2,30),castle::status::already_exists);
    EXPECT_EQ(h.try_emplace(3,30),castle::status::ok);
    EXPECT_TRUE(h.contains(2));
    EXPECT_FALSE(h.contains(9));
    EXPECT_EQ(*h.get(2),20);
    EXPECT_EQ(h.get(9),nullptr); auto it=h.find(1);
    EXPECT_NE(it,h.end());
    EXPECT_EQ(it->first,1);
    EXPECT_EQ(it->second,10);
    it->second=12;
    EXPECT_EQ(h.get(1)[0],12);
    EXPECT_EQ(h.insert_or_assign(1,15),castle::status::ok);
    EXPECT_EQ(*h.get(1),15);
    int v=16;
    EXPECT_EQ(h.insert_or_assign(1,std::move(v)),castle::status::ok);
    EXPECT_EQ(*h.get(1),16);
    EXPECT_EQ(h.insert_or_assign(4,40),castle::status::ok);
    EXPECT_TRUE(h.full());
    EXPECT_EQ(h.insert(5,50),castle::status::full);
    const H& ch=h;
    auto ci=ch.find(3);
    EXPECT_EQ(ci->second,30);
    size_t count=0;
    for(auto p=ch.cbegin();p!=ch.cend();++p)
    {
        ++count;
    }
    EXPECT_EQ(count,4U);
}

TEST(HashTableTest, TombstonesProbeAndClear)
{
    using H=castle::container::hash_table<int,int,3,ConstantHash>;
    H h;
    h.insert(1,1);
    h.insert(2,2);
    h.insert(3,3);
    EXPECT_EQ(h.erase(2),castle::status::ok);
    EXPECT_EQ(h.erase(2),castle::status::not_found);
    EXPECT_FALSE(h.contains(2));
    EXPECT_TRUE(h.contains(3));
    EXPECT_EQ(h.insert(4,4),castle::status::ok);
    EXPECT_EQ(*h.get(4),4);
    EXPECT_EQ(h.erase(9),castle::status::not_found);
    h.clear();
    EXPECT_TRUE(h.empty());
    EXPECT_EQ(h.available(),3U);
    EXPECT_EQ(h.erase(1),castle::status::not_found);
}
}
