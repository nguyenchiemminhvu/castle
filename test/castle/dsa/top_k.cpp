#include <gtest/gtest.h>

#include "castle/dsa/top_k.hpp"
#include "castle/dsa/string_view.hpp"

#include <stdint.h>

namespace
{
using castle::size_type;
using castle::dsa::array_view;
using castle::dsa::top_k;

// Compares CAN ids on the low 8 bits only, to show a custom KeyEqual with state.
struct masked_equal
{
    uint32_t mask;

    bool operator()(const uint32_t& lhs, const uint32_t& rhs) const noexcept
    {
        return (lhs & mask) == (rhs & mask);
    }
};

// Smallest tracked count found by a full scan; must match min_count() when full.
template <typename Summary>
typename Summary::counter_type scan_min(const Summary& summary)
{
    typename Summary::counter_type smallest = summary.begin()->count;
    for (const auto& tracked : summary)
    {
        if (tracked.count < smallest)
        {
            smallest = tracked.count;
        }
    }
    return smallest;
}

TEST(TopKTest, StartsEmpty)
{
    top_k<uint32_t, 4U> summary;
    static_assert(top_k<uint32_t, 4U>::capacity() == 4U, "K slots");
    EXPECT_TRUE(summary.empty());
    EXPECT_FALSE(summary.full());
    EXPECT_EQ(summary.size(), 0U);
    EXPECT_EQ(summary.total(), 0U);
    EXPECT_EQ(summary.min_count(), 0U);
    EXPECT_EQ(summary.estimate(7U), 0U);
    EXPECT_EQ(summary.find(7U), nullptr);
    EXPECT_FALSE(summary.contains(7U));
    EXPECT_EQ(summary.begin(), summary.end());
}

TEST(TopKTest, CountsExactlyUntilFull)
{
    top_k<uint32_t, 4U> summary;
    EXPECT_EQ(summary.add(10U), 1U);
    EXPECT_EQ(summary.add(10U, 4U), 5U);
    EXPECT_EQ(summary.add(20U, 2U), 2U);
    EXPECT_EQ(summary.add(30U), 1U);
    EXPECT_EQ(summary.size(), 3U);
    EXPECT_FALSE(summary.full());
    EXPECT_EQ(summary.min_count(), 0U); // nothing evicted yet
    EXPECT_EQ(summary.estimate(99U), 0U);
    EXPECT_EQ(summary.total(), 8U);

    const top_k<uint32_t, 4U>::entry_type* tracked = summary.find(10U);
    ASSERT_NE(tracked, nullptr);
    EXPECT_EQ(tracked->key, 10U);
    EXPECT_EQ(tracked->count, 5U);
    EXPECT_EQ(tracked->error, 0U);
    EXPECT_EQ(tracked->guaranteed(), 5U);
    EXPECT_TRUE(summary.contains(20U));
}

TEST(TopKTest, FullSummaryKeepsMinimumAtRoot)
{
    top_k<uint32_t, 5U> summary;
    summary.add(1U, 5U);
    summary.add(2U, 4U);
    summary.add(3U, 3U);
    summary.add(4U, 2U);
    EXPECT_EQ(summary.min_count(), 0U);
    summary.add(5U, 1U); // fills the summary: the heap is built
    EXPECT_TRUE(summary.full());
    EXPECT_EQ(summary.min_count(), 1U);
    EXPECT_EQ(summary.begin()->key, 5U);

    // Growing the minimum moves it down; the next smallest becomes the root.
    EXPECT_EQ(summary.add(5U, 10U), 11U);
    EXPECT_EQ(summary.min_count(), 2U);
    EXPECT_EQ(summary.begin()->key, 4U);
    EXPECT_EQ(scan_min(summary), summary.min_count());

    // Growing a key deep in the heap keeps the root.
    EXPECT_EQ(summary.add(1U), 6U);
    EXPECT_EQ(summary.min_count(), 2U);
}

TEST(TopKTest, NewKeyEvictsMinimumAndInheritsError)
{
    top_k<uint32_t, 3U> summary;
    summary.add(1U, 9U);
    summary.add(2U, 5U);
    summary.add(3U, 2U);

    EXPECT_EQ(summary.add(4U), 3U); // replaces key 3: count 2 + 1, error 2
    EXPECT_FALSE(summary.contains(3U));
    EXPECT_EQ(summary.size(), 3U);
    const top_k<uint32_t, 3U>::entry_type* newcomer = summary.find(4U);
    ASSERT_NE(newcomer, nullptr);
    EXPECT_EQ(newcomer->count, 3U);
    EXPECT_EQ(newcomer->error, 2U);
    EXPECT_EQ(newcomer->guaranteed(), 1U);

    // The evicted key is bounded by the new minimum.
    EXPECT_EQ(summary.min_count(), 3U);
    EXPECT_EQ(summary.estimate(3U), 3U);
    EXPECT_EQ(summary.total(), 17U);
}

TEST(TopKTest, ZeroCountChangesNothing)
{
    top_k<uint32_t, 2U> summary;
    summary.add(1U, 4U);
    summary.add(2U, 6U);
    EXPECT_EQ(summary.add(3U, 0U), 4U); // untracked: min_count(), no eviction
    EXPECT_FALSE(summary.contains(3U));
    EXPECT_EQ(summary.add(2U, 0U), 6U);
    EXPECT_EQ(summary.total(), 10U);
}

TEST(TopKTest, SingleSlot)
{
    top_k<uint32_t, 1U> summary;
    EXPECT_EQ(summary.add(1U), 1U);
    EXPECT_TRUE(summary.full());
    EXPECT_EQ(summary.add(1U), 2U);
    EXPECT_EQ(summary.add(2U), 3U); // every new key evicts the only entry
    EXPECT_EQ(summary.find(2U)->error, 2U);
    EXPECT_EQ(summary.min_count(), 3U);
}

TEST(TopKTest, SpaceSavingGuarantees)
{
    // Heavy keys 0 .. 3 hold 70 % of the stream; 300 distinct one-off keys make the noise.
    constexpr size_type slots = 10U;
    top_k<uint32_t, slots> summary;
    uint32_t truth[4] = {0U, 0U, 0U, 0U};
    uint32_t noise = 1000U;
    for (uint32_t round = 0U; round < 100U; ++round)
    {
        for (uint32_t heavy = 0U; heavy < 4U; ++heavy)
        {
            const uint32_t repeats = 4U - heavy; // 4, 3, 2, 1 per round
            summary.add(heavy, repeats);
            truth[heavy] += repeats;
        }
        for (uint32_t i = 0U; i < 3U; ++i)
        {
            summary.add(noise);
            ++noise;
        }
        if (summary.full())
        {
            EXPECT_EQ(scan_min(summary), summary.min_count());
        }
    }

    const uint64_t total = summary.total();
    EXPECT_EQ(total, 1300U);
    EXPECT_LE(static_cast<uint64_t>(summary.min_count()), total / slots);

    // Every key above N / K (130) is tracked, with count - error <= f <= count.
    for (uint32_t heavy = 0U; heavy < 3U; ++heavy)
    {
        const top_k<uint32_t, slots>::entry_type* tracked = summary.find(heavy);
        ASSERT_NE(tracked, nullptr);
        EXPECT_GT(truth[heavy], total / slots);
        EXPECT_LE(tracked->guaranteed(), truth[heavy]);
        EXPECT_GE(tracked->count, truth[heavy]);
    }

    // Key 3 (100 < N / K) carries no guarantee: tracked within bounds, or below min_count().
    const top_k<uint32_t, slots>::entry_type* light = summary.find(3U);
    if (light != nullptr)
    {
        EXPECT_LE(light->guaranteed(), truth[3]);
        EXPECT_GE(light->count, truth[3]);
    }
    else
    {
        EXPECT_LE(truth[3], summary.min_count());
    }

    // One-off keys: either tracked with a lower bound of at most 1, or bounded by min_count().
    for (uint32_t key = 1000U; key < noise; ++key)
    {
        const top_k<uint32_t, slots>::entry_type* tracked = summary.find(key);
        if (tracked != nullptr)
        {
            EXPECT_LE(tracked->guaranteed(), 1U);
        }
        EXPECT_GE(summary.estimate(key), 1U);
    }
}

TEST(TopKTest, TopListsHighestFirst)
{
    using summary_type = top_k<uint32_t, 4U>;
    summary_type summary;
    summary.add(1U, 3U);
    summary.add(2U, 8U);
    summary.add(3U, 1U);
    summary.add(4U, 5U);

    summary_type::entry_type best[2];
    EXPECT_EQ(summary.top(array_view<summary_type::entry_type>(best, 2U)), 2U);
    EXPECT_EQ(best[0].key, 2U);
    EXPECT_EQ(best[0].count, 8U);
    EXPECT_EQ(best[1].key, 4U);
    EXPECT_EQ(best[1].count, 5U);

    // A larger buffer receives only the tracked entries.
    summary_type::entry_type all[6];
    EXPECT_EQ(summary.top(array_view<summary_type::entry_type>(all, 6U)), 4U);
    EXPECT_EQ(all[2].key, 1U);
    EXPECT_EQ(all[3].key, 3U);
    EXPECT_EQ(summary.top(array_view<summary_type::entry_type>()), 0U);

    // The sorted storage is still a valid min-heap: eviction keeps working.
    EXPECT_EQ(summary.min_count(), 1U);
    EXPECT_EQ(summary.add(9U), 2U); // replaces key 3
    EXPECT_FALSE(summary.contains(3U));
    EXPECT_EQ(scan_min(summary), summary.min_count());
}

TEST(TopKTest, TopBeforeFull)
{
    using summary_type = top_k<uint32_t, 8U>;
    summary_type summary;
    summary.add(5U, 2U);
    summary.add(6U, 7U);
    summary_type::entry_type best[8];
    EXPECT_EQ(summary.top(array_view<summary_type::entry_type>(best, 8U)), 2U);
    EXPECT_EQ(best[0].key, 6U);
    EXPECT_EQ(best[1].key, 5U);
    EXPECT_EQ(summary.estimate(5U), 2U);
}

TEST(TopKTest, HalveAgesCountsAndBounds)
{
    top_k<uint32_t, 2U> summary;
    summary.add(1U, 3U);
    summary.add(2U, 9U);
    summary.add(3U, 1U); // evicts key 1: count 4, error 3

    summary.halve();
    const top_k<uint32_t, 2U>::entry_type* aged = summary.find(3U);
    ASSERT_NE(aged, nullptr);
    EXPECT_EQ(aged->count, 2U);      // 4 / 2
    EXPECT_EQ(aged->guaranteed(), 0U); // (4 - 3) / 2
    EXPECT_EQ(aged->error, 2U);
    EXPECT_EQ(summary.find(2U)->count, 4U);
    EXPECT_EQ(summary.find(2U)->error, 0U);
    EXPECT_EQ(summary.total(), 6U);

    // Even count with odd error: the lower bound rounds down, not up.
    top_k<uint32_t, 1U> single;
    single.add(1U);
    single.add(2U, 3U); // count 4, error 1
    single.halve();
    EXPECT_EQ(single.find(2U)->count, 2U);
    EXPECT_EQ(single.find(2U)->guaranteed(), 1U); // floor(3 / 2)

    // The heap order survives halving.
    EXPECT_EQ(summary.min_count(), 2U);
    EXPECT_EQ(scan_min(summary), summary.min_count());
}

TEST(TopKTest, CountsSaturate)
{
    top_k<uint32_t, 2U, uint8_t> summary;
    EXPECT_EQ(summary.add(1U, 200U), 200U);
    EXPECT_EQ(summary.add(1U, 100U), 255U);
    EXPECT_EQ(summary.add(2U, 250U), 250U);
    EXPECT_EQ(summary.add(3U, 10U), 255U); // evicts key 2: 250 + 10 saturates
    EXPECT_EQ(summary.find(3U)->error, 250U);
    EXPECT_EQ(summary.total(), 560U); // the total keeps counting past the counter limit
}

TEST(TopKTest, ClearAndCopy)
{
    top_k<uint32_t, 3U> summary;
    summary.add(1U, 2U);
    summary.add(2U);

    top_k<uint32_t, 3U> copy(summary);
    summary.clear();
    EXPECT_TRUE(summary.empty());
    EXPECT_EQ(summary.total(), 0U);
    EXPECT_FALSE(summary.contains(1U));

    EXPECT_EQ(copy.size(), 2U);
    EXPECT_EQ(copy.estimate(1U), 2U); // the copy is independent
}

TEST(TopKTest, StringViewKeys)
{
    using castle::dsa::string_view;
    top_k<string_view, 2U, uint16_t> summary;
    summary.add(string_view("door_open"));
    summary.add(string_view("door_open"));
    summary.add(string_view("low_battery"));
    EXPECT_EQ(summary.estimate(string_view("door_open")), 2U);
    EXPECT_EQ(summary.estimate(string_view("low_battery")), 1U);
    EXPECT_EQ(summary.add(string_view("overheat")), 2U); // evicts "low_battery"
    EXPECT_FALSE(summary.contains(string_view("low_battery")));
}

TEST(TopKTest, CustomKeyEqual)
{
    top_k<uint32_t, 4U, uint32_t, masked_equal> summary(masked_equal{0xFFU});
    EXPECT_EQ(summary.key_eq().mask, 0xFFU);
    summary.add(0x100U);
    summary.add(0x200U); // same low byte: same key
    EXPECT_EQ(summary.size(), 1U);
    EXPECT_EQ(summary.estimate(0x300U), 2U);
}
} // namespace
