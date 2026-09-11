#include <gtest/gtest.h>

#include "castle/iterator/circular_iterator.h"

#include <type_traits>

namespace
{

using circ = castle::circular_iterator<int*>;

TEST(CircularIteratorTest, MemberTypedefs)
{
    static_assert(std::is_same<circ::iterator_type, int*>::value, "iterator_type");
    static_assert(std::is_same<circ::value_type, int>::value, "value_type");
    static_assert(std::is_same<circ::pointer, int*>::value, "pointer");
    static_assert(std::is_same<circ::reference, int&>::value, "reference");
    static_assert(std::is_same<circ::iterator_category,
                               castle::random_access_iterator_tag>::value,
                  "category");
    SUCCEED();
}

TEST(CircularIteratorTest, BaseAndDereference)
{
    int data[3] = {7, 8, 9};
    circ it(data, data + 3, data + 1);
    EXPECT_EQ(it.base(), data + 1);
    EXPECT_EQ(*it, 8);
}

TEST(CircularIteratorTest, ArrowOperator)
{
    struct Point { int x; };
    Point pts[2] = {{1}, {2}};
    castle::circular_iterator<Point*> it(pts, pts + 2, pts);
    EXPECT_EQ(it->x, 1);
    ++it;
    EXPECT_EQ(it->x, 2);
}

TEST(CircularIteratorTest, PreIncrementWrapsAtEnd)
{
    int data[3] = {1, 2, 3};
    circ it(data, data + 3, data);
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it; // wraps back to first
    EXPECT_EQ(*it, 1);
    EXPECT_EQ(it.base(), data);
}

TEST(CircularIteratorTest, PostIncrementReturnsPreWrapValue)
{
    int data[2] = {5, 6};
    circ it(data, data + 2, data + 1); // *it == 6
    circ old = it++;
    EXPECT_EQ(*old, 6);
    EXPECT_EQ(*it, 5); // wrapped
    EXPECT_EQ(it.base(), data);
}

TEST(CircularIteratorTest, PreDecrementWrapsAtBegin)
{
    int data[3] = {1, 2, 3};
    circ it(data, data + 3, data); // at first
    --it; // wraps to last element
    EXPECT_EQ(*it, 3);
    EXPECT_EQ(it.base(), data + 2);
    --it;
    EXPECT_EQ(*it, 2);
    --it;
    EXPECT_EQ(*it, 1);
}

TEST(CircularIteratorTest, PostDecrementReturnsPreWrapValue)
{
    int data[3] = {1, 2, 3};
    circ it(data, data + 3, data); // *it == 1
    circ old = it--;
    EXPECT_EQ(*old, 1);
    EXPECT_EQ(*it, 3); // wrapped to last element
}

TEST(CircularIteratorTest, FullCycleReturnsToStart)
{
    int data[4] = {10, 20, 30, 40};
    circ it(data, data + 4, data);
    for (int i = 0; i < 4; ++i)
    {
        ++it;
    }
    EXPECT_EQ(it.base(), data);
    EXPECT_EQ(*it, 10);
}

TEST(CircularIteratorTest, IncrementDecrementRoundTrip)
{
    int data[3] = {1, 2, 3};
    circ it(data, data + 3, data + 1);
    ++it;
    ++it; // wrapped once: at data (value 1)
    EXPECT_EQ(*it, 1);
    --it;
    --it; // back to data + 1
    EXPECT_EQ(it.base(), data + 1);
    EXPECT_EQ(*it, 2);
}

TEST(CircularIteratorTest, EqualityComparesCurrentPosition)
{
    int data[3] = {1, 2, 3};
    circ a(data, data + 3, data + 1);
    circ b(data, data + 3, data + 1);
    circ c(data, data + 3, data + 2);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
}

TEST(CircularIteratorTest, SingleElementRangeStaysInPlace)
{
    int data[1] = {42};
    circ it(data, data + 1, data);
    ++it; // increments then wraps immediately back to first
    EXPECT_EQ(it.base(), data);
    EXPECT_EQ(*it, 42);
    --it;
    EXPECT_EQ(it.base(), data);
    EXPECT_EQ(*it, 42);
}

TEST(CircularIteratorTest, EmptyRangeIncrementIsNoOp)
{
    int data[1] = {0};
    circ it(data, data, data); // first == last (empty)
    ++it;
    EXPECT_EQ(it.base(), data);
    --it;
    EXPECT_EQ(it.base(), data);
}

} // namespace
