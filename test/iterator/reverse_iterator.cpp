#include <gtest/gtest.h>

#include "castle/iterator/reverse_iterator.h"

#include <type_traits>

namespace
{

using rev_iter = castle::reverse_iterator<int*>;

TEST(ReverseIteratorTest, MemberTypedefsMatchUnderlying)
{
    static_assert(std::is_same<rev_iter::iterator_type, int*>::value, "iterator_type");
    static_assert(std::is_same<rev_iter::value_type, int>::value, "value_type");
    static_assert(std::is_same<rev_iter::pointer, int*>::value, "pointer");
    static_assert(std::is_same<rev_iter::reference, int&>::value, "reference");
    static_assert(std::is_same<rev_iter::difference_type,
                               castle::difference_type>::value,
                  "difference_type");
    static_assert(std::is_same<rev_iter::iterator_category,
                               castle::random_access_iterator_tag>::value,
                  "category");
    SUCCEED();
}

TEST(ReverseIteratorTest, BaseAndDereferenceOffsetByOne)
{
    int data[4] = {10, 20, 30, 40};
    rev_iter it(data + 4); // base past-the-end
    EXPECT_EQ(it.base(), data + 4);
    EXPECT_EQ(*it, 40); // dereferences base - 1
}

TEST(ReverseIteratorTest, DefaultConstructIsConstexpr)
{
    constexpr rev_iter it{};
    (void)it;
    SUCCEED();
}

TEST(ReverseIteratorTest, PreIncrementMovesTowardFront)
{
    int data[4] = {10, 20, 30, 40};
    rev_iter it(data + 4);
    EXPECT_EQ(*it, 40);
    ++it;
    EXPECT_EQ(*it, 30);
    ++it;
    EXPECT_EQ(*it, 20);
}

TEST(ReverseIteratorTest, PostIncrementReturnsPreviousValue)
{
    int data[3] = {1, 2, 3};
    rev_iter it(data + 3);
    rev_iter old = it++;
    EXPECT_EQ(*old, 3);
    EXPECT_EQ(*it, 2);
}

TEST(ReverseIteratorTest, DecrementMovesTowardBack)
{
    int data[4] = {10, 20, 30, 40};
    rev_iter it(data + 2); // *it == 20
    EXPECT_EQ(*it, 20);
    --it;
    EXPECT_EQ(*it, 30);
    rev_iter old = it--;
    EXPECT_EQ(*old, 30);
    EXPECT_EQ(*it, 40);
}

TEST(ReverseIteratorTest, FullReverseTraversal)
{
    int data[5] = {1, 2, 3, 4, 5};
    rev_iter first(data + 5);
    rev_iter last(data);
    int expected = 5;
    for (rev_iter it = first; it != last; ++it)
    {
        EXPECT_EQ(*it, expected);
        --expected;
    }
    EXPECT_EQ(expected, 0);
}

TEST(ReverseIteratorTest, ArithmeticOperators)
{
    int data[6] = {0, 1, 2, 3, 4, 5};
    rev_iter it(data + 6); // *it == 5
    rev_iter plus = it + 2; // moves toward front -> *  == 3
    EXPECT_EQ(*plus, 3);
    rev_iter minus = plus - 1; // toward back -> * == 4
    EXPECT_EQ(*minus, 4);

    it += 3;
    EXPECT_EQ(*it, 2);
    it -= 2;
    EXPECT_EQ(*it, 4);
}

TEST(ReverseIteratorTest, FreeAddWithOffsetOnLeft)
{
    int data[4] = {10, 20, 30, 40};
    rev_iter it(data + 4);
    rev_iter shifted = castle::difference_type(2) + it;
    EXPECT_EQ(*shifted, 20);
}

TEST(ReverseIteratorTest, SubscriptOperator)
{
    int data[5] = {5, 6, 7, 8, 9};
    rev_iter it(data + 5); // it[0] == 9
    EXPECT_EQ(it[0], 9);
    EXPECT_EQ(it[1], 8);
    EXPECT_EQ(it[4], 5);
}

TEST(ReverseIteratorTest, DifferenceBetweenIterators)
{
    int data[6] = {0, 1, 2, 3, 4, 5};
    rev_iter first(data + 6);
    rev_iter last(data);
    // last.base() - first.base() == data - (data + 6) == -6
    EXPECT_EQ(last - first, 6);
    EXPECT_EQ(first - last, -6);
}

TEST(ReverseIteratorTest, ComparisonOperators)
{
    int data[5] = {0, 1, 2, 3, 4};
    rev_iter a(data + 5); // front-most (points at 4)
    rev_iter b(data + 3); // further along
    EXPECT_TRUE(a == a);
    EXPECT_TRUE(a != b);
    // a < b because b.base() < a.base().
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(b >= b);
    EXPECT_FALSE(b < a);
}

TEST(ReverseIteratorTest, ArrowOperator)
{
    struct Point { int x; int y; };
    Point pts[2] = {{1, 2}, {3, 4}};
    castle::reverse_iterator<Point*> it(pts + 2);
    EXPECT_EQ(it->x, 3);
    EXPECT_EQ(it->y, 4);
}

TEST(ReverseIteratorTest, ConvertingConstructorFromNonConst)
{
    int data[3] = {1, 2, 3};
    castle::reverse_iterator<int*> it(data + 3);
    castle::reverse_iterator<const int*> cit(it);
    EXPECT_EQ(cit.base(), data + 3);
    EXPECT_EQ(*cit, 3);
}

} // namespace
