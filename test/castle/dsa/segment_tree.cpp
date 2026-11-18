#include <gtest/gtest.h>

#include "castle/dsa/segment_tree.hpp"

#include <stdint.h>

namespace
{
using castle::size_type;
using castle::status;
using castle::dsa::lazy_segment_tree;
using castle::dsa::segment_tree;

uint32_t next_random(uint32_t& state)
{
    state = state * 1664525U + 1013904223U;
    return state >> 8U;
}

int32_t random_value(uint32_t& state)
{
    return static_cast<int32_t>(next_random(state) % 41U) - 20;
}

// Non-commutative monoid: affine maps x -> mul * x + add, combined as "left, then right".
struct affine
{
    int64_t mul;
    int64_t add;
};

bool operator==(const affine& lhs, const affine& rhs)
{
    return lhs.mul == rhs.mul && lhs.add == rhs.add;
}

struct affine_monoid
{
    using value_type = affine;

    static affine identity()
    {
        return affine{1, 0};
    }

    static affine combine(const affine& first, const affine& then)
    {
        return affine{(first.mul * then.mul) % 1000003, (first.add * then.mul + then.add) % 1000003};
    }
};

TEST(SegmentTreeTest, MonoidsAndTags)
{
    EXPECT_EQ(castle::dsa::sum_monoid<int32_t>::identity(), 0);
    EXPECT_EQ(castle::dsa::sum_monoid<int32_t>::combine(2, 3), 5);
    EXPECT_EQ(castle::dsa::min_monoid<int16_t>::identity(), 32767);
    EXPECT_EQ(castle::dsa::min_monoid<int16_t>::combine(4, -1), -1);
    EXPECT_EQ(castle::dsa::min_monoid<int16_t>::combine(-1, 4), -1);
    EXPECT_EQ(castle::dsa::max_monoid<int16_t>::identity(), -32768);
    EXPECT_EQ(castle::dsa::max_monoid<int16_t>::combine(4, -1), 4);
    EXPECT_EQ(castle::dsa::max_monoid<int16_t>::combine(-1, 4), 4);
    EXPECT_DOUBLE_EQ(castle::dsa::max_monoid<double>::identity(), -castle::numeric_limits<double>::max());

    const castle::dsa::assign_tag<int32_t> inactive;
    const castle::dsa::assign_tag<int32_t> active = 7;
    EXPECT_FALSE(inactive.active);
    EXPECT_TRUE(active.active);
    EXPECT_EQ(active.value, 7);

    static_assert(segment_tree<castle::dsa::sum_monoid<int32_t>, 5U>::static_leaf_capacity == 8U, "rounded up");
    static_assert(segment_tree<castle::dsa::sum_monoid<int32_t>, 8U>::static_leaf_capacity == 8U, "exact");
    static_assert(lazy_segment_tree<castle::dsa::range_add_sum<int32_t>, 9U>::static_leaf_capacity == 16U, "rounded up");
}

TEST(SegmentTreeTest, LeetCodeRangeSumQueryMutable)
{
    const int32_t nums[] = {1, 3, 5};
    segment_tree<castle::dsa::sum_monoid<int32_t>, 8U> tree;
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.capacity(), 8U);
    EXPECT_EQ(tree.all(), 0);
    ASSERT_EQ(tree.build(nums, 3U), status::ok);
    EXPECT_EQ(tree.size(), 3U);
    EXPECT_EQ(tree.query(0U, 2U), 9);
    EXPECT_EQ(tree.set(1U, 2), status::ok);
    EXPECT_EQ(tree.query(0U, 2U), 8);
    EXPECT_EQ(tree.all(), 8);
    EXPECT_EQ(tree.get(1U), 2);
}

