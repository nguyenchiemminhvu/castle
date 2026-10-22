#include <gtest/gtest.h>

#include "castle/core/traits.hpp"
#include "castle_ext/comm/can/detail/slot_table.hpp"

namespace
{

using namespace castle::comm::can;
using castle::comm::can::detail::slot_table;

struct payload
{
    uint32_t id;
    uint8_t flags;
    int16_t offset;

    payload() : id(0U), flags(0U), offset(0) {}
    payload(uint32_t i, uint8_t f, int16_t o) : id(i), flags(f), offset(o) {}
};

static_assert(slot_table<int, 1U>::capacity() == 1U, "capacity is a constant expression");
static_assert(slot_table<int, 7U>::capacity() == 7U, "capacity is a constant expression");
static_assert(slot_table<filter, 4U>::capacity() == 4U, "capacity is independent of T");
static_assert(castle::meta::is_trivially_copyable<filter>::value, "filter is storable as plain value");
static_assert(castle::meta::is_trivially_copyable<filter_handle>::value, "handle is a plain value");

TEST(CanSlotTable, StartsEmpty)
{
    slot_table<int, 4U> table;
    EXPECT_EQ(table.count(), 0U);
    EXPECT_EQ(table.capacity(), 4U);
    for (filter_handle h = -2; h < 6; ++h)
    {
        EXPECT_FALSE(table.is_used(h)) << h;
    }
}

TEST(CanSlotTable, AcquireHandsOutLowestFreeSlotFirst)
{
    slot_table<int, 4U> table;
    EXPECT_EQ(table.acquire(10), 0);
    EXPECT_EQ(table.acquire(11), 1);
    EXPECT_EQ(table.acquire(12), 2);
    EXPECT_EQ(table.acquire(13), 3);
    EXPECT_EQ(table.count(), 4U);
    EXPECT_EQ(table.at(0), 10);
    EXPECT_EQ(table.at(1), 11);
    EXPECT_EQ(table.at(2), 12);
    EXPECT_EQ(table.at(3), 13);
}

TEST(CanSlotTable, AcquireOnFullTableReturnsInvalidFilterAndKeepsContents)
{
    slot_table<int, 2U> table;
    ASSERT_EQ(table.acquire(1), 0);
    ASSERT_EQ(table.acquire(2), 1);
    EXPECT_EQ(table.acquire(3), invalid_filter);
    EXPECT_EQ(table.acquire(4), invalid_filter);
    EXPECT_EQ(table.count(), 2U);
    EXPECT_EQ(table.at(0), 1);
    EXPECT_EQ(table.at(1), 2);
}

TEST(CanSlotTable, ReleasedSlotIsReusedLowestFirst)
{
    slot_table<int, 4U> table;
    for (int i = 0; i < 4; ++i)
    {
        ASSERT_EQ(table.acquire(i), i);
    }
    EXPECT_TRUE(table.release(2));
    EXPECT_TRUE(table.release(1));
    EXPECT_EQ(table.count(), 2U);
    EXPECT_FALSE(table.is_used(1));
    EXPECT_FALSE(table.is_used(2));

    EXPECT_EQ(table.acquire(100), 1);
    EXPECT_EQ(table.acquire(200), 2);
    EXPECT_EQ(table.acquire(300), invalid_filter);
    EXPECT_EQ(table.at(0), 0);
    EXPECT_EQ(table.at(1), 100);
    EXPECT_EQ(table.at(2), 200);
    EXPECT_EQ(table.at(3), 3);
}

TEST(CanSlotTable, IsUsedHandlesNegativeLargeAndFreeHandles)
{
    slot_table<int, 3U> table;
    ASSERT_EQ(table.acquire(5), 0);

    EXPECT_TRUE(table.is_used(0));
    EXPECT_FALSE(table.is_used(1));
    EXPECT_FALSE(table.is_used(2));
    EXPECT_FALSE(table.is_used(3));
    EXPECT_FALSE(table.is_used(1000));
    EXPECT_FALSE(table.is_used(2147483647));
    EXPECT_FALSE(table.is_used(-1));
    EXPECT_FALSE(table.is_used(invalid_filter));
    EXPECT_FALSE(table.is_used(-2147483647 - 1));
}

TEST(CanSlotTable, ReleaseOfUnusedHandleReturnsFalseAndChangesNothing)
{
    slot_table<int, 3U> table;
    ASSERT_EQ(table.acquire(7), 0);

    EXPECT_FALSE(table.release(1));          // free slot
    EXPECT_FALSE(table.release(3));          // one past the end
    EXPECT_FALSE(table.release(99));         // far outside
    EXPECT_FALSE(table.release(-1));         // negative
    EXPECT_FALSE(table.release(invalid_filter));
    EXPECT_EQ(table.count(), 1U);
    EXPECT_TRUE(table.is_used(0));
    EXPECT_EQ(table.at(0), 7);

    EXPECT_TRUE(table.release(0));
    EXPECT_FALSE(table.release(0));          // double release
    EXPECT_EQ(table.count(), 0U);
}

TEST(CanSlotTable, ReleaseUpdatesCountAndUsage)
{
    slot_table<int, 3U> table;
    ASSERT_EQ(table.acquire(1), 0);
    ASSERT_EQ(table.acquire(2), 1);
    EXPECT_EQ(table.count(), 2U);
    EXPECT_TRUE(table.release(0));
    EXPECT_EQ(table.count(), 1U);
    EXPECT_FALSE(table.is_used(0));
    EXPECT_TRUE(table.is_used(1));
    EXPECT_TRUE(table.release(1));
    EXPECT_EQ(table.count(), 0U);
}

TEST(CanSlotTable, ClearFreesEverySlot)
{
    slot_table<int, 3U> table;
    for (int i = 0; i < 3; ++i)
    {
        ASSERT_EQ(table.acquire(i), i);
    }
    ASSERT_EQ(table.count(), 3U);

    table.clear();
    EXPECT_EQ(table.count(), 0U);
    for (filter_handle h = 0; h < 3; ++h)
    {
        EXPECT_FALSE(table.is_used(h)) << h;
        EXPECT_FALSE(table.release(h)) << h;
    }
    EXPECT_EQ(table.acquire(42), 0);
    EXPECT_EQ(table.at(0), 42);
    EXPECT_EQ(table.capacity(), 3U);
}

TEST(CanSlotTable, ClearOnEmptyTableIsHarmless)
{
    slot_table<int, 2U> table;
    table.clear();
    EXPECT_EQ(table.count(), 0U);
    EXPECT_EQ(table.acquire(1), 0);
}

TEST(CanSlotTable, MutableAtAllowsInPlaceModification)
{
    slot_table<int, 2U> table;
    const filter_handle h = table.acquire(5);
    ASSERT_EQ(h, 0);
    table.at(h) = 99;
    EXPECT_EQ(table.at(h), 99);
    int& ref = table.at(h);
    ref += 1;
    EXPECT_EQ(table.at(h), 100);
}

TEST(CanSlotTable, ConstAtReadsStoredValue)
{
    slot_table<int, 2U> table;
    ASSERT_EQ(table.acquire(11), 0);
    ASSERT_EQ(table.acquire(22), 1);

    const slot_table<int, 2U>& view = table;
    EXPECT_EQ(view.at(0), 11);
    EXPECT_EQ(view.at(1), 22);
    EXPECT_EQ(view.count(), 2U);
    EXPECT_TRUE(view.is_used(1));
    EXPECT_FALSE(view.is_used(2));
    EXPECT_EQ(view.capacity(), 2U);
    EXPECT_EQ(&view.at(1), &table.at(1));
}

TEST(CanSlotTable, SingleSlotTable)
{
    slot_table<int, 1U> table;
    EXPECT_EQ(table.acquire(1), 0);
    EXPECT_EQ(table.acquire(2), invalid_filter);
    EXPECT_TRUE(table.release(0));
    EXPECT_EQ(table.acquire(3), 0);
    EXPECT_EQ(table.at(0), 3);
}

TEST(CanSlotTable, WorksBeyondOneMachineWordOfSlots)
{
    slot_table<uint16_t, 70U> table;
    ASSERT_EQ(table.capacity(), 70U);
    for (int i = 0; i < 70; ++i)
    {
        ASSERT_EQ(table.acquire(static_cast<uint16_t>(i * 3)), i) << i;
    }
    EXPECT_EQ(table.count(), 70U);
    EXPECT_EQ(table.acquire(1U), invalid_filter);
    EXPECT_TRUE(table.is_used(69));
    EXPECT_FALSE(table.is_used(70));
    EXPECT_EQ(table.at(69), 207U);
    EXPECT_TRUE(table.release(65));
    EXPECT_EQ(table.acquire(0xABCDU), 65);
    EXPECT_EQ(table.at(65), 0xABCDU);
}

TEST(CanSlotTable, StoresFilterValues)
{
    slot_table<filter, 3U> table;
    const filter_handle a = table.acquire(filter::exact(0x123U));
    const filter_handle b = table.acquire(filter::any(id_format::extended));
    ASSERT_EQ(a, 0);
    ASSERT_EQ(b, 1);
    EXPECT_TRUE(table.at(a).matches(frame(0x123U)));
    EXPECT_FALSE(table.at(a).matches(frame(0x124U)));
    EXPECT_TRUE(table.at(b).matches(frame(0x5U, id_format::extended)));
    table.at(a).mask = 0U;
    EXPECT_TRUE(table.at(a).matches(frame(0x124U)));
}

TEST(CanSlotTable, StoresStructPayloadAndOverwritesOnReuse)
{
    slot_table<payload, 2U> table;
    const filter_handle first = table.acquire(payload(0x1FFU, 3U, -5));
    const filter_handle second = table.acquire(payload(0x200U, 7U, 12));
    ASSERT_EQ(first, 0);
    ASSERT_EQ(second, 1);
    EXPECT_EQ(table.at(first).id, 0x1FFU);
    EXPECT_EQ(table.at(first).flags, 3U);
    EXPECT_EQ(table.at(first).offset, -5);
    EXPECT_EQ(table.at(second).id, 0x200U);

    ASSERT_TRUE(table.release(first));
    ASSERT_EQ(table.acquire(payload(0x300U, 1U, 1)), first);
    EXPECT_EQ(table.at(first).id, 0x300U);
    EXPECT_EQ(table.at(first).flags, 1U);
    EXPECT_EQ(table.at(first).offset, 1);
    EXPECT_EQ(table.at(second).offset, 12);
}

TEST(CanSlotTable, InterleavedAcquireReleaseKeepsCountConsistent)
{
    slot_table<int, 3U> table;
    ASSERT_EQ(table.acquire(1), 0);
    ASSERT_EQ(table.acquire(2), 1);
    ASSERT_TRUE(table.release(0));
    ASSERT_EQ(table.acquire(3), 0);
    ASSERT_EQ(table.acquire(4), 2);
    EXPECT_EQ(table.count(), 3U);
    EXPECT_EQ(table.acquire(5), invalid_filter);
    ASSERT_TRUE(table.release(1));
    ASSERT_TRUE(table.release(2));
    EXPECT_EQ(table.count(), 1U);
    EXPECT_EQ(table.acquire(6), 1);
    EXPECT_EQ(table.at(0), 3);
    EXPECT_EQ(table.at(1), 6);
}

} // namespace
