#include "castle_ext/data/data_slot.hpp"

#include "castle/utility/move.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <stdint.h>
#include <thread>

namespace
{
struct sample
{
    uint32_t value = 0U;
};

// Counts every copy so tests can prove that reads never copy the value.
struct counted
{
    static inline uint32_t copies = 0U;

    counted() = default;

    explicit counted(uint32_t initial)
        : value(initial)
    {
    }

    counted(const counted& other)
        : value(other.value)
    {
        ++copies;
    }

    counted& operator=(const counted& other)
    {
        value = other.value;
        ++copies;
        return *this;
    }

    uint32_t value = 0U;
};

// Copy-assignment re-enters publish() on the same slot, emulating an overlapping second writer.
struct reentrant
{
    static inline castle::status (*on_assign)() = nullptr;
    static inline castle::status nested_result = castle::status::unknown_error;

    reentrant& operator=(const reentrant& other)
    {
        value = other.value;
        if (on_assign != nullptr)
        {
            nested_result = on_assign();
        }
        return *this;
    }

    uint32_t value = 0U;
};

castle_ext::data::data_slot<reentrant>* reentrant_slot = nullptr;

castle::status publish_nested()
{
    return reentrant_slot->publish(reentrant{});
}

struct wide_record
{
    uint32_t words[32]{};
};
} // namespace

TEST(DataSlotTest, StartsWithDefaultValueAndGenerationZero)
{
    castle_ext::data::data_slot<sample> slot;
    const castle_ext::data::snapshot<sample> initial = slot.get_snapshot();
    ASSERT_TRUE(initial);
    EXPECT_EQ(initial->value, 0U);
    EXPECT_EQ(initial.generation(), 0U);
}

TEST(DataSlotTest, PublishMakesValueCurrentAndAdvancesGeneration)
{
    castle_ext::data::data_slot<sample> slot;
    EXPECT_EQ(slot.publish(sample{7U}), castle::status::ok);
    {
        const castle_ext::data::snapshot<sample> first = slot.get_snapshot();
        EXPECT_EQ((*first).value, 7U);
        EXPECT_EQ(first.get()->value, 7U);
        EXPECT_EQ(first.generation(), 1U);
    }

    for (uint32_t i = 2U; i <= 10U; ++i)
    {
        ASSERT_EQ(slot.publish(sample{i}), castle::status::ok);
        const castle_ext::data::snapshot<sample> latest = slot.get_snapshot();
        EXPECT_EQ(latest->value, i);
        EXPECT_EQ(latest.generation(), i);
    }
}

TEST(DataSlotTest, HeldSnapshotIsStableWhileNewerValuesArePublished)
{
    castle_ext::data::data_slot<sample, 1U> slot;
    ASSERT_EQ(slot.publish(sample{1U}), castle::status::ok);
    const castle_ext::data::snapshot<sample> held = slot.get_snapshot();

    // With one held snapshot the writer alternates between the two spare buffers forever.
    for (uint32_t i = 2U; i <= 20U; ++i)
    {
        ASSERT_EQ(slot.publish(sample{i}), castle::status::ok);
    }
    EXPECT_EQ(held->value, 1U);
    EXPECT_EQ(held.generation(), 1U);
    EXPECT_EQ(slot.get_snapshot()->value, 20U);
}

TEST(DataSlotTest, ReportsFullWhenReadersHoldEverySpareBufferAndKeepsCurrentValue)
{
    castle_ext::data::data_slot<sample, 1U> slot;   // 3 buffers
    castle_ext::data::snapshot<sample> oldest = slot.get_snapshot();   // generation 0
    ASSERT_EQ(slot.publish(sample{1U}), castle::status::ok);
    castle_ext::data::snapshot<sample> older = slot.get_snapshot();    // generation 1
    ASSERT_EQ(slot.publish(sample{2U}), castle::status::ok);

    EXPECT_EQ(slot.publish(sample{3U}), castle::status::full);
    EXPECT_EQ(slot.get_snapshot()->value, 2U);
    EXPECT_EQ(slot.get_snapshot().generation(), 2U);

    // Releasing a retired snapshot returns its buffer to the writer.
    oldest.reset();
    EXPECT_EQ(slot.publish(sample{3U}), castle::status::ok);
    EXPECT_EQ(slot.get_snapshot().generation(), 3U);
    EXPECT_EQ(older->value, 1U);
}

TEST(DataSlotTest, MaxReadersSizesTheNumberOfHeldOldValues)
{
    castle_ext::data::data_slot<sample, 3U> slot;   // 5 buffers
    castle_ext::data::snapshot<sample> held[4];
    for (uint32_t i = 0U; i < 4U; ++i)
    {
        held[i] = slot.get_snapshot();
        ASSERT_EQ(slot.publish(sample{i + 1U}), castle::status::ok);
    }
    EXPECT_EQ(slot.publish(sample{99U}), castle::status::full);
    for (uint32_t i = 0U; i < 4U; ++i)
    {
        EXPECT_EQ(held[i]->value, i);
    }
}

TEST(DataSlotTest, SnapshotsOfTheCurrentValueReleaseWithoutBlockingTheWriter)
{
    castle_ext::data::data_slot<sample, 1U> slot;
    ASSERT_EQ(slot.publish(sample{1U}), castle::status::ok);

    // Many readers of the current buffer acquire and release while it stays current.
    for (uint32_t i = 0U; i < 1000U; ++i)
    {
        castle_ext::data::snapshot<sample> a = slot.get_snapshot();
        castle_ext::data::snapshot<sample> b = slot.get_snapshot();
        EXPECT_EQ(a.get(), b.get());
    }

    // The released counts must not leak: two retire/reuse rounds still succeed.
    castle_ext::data::snapshot<sample> held = slot.get_snapshot();
    ASSERT_EQ(slot.publish(sample{2U}), castle::status::ok);
    ASSERT_EQ(slot.publish(sample{3U}), castle::status::ok);
    held.reset();
    ASSERT_EQ(slot.publish(sample{4U}), castle::status::ok);
    ASSERT_EQ(slot.publish(sample{5U}), castle::status::ok);
    EXPECT_EQ(slot.get_snapshot()->value, 5U);
}

