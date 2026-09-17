#include <gtest/gtest.h>
#include "castle/container/array_view.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/container/vector.hpp"
#include "castle/iterator/operations.hpp"

namespace
{
uint32_t sum(castle::container::initializer_list<uint32_t> list)
{
    uint32_t total = 0U;
    for (uint32_t value : list)
    {
        total += value;
    }
    return total;
}

TEST(InitializerListTest, BasicAccessAndFreeFunctions)
{
    castle::container::initializer_list<int> list = {1, 2, 3, 4};
    EXPECT_EQ(list.size(), 4U);
    EXPECT_EQ(castle::container::size(list), 4U);
    EXPECT_FALSE(castle::container::empty(list));
    EXPECT_EQ(castle::container::front(list), 1);
    EXPECT_EQ(castle::container::back(list), 4);
    EXPECT_EQ(castle::container::begin(list), list.begin());
    EXPECT_EQ(castle::container::end(list), list.end());

    castle::container::initializer_list<int> empty_list;
    EXPECT_TRUE(castle::container::empty(empty_list));
    EXPECT_EQ(castle::container::size(empty_list), 0U);
}

TEST(InitializerListTest, WorksAsFunctionParameter)
{
    EXPECT_EQ(sum({1U, 2U, 3U, 4U}), 10U);
    EXPECT_EQ(sum({}), 0U);
}

TEST(InitializerListTest, IteratesLikeAnyCastleIterator)
{
    castle::container::initializer_list<int> list = {10, 20, 30};
    EXPECT_EQ(castle::distance(list.begin(), list.end()), 3);
    EXPECT_EQ(*castle::next(list.begin()), 20);
    EXPECT_EQ(*castle::prev(list.end()), 30);
}

TEST(InitializerListTest, ReverseIteration)
{
    castle::container::initializer_list<int> list = {1, 2, 3};
    auto it = castle::container::rbegin(list);
    CASTLE_CONST auto stop = castle::container::rend(list);
    int collected = 0;
    for (; it != stop; ++it)
    {
        collected = collected * 10 + *it;
    }
    EXPECT_EQ(collected, 321);
}

TEST(InitializerListTest, CollaboratesWithVector)
{
    castle::container::vector<int, 4> v = {1, 2, 3};
    EXPECT_EQ(v.size(), 3U);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v.back(), 3);

    v = {9, 8};
    EXPECT_EQ(v.size(), 2U);
    EXPECT_EQ(v[0], 9);
    EXPECT_EQ(v[1], 8);

    castle::container::vector<int, 3> full = {1, 2, 3};
    EXPECT_TRUE(full.full());
}

TEST(InitializerListTest, CollaboratesWithArrayView)
{
    castle::container::initializer_list<int> list = {5, 6, 7};
    auto view = castle::container::make_array_view(list);
    EXPECT_EQ(view.size(), 3U);
    EXPECT_EQ(view.front(), 5);
    EXPECT_EQ(view.back(), 7);
}
}