TEST(SegmentTreeTest, RejectsInvalidArguments)
{
    const int32_t nums[] = {4, 1, 7};
    segment_tree<castle::dsa::min_monoid<int32_t>, 4U> tree;
    EXPECT_EQ(tree.build(nums, 5U), status::out_of_range);
    EXPECT_EQ(tree.build(nullptr, 1U), status::invalid_argument);
    EXPECT_EQ(tree.assign(5U, 0), status::out_of_range);
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.build(nullptr, 0U), status::ok);
    EXPECT_EQ(tree.all(), castle::dsa::min_monoid<int32_t>::identity());

    ASSERT_EQ(tree.build(nums, 3U), status::ok);
    const int32_t none = castle::dsa::min_monoid<int32_t>::identity();
    EXPECT_EQ(tree.set(3U, 0), status::out_of_range);
    EXPECT_EQ(tree.get(3U), none);
    EXPECT_EQ(tree.query(2U, 1U), none);
    EXPECT_EQ(tree.query(0U, 3U), none);
    EXPECT_EQ(tree.max_right(4U, [](int32_t) { return true; }), tree.npos);
    EXPECT_EQ(tree.min_left(4U, [](int32_t) { return true; }), tree.npos);
    EXPECT_EQ(tree.all(), 1);
}

TEST(SegmentTreeTest, AssignFillsAndSingleElementTree)
{
    segment_tree<castle::dsa::max_monoid<int32_t>, 6U> tree;
    ASSERT_EQ(tree.assign(1U, 5), status::ok);
    EXPECT_EQ(tree.all(), 5);
    EXPECT_EQ(tree.query(0U, 0U), 5);
    EXPECT_EQ(tree.set(0U, -3), status::ok);
    EXPECT_EQ(tree.all(), -3);

    ASSERT_EQ(tree.assign(6U), status::ok); // identity fill
    EXPECT_EQ(tree.all(), castle::dsa::max_monoid<int32_t>::identity());
    EXPECT_EQ(tree.set(4U, 9), status::ok);
    EXPECT_EQ(tree.query(0U, 5U), 9);
    EXPECT_EQ(tree.query(0U, 3U), castle::dsa::max_monoid<int32_t>::identity());

    tree.clear();
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.all(), castle::dsa::max_monoid<int32_t>::identity());
}

template <typename Monoid>
void check_random_point_tree(uint32_t seed)
{
    uint32_t state = seed;
    for (size_type size = 1U; size <= 19U; ++size)
    {
        int32_t reference[19] = {};
        for (size_type i = 0U; i < size; ++i)
        {
            reference[i] = random_value(state);
        }
        segment_tree<Monoid, 19U> tree;
        ASSERT_EQ(tree.build(reference, size), status::ok);
        for (size_type step = 0U; step < 40U; ++step)
        {
            const size_type index = next_random(state) % size;
            reference[index] = random_value(state);
            ASSERT_EQ(tree.set(index, reference[index]), status::ok);

            for (size_type left = 0U; left < size; ++left)
            {
                int32_t expected = Monoid::identity();
                for (size_type right = left; right < size; ++right)
                {
                    expected = Monoid::combine(expected, reference[right]);
                    ASSERT_EQ(tree.query(left, right), expected);
                }
            }
            ASSERT_EQ(tree.get(index), reference[index]);
            ASSERT_EQ(tree.all(), tree.query(0U, size - 1U));
        }
    }
}

TEST(SegmentTreeTest, RandomPointUpdatesMatchBruteForce)
{
    check_random_point_tree<castle::dsa::sum_monoid<int32_t>>(1U);
    check_random_point_tree<castle::dsa::min_monoid<int32_t>>(2U);
    check_random_point_tree<castle::dsa::max_monoid<int32_t>>(3U);
}

TEST(SegmentTreeTest, NonCommutativeMonoidKeepsOrder)
{
    uint32_t state = 5U;
    affine maps[13] = {};
    for (size_type i = 0U; i < 13U; ++i)
    {
        maps[i] = affine{static_cast<int64_t>(next_random(state) % 7U) + 1, static_cast<int64_t>(next_random(state) % 10U)};
    }
    segment_tree<affine_monoid, 13U> tree;
    ASSERT_EQ(tree.build(maps, 13U), status::ok);
    for (size_type left = 0U; left < 13U; ++left)
    {
        affine expected = affine_monoid::identity();
        for (size_type right = left; right < 13U; ++right)
        {
            expected = affine_monoid::combine(expected, maps[right]);
            ASSERT_TRUE(tree.query(left, right) == expected);
        }
    }
}

