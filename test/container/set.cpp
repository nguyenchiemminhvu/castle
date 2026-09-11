#include <gtest/gtest.h>
#include "castle/container/set.h"

namespace
{
TEST(SetTest, FullWrapperCoverage)
{
    using S=castle::container::set<int,3>;
    S s;
    EXPECT_EQ(s.capacity(),3U); EXPECT_EQ(s.insert(2),castle::status::ok);
    EXPECT_EQ(s.insert(1),castle::status::ok);
    EXPECT_EQ(s.insert(3),castle::status::ok);
    EXPECT_EQ(s.insert(4),castle::status::full);
    EXPECT_EQ(s.insert(2),castle::status::already_exists);
    EXPECT_TRUE(s.contains(1));
    EXPECT_FALSE(s.contains(8));
    EXPECT_EQ(*s.find(2),2);
    EXPECT_EQ(*s.lower_bound(2),2);
    EXPECT_EQ(*s.upper_bound(2),3); const S& cs=s; auto i=cs.begin();
    EXPECT_EQ(*i,1);
    ++i;
    EXPECT_EQ(*i,2);
    i++;
    EXPECT_EQ(*i,3);
    EXPECT_EQ(s.erase(2),castle::status::ok);
    EXPECT_EQ(s.erase(2),castle::status::not_found);
    s.clear();
    EXPECT_TRUE(s.empty());
}
}
