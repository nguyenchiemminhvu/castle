#include <gtest/gtest.h>
#include "castle/container/map.h"

namespace
{
TEST(MapTest, FullWrapperCoverage)
{
    using M=castle::container::map<int,int,3>;
    M m;
    EXPECT_EQ(m.capacity(),3U);
    EXPECT_EQ(m.insert(2,20),castle::status::ok);
    EXPECT_EQ(m.insert(1,10),castle::status::ok);
    int v=30;
    EXPECT_EQ(m.insert(3,std::move(v)),castle::status::ok);
    EXPECT_EQ(m.insert(4,40),castle::status::full);
    EXPECT_EQ(*m.get(2),20);
    EXPECT_EQ(m.get(9),nullptr);
    EXPECT_TRUE(m.contains(1));
    EXPECT_FALSE(m.contains(9));
    EXPECT_EQ(m.try_emplace(2,200),castle::status::already_exists);
    EXPECT_EQ(m.find(2)->second,20);
    EXPECT_EQ(m.lower_bound(2)->first,2);
    EXPECT_EQ(m.upper_bound(2)->first,3);
    EXPECT_EQ(m.erase(2),castle::status::ok);
    EXPECT_EQ(m.erase(2),castle::status::not_found);
    auto it=m.begin();
    EXPECT_NE(m.erase(it),m.end());
    const M& cm=m;
    EXPECT_EQ(cm.cbegin()->first,3);
    m.clear();
    EXPECT_TRUE(m.empty());
}
}
