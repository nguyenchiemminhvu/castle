#include <gtest/gtest.h>

#include "castle/iterator/fixed_iterator.h"

#include <type_traits>

namespace
{

using fixed = castle::fixed_iterator<int*>;

TEST(FixedIteratorTest, MemberTypedefs)
{
    static_assert(std::is_same<fixed::iterator_type, int*>::value, "iterator_type");
    static_assert(std::is_same<fixed::value_type, int>::value, "value_type");
    static_assert(std::is_same<fixed::pointer, int*>::value, "pointer");
    static_assert(std::is_same<fixed::reference, int&>::value, "reference");
    static_assert(std::is_same<fixed::iterator_category,
                               castle::random_access_iterator_tag>::value,
                  "category");
    SUCCEED();
}

TEST(FixedIteratorTest, BaseAndDereference)
{
    int data[3] = {7, 8, 9};
    // Constructor order is (first, current, last).
    fixed it(data, data + 1, data + 3);
    EXPECT_EQ(it.base(), data + 1);
    EXPECT_EQ(*it, 8);
}

TEST(FixedIteratorTest, ArrowOperator)
{
    struct Point { int x; };
    Point pts[2] = {{1}, {2}};
    castle::fixed_iterator<Point*> it(pts, pts, pts + 2);
    EXPECT_EQ(it->x, 1);
}

TEST(FixedIteratorTest, ValidReportsWhetherAtEnd)
{
    int data[2] = {1, 2};
    fixed it(data, data, data + 2);
    EXPECT_TRUE(it.valid());
    ++it;
    EXPECT_TRUE(it.valid());
    ++it; // now at last
    EXPECT_FALSE(it.valid());
}

TEST(FixedIteratorTest, IncrementStopsAtLast)
{
    int data[3] = {1, 2, 3};
    fixed it(data, data, data + 3);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it; // reaches last (past 3)
    EXPECT_EQ(it.base(), data + 3);
    ++it; // clamped: never goes beyond last
    EXPECT_EQ(it.base(), data + 3);
    EXPECT_FALSE(it.valid());
}

TEST(FixedIteratorTest, PostIncrementReturnsPreviousValue)
{
    int data[3] = {1, 2, 3};
    fixed it(data, data, data + 3);
    fixed old = it++;
    EXPECT_EQ(*old, 1);
    EXPECT_EQ(*it, 2);
}

TEST(FixedIteratorTest, DecrementStopsAtFirst)
{
    int data[3] = {1, 2, 3};
    fixed it(data, data + 2, data + 3); // *it == 3
    --it;
    EXPECT_EQ(*it, 2);
    --it;
    EXPECT_EQ(*it, 1);
    EXPECT_EQ(it.base(), data);
    --it; // clamped at first
    EXPECT_EQ(it.base(), data);
    EXPECT_EQ(*it, 1);
}

TEST(FixedIteratorTest, PostDecrementReturnsPreviousValue)
{
    int data[3] = {1, 2, 3};
    fixed it(data, data + 1, data + 3); // *it == 2
    fixed old = it--;
    EXPECT_EQ(*old, 2);
    EXPECT_EQ(*it, 1);
}

TEST(FixedIteratorTest, ClampedDecrementAtFirstIsNoOp)
{
    int data[3] = {1, 2, 3};
    fixed it(data, data, data + 3); // at first
    --it;
    EXPECT_EQ(it.base(), data);
    EXPECT_EQ(*it, 1);
}

TEST(FixedIteratorTest, ForwardTraversalOverRange)
{
    int data[4] = {10, 20, 30, 40};
    fixed it(data, data, data + 4);
    int sum = 0;
    while (it.valid())
    {
        sum += *it;
        ++it;
    }
    EXPECT_EQ(sum, 100);
}

TEST(FixedIteratorTest, EqualityComparesCurrentPosition)
{
    int data[3] = {1, 2, 3};
    fixed a(data, data + 1, data + 3);
    fixed b(data, data + 1, data + 3);
    fixed c(data, data + 2, data + 3);
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a != b);
}

TEST(FixedIteratorTest, EmptyRangeIsImmediatelyInvalid)
{
    int data[1] = {0};
    fixed it(data, data, data); // current == last
    EXPECT_FALSE(it.valid());
    ++it;
    EXPECT_EQ(it.base(), data);
}

} // namespace
