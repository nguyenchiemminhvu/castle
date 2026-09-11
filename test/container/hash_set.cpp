#include <gtest/gtest.h>
#include "castle/container/hash_set.h"

namespace
{
TEST(HashSetTest, BasicAndCapacity)
{
    castle::container::hash_set<int,3> s;
    EXPECT_EQ(s.capacity(),3U);
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.insert(2),castle::status::ok);
    EXPECT_EQ(s.insert(2),castle::status::already_exists);
    EXPECT_EQ(s.insert(1),castle::status::ok);
    EXPECT_EQ(s.insert(3),castle::status::ok);
    EXPECT_TRUE(s.full());
    EXPECT_EQ(s.insert(4),castle::status::full);
    EXPECT_TRUE(s.contains(1));
    EXPECT_FALSE(s.contains(9));
    auto it=s.find(2);
    EXPECT_NE(it,s.end());
    EXPECT_EQ(*it,2);
    const auto& cs=s;
    size_t n=0;
    for(auto i=cs.cbegin();i!=cs.cend();++i) ++n;
    EXPECT_EQ(n,3U);
    EXPECT_EQ(s.erase(2),castle::status::ok);
    EXPECT_EQ(s.erase(2),castle::status::not_found);
    s.clear();
    EXPECT_TRUE(s.empty());
}
}
