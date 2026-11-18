#include <gtest/gtest.h>

#include "castle/dsa/fenwick_tree.hpp"

#include <stdint.h>

namespace
{
using castle::size_type;
using castle::status;
using castle::dsa::fenwick_tree;

uint32_t next_random(uint32_t& state)
{
    state = state * 1664525U + 1013904223U;
    return state >> 8U;
}

TEST(FenwickTreeTest, ConstructionAndCapacity)
{
    static_assert(fenwick_tree<int32_t, 7U>::static_capacity == 7U, "static capacity");
    static_assert(sizeof(fenwick_tree<int32_t, 8U>) == 8U * sizeof(int32_t) + sizeof(size_type), "one value per element");

    fenwick_tree<int32_t, 8U> empty_tree;
    EXPECT_TRUE(empty_tree.empty());
    EXPECT_EQ(empty_tree.size(), 0U);
    EXPECT_EQ(empty_tree.capacity(), 8U);
    EXPECT_EQ(empty_tree.prefix_sum(0U), 0);

    fenwick_tree<int32_t, 8U> zeros(5U);
    EXPECT_EQ(zeros.size(), 5U);
    EXPECT_FALSE(zeros.empty());
    EXPECT_EQ(zeros.prefix_sum(5U), 0);
}

TEST(FenwickTreeTest, AssignZeroesAndRejectsOversize)
{
    const int32_t values[] = {4, 5, 6};
    fenwick_tree<int32_t, 4U> tree;
    ASSERT_EQ(tree.build(values, 3U), status::ok);

    EXPECT_EQ(tree.assign(5U), status::out_of_range);
    EXPECT_EQ(tree.size(), 3U);
    EXPECT_EQ(tree.prefix_sum(3U), 15);

    EXPECT_EQ(tree.assign(4U), status::ok);
    EXPECT_EQ(tree.size(), 4U);
    EXPECT_EQ(tree.prefix_sum(4U), 0);
    EXPECT_EQ(tree.add(3U, 2), status::ok);
    EXPECT_EQ(tree.range_sum(3U, 3U), 2);
}

TEST(FenwickTreeTest, BuildValidatesInput)
{
    const int32_t values[] = {1, 2, 3, 4, 5};
    fenwick_tree<int32_t, 4U> tree;
    EXPECT_EQ(tree.build(values, 5U), status::out_of_range);
    EXPECT_EQ(tree.build(nullptr, 2U), status::invalid_argument);
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.build(nullptr, 0U), status::ok);
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.build(values, 4U), status::ok);
    EXPECT_EQ(tree.prefix_sum(4U), 10);
}

TEST(FenwickTreeTest, RandomOperationsMatchPlainArray)
{
    uint32_t state = 7U;
    for (size_type size = 1U; size <= 17U; ++size)
    {
        int64_t reference[17] = {};
        for (size_type i = 0U; i < size; ++i)
        {
            reference[i] = static_cast<int64_t>(next_random(state) % 201U) - 100;
        }
        fenwick_tree<int64_t, 17U> tree;
        ASSERT_EQ(tree.build(reference, size), status::ok);

        for (size_type step = 0U; step < 60U; ++step)
        {
            const size_type index = next_random(state) % size;
            const int64_t value = static_cast<int64_t>(next_random(state) % 201U) - 100;
            if ((step % 2U) == 0U)
            {
                ASSERT_EQ(tree.add(index, value), status::ok);
                reference[index] += value;
            }
            else
            {
                ASSERT_EQ(tree.set(index, value), status::ok);
                reference[index] = value;
            }

            int64_t prefix = 0;
            ASSERT_EQ(tree.prefix_sum(0U), 0);
            for (size_type count = 1U; count <= size; ++count)
            {
                prefix += reference[count - 1U];
                ASSERT_EQ(tree.prefix_sum(count), prefix);
                ASSERT_EQ(tree.get(count - 1U), reference[count - 1U]);
            }
            const size_type left = next_random(state) % size;
            const size_type right = left + next_random(state) % (size - left);
            int64_t expected = 0;
            for (size_type i = left; i <= right; ++i)
            {
                expected += reference[i];
            }
            ASSERT_EQ(tree.range_sum(left, right), expected);
        }
    }
}

TEST(FenwickTreeTest, OutOfRangeArgumentsAreRejected)
{
    const int32_t values[] = {3, 1, 4};
    fenwick_tree<int32_t, 8U> tree;
    ASSERT_EQ(tree.build(values, 3U), status::ok);

    EXPECT_EQ(tree.add(3U, 1), status::out_of_range);
    EXPECT_EQ(tree.set(3U, 1), status::out_of_range);
    EXPECT_EQ(tree.get(3U), 0);
    EXPECT_EQ(tree.prefix_sum(4U), 0);
    EXPECT_EQ(tree.range_sum(2U, 1U), 0);
    EXPECT_EQ(tree.range_sum(1U, 3U), 0);
    EXPECT_EQ(tree.prefix_sum(3U), 8); // unchanged
}

TEST(FenwickTreeTest, LowerBoundFindsFirstPrefixReachingTarget)
{
    using tree_type = fenwick_tree<uint32_t, 16U>;
    uint32_t state = 99U;
    for (size_type size = 1U; size <= 16U; ++size)
    {
        uint32_t values[16] = {};
        uint32_t total = 0U;
        for (size_type i = 0U; i < size; ++i)
        {
            values[i] = next_random(state) % 4U; // zeros included
            total += values[i];
        }
        tree_type tree;
        ASSERT_EQ(tree.build(values, size), status::ok);
        for (uint32_t target = 1U; target <= total + 1U; ++target)
        {
            size_type expected = tree_type::npos;
            uint32_t prefix = 0U;
            for (size_type i = 0U; i < size; ++i)
            {
                prefix += values[i];
                if (prefix >= target)
                {
                    expected = i;
                    break;
                }
            }
            ASSERT_EQ(tree.lower_bound(target), expected) << "size " << size << " target " << target;
        }
        EXPECT_EQ(tree.lower_bound(0U), 0U);
    }

    tree_type empty_tree;
    EXPECT_EQ(empty_tree.lower_bound(0U), tree_type::npos);
    EXPECT_EQ(empty_tree.lower_bound(1U), tree_type::npos);
}

TEST(FenwickTreeTest, ClearAndFloatingPointValues)
{
    const double values[] = {0.5, 1.25, 2.0};
    fenwick_tree<double, 3U> tree;
    ASSERT_EQ(tree.build(values, 3U), status::ok);
    EXPECT_DOUBLE_EQ(tree.range_sum(1U, 2U), 3.25);
    EXPECT_EQ(tree.set(0U, 4.0), status::ok);
    EXPECT_DOUBLE_EQ(tree.prefix_sum(3U), 7.25);
    tree.clear();
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.add(0U, 1.0), status::out_of_range);
}
} // namespace
