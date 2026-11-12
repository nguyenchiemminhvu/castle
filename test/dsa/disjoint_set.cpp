#include <gtest/gtest.h>

#include "castle/dsa/disjoint_set.hpp"

#include <stdint.h>

namespace
{
using castle::dsa::disjoint_set;
using castle::size_type;

TEST(DisjointSetTest, IndexTypeIsSmallestFittingUnsigned)
{
    static_assert(sizeof(disjoint_set<255U>::index_type) == 1U, "255 nodes fit uint8_t");
    static_assert(sizeof(disjoint_set<256U>::index_type) == 2U, "256 nodes need uint16_t");
    static_assert(sizeof(disjoint_set<65536U>::index_type) == 4U, "65536 nodes need uint32_t");
    static_assert(disjoint_set<12U>::static_capacity == 12U, "static capacity");
    EXPECT_LE(sizeof(disjoint_set<16U>), 3U * 16U + 2U * sizeof(size_type));
}

TEST(DisjointSetTest, DefaultConstructionMakesCapacitySingletons)
{
    disjoint_set<5U> set;
    EXPECT_EQ(set.size(), 5U);
    EXPECT_EQ(set.capacity(), 5U);
    EXPECT_FALSE(set.empty());
    EXPECT_EQ(set.group_count(), 5U);
    for (size_type i = 0U; i < 5U; ++i)
    {
        EXPECT_EQ(set.find(i), i);
        EXPECT_EQ(set.group_size(i), 1U);
    }
    EXPECT_FALSE(set.same_set(0U, 4U));
    EXPECT_TRUE(set.same_set(3U, 3U));
}

TEST(DisjointSetTest, ExplicitNodeCountAndAssign)
{
    disjoint_set<8U> set(3U);
    EXPECT_EQ(set.size(), 3U);
    EXPECT_EQ(set.capacity(), 8U);
    EXPECT_EQ(set.group_count(), 3U);

    EXPECT_TRUE(set.unite(0U, 1U));
    EXPECT_EQ(set.assign(6U), castle::status::ok);
    EXPECT_EQ(set.size(), 6U);
    EXPECT_EQ(set.group_count(), 6U);
    EXPECT_FALSE(set.same_set(0U, 1U));

    EXPECT_TRUE(set.unite(4U, 5U));
    EXPECT_EQ(set.assign(9U), castle::status::out_of_range);
    EXPECT_EQ(set.size(), 6U);
    EXPECT_EQ(set.group_count(), 5U);
    EXPECT_TRUE(set.same_set(4U, 5U));

    EXPECT_EQ(set.assign(0U), castle::status::ok);
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.group_count(), 0U);
    EXPECT_TRUE(set.groups().empty());
}

TEST(DisjointSetTest, UniteMergesAndReportsAlreadyJoined)
{
    disjoint_set<6U> set;
    EXPECT_TRUE(set.unite(0U, 1U));
    EXPECT_TRUE(set.unite(2U, 1U));
    EXPECT_FALSE(set.unite(0U, 2U));
    EXPECT_FALSE(set.unite(3U, 3U));

    EXPECT_EQ(set.group_count(), 4U);
    EXPECT_TRUE(set.same_set(0U, 2U));
    EXPECT_FALSE(set.same_set(0U, 3U));
    EXPECT_EQ(set.group_size(2U), 3U);
    EXPECT_EQ(set.group_size(5U), 1U);
    EXPECT_EQ(set.find(0U), set.find(2U));
}