template <typename Tree>
void check_binary_searches(Tree& tree, const int32_t* reference, size_type size)
{
    for (int32_t limit = 0; limit <= 60; limit += 3) // limit >= 0, so pred(identity) holds
    {
        const auto within = [limit](int32_t sum) { return sum <= limit; };
        for (size_type left = 0U; left <= size; ++left)
        {
            size_type expected = left;
            int32_t sum = 0;
            while ((expected < size) && (sum + reference[expected] <= limit))
            {
                sum += reference[expected];
                ++expected;
            }
            ASSERT_EQ(tree.max_right(left, within), expected) << "left " << left << " limit " << limit;
        }
        for (size_type end = 0U; end <= size; ++end)
        {
            size_type expected = end;
            int32_t sum = 0;
            while ((expected > 0U) && (sum + reference[expected - 1U] <= limit))
            {
                sum += reference[expected - 1U];
                --expected;
            }
            ASSERT_EQ(tree.min_left(end, within), expected) << "end " << end << " limit " << limit;
        }
    }
}

TEST(SegmentTreeTest, MaxRightAndMinLeftMatchLinearScan)
{
    uint32_t state = 11U;
    for (size_type size = 0U; size <= 17U; ++size)
    {
        int32_t reference[17] = {};
        for (size_type i = 0U; i < size; ++i)
        {
            reference[i] = static_cast<int32_t>(next_random(state) % 9U); // non-negative: monotone prefixes
        }
        segment_tree<castle::dsa::sum_monoid<int32_t>, 17U> tree;
        ASSERT_EQ(tree.build(reference, size), status::ok);
        check_binary_searches(tree, reference, size);

        lazy_segment_tree<castle::dsa::range_add_sum<int32_t>, 17U> lazy;
        ASSERT_EQ(lazy.build(reference, size), status::ok);
        if (size > 2U)
        {
            // Pending tags must be pushed while descending.
            ASSERT_EQ(lazy.apply(1U, size - 2U, 2), status::ok);
            for (size_type i = 1U; i + 1U < size; ++i)
            {
                reference[i] += 2;
            }
        }
        check_binary_searches(lazy, reference, size);
    }
}

TEST(SegmentTreeTest, LazyRejectsInvalidArguments)
{
    const int32_t values[] = {1, 2, 3};
    lazy_segment_tree<castle::dsa::range_add_min<int32_t>, 4U> tree;
    const int32_t none = castle::dsa::min_monoid<int32_t>::identity();
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.capacity(), 4U);
    EXPECT_EQ(tree.all(), none);
    EXPECT_EQ(tree.build(values, 5U), status::out_of_range);
    EXPECT_EQ(tree.build(nullptr, 2U), status::invalid_argument);
    EXPECT_EQ(tree.assign(5U, 0), status::out_of_range);
    EXPECT_EQ(tree.build(nullptr, 0U), status::ok);

    ASSERT_EQ(tree.build(values, 3U), status::ok);
    EXPECT_EQ(tree.size(), 3U);
    EXPECT_EQ(tree.apply(2U, 1U, 5), status::out_of_range);
    EXPECT_EQ(tree.apply(0U, 3U, 5), status::out_of_range);
    EXPECT_EQ(tree.set(3U, 0), status::out_of_range);
    EXPECT_EQ(tree.get(3U), none);
    EXPECT_EQ(tree.query(2U, 1U), none);
    EXPECT_EQ(tree.query(1U, 3U), none);
    EXPECT_EQ(tree.max_right(4U, [](int32_t) { return true; }), tree.npos);
    EXPECT_EQ(tree.min_left(4U, [](int32_t) { return true; }), tree.npos);
    EXPECT_EQ(tree.all(), 1);

    tree.clear();
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.all(), none);
}

