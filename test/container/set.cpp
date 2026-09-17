#include <gtest/gtest.h>
#include "castle/container/set.hpp"

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

TEST(SetTest, InitializerListConstructor)
{
    using S=castle::container::set<int,4>;
    S s{3,1,2,1};
    EXPECT_EQ(s.size(),3U);
    EXPECT_TRUE(s.contains(1));
    EXPECT_TRUE(s.contains(2));
    EXPECT_TRUE(s.contains(3));
    auto it=s.begin();
    EXPECT_EQ(*it,1);
    ++it;
    EXPECT_EQ(*it,2);
    ++it;
    EXPECT_EQ(*it,3);
    S empty{};
    EXPECT_TRUE(empty.empty());
}

TEST(SetTest, InitializerListStopsWhenCapacityReached)
{
    using S = castle::container::set<int, 3>;

    S s{5, 1, 4, 2, 3, 6};

    EXPECT_EQ(s.size(), 3U);
    EXPECT_TRUE(s.full());

    // Only first 3 inserted values should be present
    EXPECT_TRUE(s.contains(5));
    EXPECT_TRUE(s.contains(1));
    EXPECT_TRUE(s.contains(4));

    // These are never processed because constructor breaks
    EXPECT_FALSE(s.contains(2));
    EXPECT_FALSE(s.contains(3));
    EXPECT_FALSE(s.contains(6));

    auto it = s.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 4);
    ++it;
    EXPECT_EQ(*it, 5);
    ++it;
    EXPECT_EQ(it, s.end());
}

TEST(SetTest, InitializerListBreakAfterCapacityDespiteDuplicates)
{
    using S = castle::container::set<int, 3>;

    S s{1, 1, 2, 3, 4, 5};

    EXPECT_EQ(s.size(), 3U);
    EXPECT_TRUE(s.full());

    EXPECT_TRUE(s.contains(1));
    EXPECT_TRUE(s.contains(2));
    EXPECT_TRUE(s.contains(3));

    EXPECT_FALSE(s.contains(4));
    EXPECT_FALSE(s.contains(5));
}

}