TEST(DisjointSetTest, UnionByRankKeepsTallerTreeAsRoot)
{
    disjoint_set<8U> set;
    // Equal ranks: the left root wins and its rank grows to 1.
    EXPECT_TRUE(set.unite(0U, 1U));
    EXPECT_EQ(set.find(1U), 0U);

    // Rank 0 vs rank 1: the rank-1 root stays root even when passed second.
    EXPECT_TRUE(set.unite(2U, 0U));
    EXPECT_EQ(set.find(2U), 0U);

    // Two rank-1 trees: the left root wins and reaches rank 2.
    EXPECT_TRUE(set.unite(4U, 5U));
    EXPECT_TRUE(set.unite(4U, 0U));
    EXPECT_EQ(set.find(0U), 4U);

    // A fresh rank-2 tree beside it: 6 alone (rank 0) must hang below 4.
    EXPECT_TRUE(set.unite(6U, 1U));
    EXPECT_EQ(set.find(6U), 4U);
    EXPECT_EQ(set.group_size(6U), 6U);
}

TEST(DisjointSetTest, FindCompressesDeepPaths)
{
    disjoint_set<4U> set;
    EXPECT_TRUE(set.unite(0U, 1U));
    EXPECT_TRUE(set.unite(2U, 3U));
    EXPECT_TRUE(set.unite(0U, 2U)); // 3 -> 2 -> 0
    EXPECT_EQ(set.find(3U), 0U);
    EXPECT_EQ(set.find(3U), 0U);
    EXPECT_EQ(set.group_size(3U), 4U);
    EXPECT_EQ(set.group_count(), 1U);
}

TEST(DisjointSetTest, ResetSingletonClearsRank)
{
    disjoint_set<3U> set;
    EXPECT_TRUE(set.unite(0U, 1U)); // root 0, rank 1
    set.reset(1U);                  // root 0 keeps rank 1, size 1
    EXPECT_EQ(set.group_count(), 3U);
    set.reset(0U);                  // already singleton: rank must drop to 0
    EXPECT_EQ(set.group_count(), 3U);
    EXPECT_EQ(set.group_size(0U), 1U);

    // With rank 0 on both sides the left root wins; a stale rank 1 on node 0 would win instead.
    EXPECT_TRUE(set.unite(2U, 0U));
    EXPECT_EQ(set.find(0U), 2U);
}

TEST(DisjointSetTest, ResetInnerNodeRelinksItsChildrenToRoot)
{
    disjoint_set<5U> set;
    EXPECT_TRUE(set.unite(0U, 1U));
    EXPECT_TRUE(set.unite(2U, 3U));
    EXPECT_TRUE(set.unite(0U, 2U)); // 3 -> 2 -> 0, 1 -> 0
    EXPECT_EQ(set.group_count(), 2U);

    set.reset(2U);
    EXPECT_EQ(set.group_count(), 3U);
    EXPECT_EQ(set.find(2U), 2U);
    EXPECT_EQ(set.group_size(2U), 1U);
    EXPECT_TRUE(set.same_set(3U, 0U));
    EXPECT_TRUE(set.same_set(1U, 0U));
    EXPECT_FALSE(set.same_set(2U, 3U));
    EXPECT_EQ(set.group_size(0U), 3U);
    EXPECT_EQ(set.find(3U), 0U);
}

TEST(DisjointSetTest, ResetRootPromotesFirstChildAndKeepsRank)
{
    disjoint_set<5U> set;
    EXPECT_TRUE(set.unite(0U, 1U));
    EXPECT_TRUE(set.unite(0U, 2U)); // 1 -> 0, 2 -> 0, root 0 rank 1
    set.reset(0U);

    EXPECT_EQ(set.group_count(), 4U);
    EXPECT_EQ(set.find(0U), 0U);
    EXPECT_EQ(set.group_size(0U), 1U);
    EXPECT_EQ(set.find(1U), 1U);
    EXPECT_EQ(set.find(2U), 1U);
    EXPECT_EQ(set.group_size(2U), 2U);

    // The heir inherited rank 1, so a rank-0 singleton passed first still hangs below it.
    EXPECT_TRUE(set.unite(3U, 2U));
    EXPECT_EQ(set.find(3U), 1U);
    EXPECT_EQ(set.group_size(3U), 3U);
}