TEST(DataSlotTest, RejectsOverlappingPublishWithDataLoss)
{
    castle_ext::data::data_slot<reentrant> slot;
    reentrant_slot = &slot;
    reentrant::on_assign = &publish_nested;

    reentrant value;
    value.value = 5U;
    EXPECT_EQ(slot.publish(value), castle::status::ok);
    EXPECT_EQ(reentrant::nested_result, castle::status::data_loss);

    reentrant::on_assign = nullptr;
    EXPECT_EQ(slot.get_snapshot()->value, 5U);
    EXPECT_EQ(slot.get_snapshot().generation(), 1U);
    EXPECT_EQ(slot.publish(value), castle::status::ok);
}

TEST(DataSlotTest, PublishCopiesOnceAndReadsNeverCopy)
{
    castle_ext::data::data_slot<counted, 2U> slot;
    counted::copies = 0U;

    ASSERT_EQ(slot.publish(counted{42U}), castle::status::ok);
    EXPECT_EQ(counted::copies, 1U);

    const castle_ext::data::snapshot<counted> first = slot.get_snapshot();
    const castle_ext::data::snapshot<counted> second = slot.get_snapshot();
    EXPECT_EQ(first->value, 42U);
    EXPECT_EQ(second->value, 42U);
    EXPECT_EQ(counted::copies, 1U);
}

TEST(SnapshotTest, EmptySnapshotReportsNoValue)
{
    castle_ext::data::snapshot<sample> empty;
    EXPECT_FALSE(empty);
    EXPECT_EQ(empty.get(), nullptr);
    EXPECT_EQ(empty.generation(), 0U);
    empty.reset();
    EXPECT_FALSE(empty);
}

TEST(SnapshotTest, MoveTransfersOwnershipAndReleasesOnce)
{
    castle_ext::data::data_slot<sample, 1U> slot;
    ASSERT_EQ(slot.publish(sample{1U}), castle::status::ok);

    castle_ext::data::snapshot<sample> source = slot.get_snapshot();
    castle_ext::data::snapshot<sample> moved = CASTLE_MOVE(source);
    EXPECT_FALSE(source);
    ASSERT_TRUE(moved);
    EXPECT_EQ(moved->value, 1U);

    castle_ext::data::snapshot<sample> assigned;
    assigned = CASTLE_MOVE(moved);
    EXPECT_FALSE(moved);
    EXPECT_EQ(assigned->value, 1U);

    castle_ext::data::snapshot<sample>& alias = assigned;
    assigned = CASTLE_MOVE(alias);
    ASSERT_TRUE(assigned);
    EXPECT_EQ(assigned->value, 1U);

    castle_ext::data::snapshot<sample> from_empty = CASTLE_MOVE(source);
    EXPECT_FALSE(from_empty);

    // Move-assigning over a held snapshot releases the old buffer: generation 1 becomes reusable.
    ASSERT_EQ(slot.publish(sample{2U}), castle::status::ok);
    castle_ext::data::snapshot<sample> newer = slot.get_snapshot();
    ASSERT_EQ(slot.publish(sample{3U}), castle::status::ok);
    ASSERT_EQ(slot.publish(sample{4U}), castle::status::full);
    assigned = CASTLE_MOVE(newer);
    EXPECT_FALSE(newer);
    EXPECT_EQ(assigned->value, 2U);
    EXPECT_EQ(slot.publish(sample{4U}), castle::status::ok);
}

TEST(DataSlotTest, ConcurrentReadersAlwaysSeeConsistentValues)
{
    constexpr uint32_t reader_count = 3U;
    constexpr uint32_t publications = 20000U;
    castle_ext::data::data_slot<wide_record, reader_count> slot;

    std::atomic<bool> done{false};
    std::atomic<uint32_t> torn{0U};
    std::atomic<uint32_t> backwards{0U};
    std::atomic<uint32_t> failed_publications{0U};

    auto reader = [&]()
    {
        uint32_t last_generation = 0U;
        while (!done.load(std::memory_order_relaxed))
        {
            const castle_ext::data::snapshot<wide_record> view = slot.get_snapshot();
            for (uint32_t i = 1U; i < 32U; ++i)
            {
                if (view->words[i] != view->words[0])
                {
                    torn.fetch_add(1U);
                }
            }
            if (view->words[0] != view.generation() || view.generation() < last_generation)
            {
                backwards.fetch_add(1U);
            }
            last_generation = view.generation();
        }
    };

    std::thread readers[reader_count];
    for (std::thread& thread : readers)
    {
        thread = std::thread(reader);
    }

    wide_record record{};
    for (uint32_t generation = 1U; generation <= publications; ++generation)
    {
        for (uint32_t& word : record.words)
        {
            word = generation;
        }
        if (slot.publish(record) != castle::status::ok)
        {
            failed_publications.fetch_add(1U);
        }
    }

    done.store(true);
    for (std::thread& thread : readers)
    {
        thread.join();
    }

    // Each reader holds at most one snapshot, so MaxReaders == reader_count never runs out.
    EXPECT_EQ(failed_publications.load(), 0U);
    EXPECT_EQ(torn.load(), 0U);
    EXPECT_EQ(backwards.load(), 0U);
    EXPECT_EQ(slot.get_snapshot().generation(), publications);
}
