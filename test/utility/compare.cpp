#include <gtest/gtest.h>

#include "castle/utility/compare.h"

namespace
{

TEST(CompareTest, LessFunctor)
{
    castle::less<int> less;
    EXPECT_TRUE(less(1, 2));
    EXPECT_FALSE(less(2, 1));
    EXPECT_FALSE(less(2, 2));
}

TEST(CompareTest, GreaterFunctor)
{
    castle::greater<int> greater;
    EXPECT_TRUE(greater(2, 1));
    EXPECT_FALSE(greater(1, 2));
    EXPECT_FALSE(greater(2, 2));
}

TEST(CompareTest, EqualToFunctor)
{
    castle::equal_to<int> equal;
    EXPECT_TRUE(equal(3, 3));
    EXPECT_FALSE(equal(3, 4));
}

TEST(CompareTest, FunctorsAreConstexpr)
{
    static_assert(castle::less<int>()(1, 2), "less must be constexpr");
    static_assert(castle::greater<int>()(2, 1), "greater must be constexpr");
    static_assert(castle::equal_to<int>()(5, 5), "equal_to must be constexpr");
}

TEST(CompareTest, CompareRelationalOperations)
{
    using cmp = castle::compare<int>;
    EXPECT_TRUE(cmp::lt(1, 2));
    EXPECT_FALSE(cmp::lt(2, 1));
    EXPECT_TRUE(cmp::gt(2, 1));
    EXPECT_TRUE(cmp::lte(2, 2));
    EXPECT_TRUE(cmp::lte(1, 2));
    EXPECT_TRUE(cmp::gte(2, 2));
    EXPECT_TRUE(cmp::gte(2, 1));
    EXPECT_TRUE(cmp::eq(2, 2));
    EXPECT_FALSE(cmp::eq(2, 3));
    EXPECT_TRUE(cmp::ne(2, 3));
    EXPECT_FALSE(cmp::ne(2, 2));
}

TEST(CompareTest, CmpReturnsOrdering)
{
    using cmp = castle::compare<int>;
    EXPECT_EQ(cmp::cmp(1, 2), cmp::Less);
    EXPECT_EQ(cmp::cmp(2, 2), cmp::Equal);
    EXPECT_EQ(cmp::cmp(3, 2), cmp::Greater);
}

TEST(CompareTest, EnumValues)
{
    using cmp = castle::compare<int>;
    EXPECT_EQ(static_cast<int>(cmp::Less), -1);
    EXPECT_EQ(static_cast<int>(cmp::Equal), 0);
    EXPECT_EQ(static_cast<int>(cmp::Greater), 1);
}

TEST(CompareTest, CompareWithGreaterReversesOrdering)
{
    using cmp = castle::compare<int, castle::greater<int>>;
    EXPECT_TRUE(cmp::lt(2, 1));
    EXPECT_TRUE(cmp::gt(1, 2));
    EXPECT_TRUE(cmp::eq(2, 2));
}

TEST(CompareTest, ThreeWayFreeFunction)
{
    EXPECT_EQ(castle::cmp_3_ways<int>(1, 2), -1);
    EXPECT_EQ(castle::cmp_3_ways<int>(2, 2), 0);
    EXPECT_EQ(castle::cmp_3_ways<int>(3, 2), 1);
}

TEST(CompareTest, ThreeWayIsConstexpr)
{
    static_assert(castle::cmp_3_ways<int>(1, 2) == -1, "cmp_3_ways must be constexpr");
    static_assert(castle::cmp_3_ways<int>(5, 5) == 0, "cmp_3_ways must be constexpr");
}

} // namespace