TEST(DisjointSetTest, ResetAllRestoresSingletonsAndKeepsSize)
{
    disjoint_set<6U> set(4U);
    EXPECT_TRUE(set.unite(0U, 1U));
    EXPECT_TRUE(set.unite(2U, 3U));
    EXPECT_TRUE(set.unite(1U, 3U));
    EXPECT_EQ(set.group_count(), 1U);

    set.reset();
    EXPECT_EQ(set.size(), 4U);
    EXPECT_EQ(set.group_count(), 4U);
    for (size_type i = 0U; i < 4U; ++i)
    {
        EXPECT_EQ(set.find(i), i);
        EXPECT_EQ(set.group_size(i), 1U);
    }
    // Ranks are back to 0: the left root wins again.
    EXPECT_TRUE(set.unite(3U, 0U));
    EXPECT_EQ(set.find(0U), 3U);
}

TEST(DisjointSetTest, GroupsAreOrderedBySmallestMemberWithSortedMembers)
{
    disjoint_set<7U> set;
    EXPECT_TRUE(set.unite(6U, 2U));
    EXPECT_TRUE(set.unite(5U, 0U));
    EXPECT_TRUE(set.unite(2U, 4U));

    disjoint_set<7U>::group_list groups = set.groups();
    ASSERT_EQ(groups.size(), 4U);
    EXPECT_FALSE(groups.empty());

    // {0,5} {1} {2,4,6} {3}
    const uint8_t expected_members[7] = {0U, 5U, 1U, 2U, 4U, 6U, 3U};
    const size_type expected_sizes[4] = {2U, 1U, 3U, 1U};
    size_type flat = 0U;
    for (size_type g = 0U; g < groups.size(); ++g)
    {
        disjoint_set<7U>::group_list::value_type members = groups[g];
        ASSERT_EQ(members.size(), expected_sizes[g]);
        for (size_type m = 0U; m < members.size(); ++m)
        {
            EXPECT_EQ(members[m], expected_members[flat]);
            EXPECT_EQ(groups.group_of(members[m]), g);
            ++flat;
        }
    }
    EXPECT_EQ(flat, 7U);
}

TEST(DisjointSetTest, GroupListRangeForAndSnapshotIndependence)
{
    disjoint_set<4U> set;
    EXPECT_TRUE(set.unite(1U, 3U));
    const disjoint_set<4U>::group_list groups = set.groups();

    size_type visited_groups = 0U;
    size_type visited_nodes = 0U;
    for (disjoint_set<4U>::group_list::value_type members : groups)
    {
        ++visited_groups;
        visited_nodes += members.size();
    }
    EXPECT_EQ(visited_groups, 3U);
    EXPECT_EQ(visited_nodes, 4U);

    // Later changes do not alter the snapshot.
    EXPECT_TRUE(set.unite(0U, 2U));
    EXPECT_EQ(groups.size(), 3U);
    EXPECT_EQ(set.groups().size(), 2U);

    // Iterators of different snapshots never compare equal.
    const disjoint_set<4U>::group_list other = set.groups();
    EXPECT_TRUE(groups.begin() == groups.begin());
    EXPECT_TRUE(groups.begin() != other.begin());
    EXPECT_TRUE(groups.begin() != groups.end());
    EXPECT_EQ((*groups.begin()).size(), 1U);
}