TEST(SegmentTreeTest, LazySingleElementAndDefaultAssign)
{
    lazy_segment_tree<castle::dsa::range_add_sum<int32_t>, 1U> one;
    ASSERT_EQ(one.assign(1U, 4), status::ok);
    EXPECT_EQ(one.apply(0U, 0U, 3), status::ok);
    EXPECT_EQ(one.get(0U), 7);
    EXPECT_EQ(one.set(0U, 1), status::ok);
    EXPECT_EQ(one.query(0U, 0U), 1);
    EXPECT_EQ(one.all(), 1);

    lazy_segment_tree<castle::dsa::range_assign_max<int32_t>, 4U> peaks;
    ASSERT_EQ(peaks.assign(4U), status::ok); // identity fill
    EXPECT_EQ(peaks.all(), castle::dsa::max_monoid<int32_t>::identity());
    EXPECT_EQ(peaks.apply(1U, 2U, 6), status::ok);
    EXPECT_EQ(peaks.all(), 6);
}

template <typename Policy>
void check_random_lazy_tree(uint32_t seed, bool assigns)
{
    using monoid = Policy;
    uint32_t state = seed;
    for (size_type size = 1U; size <= 19U; ++size)
    {
        int32_t reference[19] = {};
        for (size_type i = 0U; i < size; ++i)
        {
            reference[i] = random_value(state);
        }
        lazy_segment_tree<Policy, 19U> tree;
        ASSERT_EQ(tree.build(reference, size), status::ok);

        for (size_type step = 0U; step < 60U; ++step)
        {
            const size_type left = next_random(state) % size;
            const size_type right = left + next_random(state) % (size - left);
            const int32_t value = random_value(state);
            switch (next_random(state) % 4U)
            {
            case 0U:
                ASSERT_EQ(tree.apply(left, right, value), status::ok);
                for (size_type i = left; i <= right; ++i)
                {
                    reference[i] = assigns ? value : reference[i] + value;
                }
                break;
            case 1U:
                ASSERT_EQ(tree.set(left, value), status::ok);
                reference[left] = value;
                break;
            case 2U:
                ASSERT_EQ(tree.get(right), reference[right]);
                break;
            default:
            {
                int32_t expected = monoid::identity();
                for (size_type i = left; i <= right; ++i)
                {
                    expected = monoid::combine(expected, reference[i]);
                }
                ASSERT_EQ(tree.query(left, right), expected);
                break;
            }
            }
        }
        int32_t total = monoid::identity();
        for (size_type i = 0U; i < size; ++i)
        {
            total = monoid::combine(total, reference[i]);
            ASSERT_EQ(tree.get(i), reference[i]);
        }
        ASSERT_EQ(tree.all(), total);
    }
}

TEST(SegmentTreeTest, LazyRangeUpdatesMatchBruteForce)
{
    check_random_lazy_tree<castle::dsa::range_add_sum<int32_t>>(21U, false);
    check_random_lazy_tree<castle::dsa::range_add_min<int32_t>>(22U, false);
    check_random_lazy_tree<castle::dsa::range_add_max<int32_t>>(23U, false);
    check_random_lazy_tree<castle::dsa::range_assign_sum<int32_t>>(24U, true);
    check_random_lazy_tree<castle::dsa::range_assign_min<int32_t>>(25U, true);
    check_random_lazy_tree<castle::dsa::range_assign_max<int32_t>>(26U, true);
}

TEST(SegmentTreeTest, LazyComposesPendingTagsInOrder)
{
    lazy_segment_tree<castle::dsa::range_assign_sum<int32_t>, 8U> tree;
    ASSERT_EQ(tree.assign(8U, 1), status::ok);
    ASSERT_EQ(tree.apply(0U, 7U, 3), status::ok); // pending at the root
    ASSERT_EQ(tree.apply(0U, 7U, 5), status::ok); // newer assignment wins
    EXPECT_EQ(tree.query(2U, 5U), 20);
    ASSERT_EQ(tree.apply(4U, 7U, 0), status::ok);
    EXPECT_EQ(tree.all(), 20);
    EXPECT_EQ(tree.get(3U), 5);
    EXPECT_EQ(tree.get(4U), 0);
}
} // namespace