TEST(DisjointSetTest, NodesOutsideRuntimeSizeAreRejected)
{
    // Capacity 8, but only nodes 0..2 are active; 5 is reserved storage, 9 is beyond capacity.
    disjoint_set<8U> set(3U);
    EXPECT_TRUE(set.unite(0U, 1U));

    EXPECT_EQ(set.find(5U), disjoint_set<8U>::npos);
    EXPECT_EQ(set.find(9U), disjoint_set<8U>::npos);
    EXPECT_FALSE(set.unite(2U, 5U));
    EXPECT_FALSE(set.unite(5U, 2U));
    EXPECT_FALSE(set.unite(9U, 0U));
    EXPECT_FALSE(set.same_set(0U, 5U));
    EXPECT_FALSE(set.same_set(5U, 0U));
    EXPECT_EQ(set.group_size(5U), 0U);
    EXPECT_EQ(set.group_size(9U), 0U);
    set.reset(5U);
    set.reset(9U);

    // None of the rejected calls changed the partition.
    EXPECT_EQ(set.group_count(), 2U);
    EXPECT_TRUE(set.same_set(0U, 1U));
    EXPECT_FALSE(set.same_set(0U, 2U));
    EXPECT_EQ(set.group_size(0U), 2U);
    EXPECT_EQ(set.group_size(2U), 1U);

    // Growing the node count makes node 5 a fresh singleton, not a hidden member of node 0's group.
    EXPECT_EQ(set.assign(6U), castle::status::ok);
    EXPECT_EQ(set.find(5U), 5U);
    EXPECT_TRUE(set.unite(1U, 5U));
    EXPECT_FALSE(set.same_set(0U, 1U));
    EXPECT_EQ(set.group_count(), 5U);

    const disjoint_set<8U>::group_list groups = set.groups();
    EXPECT_EQ(groups.group_of(6U), disjoint_set<8U>::npos);
    EXPECT_TRUE(groups[groups.size()].empty());
    EXPECT_EQ(groups[groups.size()].data(), nullptr);
}

TEST(DisjointSetTest, CopiesAreIndependent)
{
    disjoint_set<4U> original;
    EXPECT_TRUE(original.unite(0U, 1U));
    disjoint_set<4U> copy = original;
    EXPECT_TRUE(copy.unite(2U, 3U));
    EXPECT_EQ(original.group_count(), 3U);
    EXPECT_EQ(copy.group_count(), 2U);
    EXPECT_FALSE(original.same_set(2U, 3U));
}

TEST(DisjointSetTest, MatchesNaiveModelUnderRandomOperations)
{
    constexpr size_type nodes = 40U;
    disjoint_set<nodes> set;
    size_type label[nodes];
    size_type next_label = nodes;
    for (size_type i = 0U; i < nodes; ++i)
    {
        label[i] = i;
    }

    uint32_t seed = 12345U;
    for (int step = 0; step < 2000; ++step)
    {
        seed = seed * 1103515245U + 12345U;
        const size_type a = (seed >> 8U) % nodes;
        seed = seed * 1103515245U + 12345U;
        const size_type b = (seed >> 8U) % nodes;
        if ((seed >> 4U) % 8U == 0U)
        {
            set.reset(a);
            label[a] = next_label++;
        }
        else
        {
            const bool merged = set.unite(a, b);
            const size_type from = label[b];
            const size_type to = label[a];
            EXPECT_EQ(merged, from != to);
            for (size_type i = 0U; i < nodes; ++i)
            {
                if (label[i] == from)
                {
                    label[i] = to;
                }
            }
        }

        size_type distinct = 0U;
        for (size_type i = 0U; i < nodes; ++i)
        {
            bool first = true;
            size_type members = 0U;
            for (size_type j = 0U; j < nodes; ++j)
            {
                if (label[j] == label[i])
                {
                    first = first && (j >= i);
                    ++members;
                }
            }
            distinct += first ? 1U : 0U;
            ASSERT_EQ(set.group_size(i), members);
        }
        ASSERT_EQ(set.group_count(), distinct);
        ASSERT_EQ(set.same_set(a, b), label[a] == label[b]);
    }

    const disjoint_set<nodes>::group_list groups = set.groups();
    ASSERT_EQ(groups.size(), set.group_count());
    for (size_type i = 0U; i < nodes; ++i)
    {
        for (size_type j = 0U; j < nodes; ++j)
        {
            EXPECT_EQ(groups.group_of(i) == groups.group_of(j), label[i] == label[j]);
        }
    }
}

} // namespace
