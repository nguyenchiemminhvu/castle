#include <gtest/gtest.h>

#include "castle_ext/protocols/lin/schedule.hpp"

namespace
{
using namespace castle::protocols::lin;

TEST(LinSchedule, CapacityValidationAndStatus)
{
    schedule_table<2U> schedule;
    EXPECT_TRUE(schedule.empty());
    EXPECT_EQ(schedule.size(), 0U);
    EXPECT_EQ(schedule.capacity(), 2U);
    schedule_entry slot{};
    EXPECT_EQ(schedule.next(slot), castle::status::empty);
    EXPECT_EQ(schedule.get(0U, slot), castle::status::out_of_range);
    EXPECT_EQ(schedule.add(62U, response_owner::master), castle::status::invalid_argument);
    EXPECT_EQ(schedule.add(256U, response_owner::master), castle::status::invalid_argument);
    EXPECT_EQ(schedule.add(1U, static_cast<response_owner>(7U)), castle::status::invalid_argument);
    EXPECT_EQ(schedule.add(1U, response_owner::master, 0U), castle::status::invalid_argument);
    EXPECT_TRUE(schedule.empty());
    EXPECT_EQ(schedule.add(0x10U, response_owner::slave, 5U), castle::status::ok);
    EXPECT_EQ(schedule.add(0x11U, response_owner::master, 2U), castle::status::ok);
    EXPECT_FALSE(schedule.empty());
    EXPECT_EQ(schedule.add(0x12U, response_owner::slave), castle::status::full);
}

TEST(LinSchedule, GetNextWrapResetClearAndDuplicateIDs)
{
    schedule_table<3U> schedule;
    EXPECT_EQ(schedule.add(0x10U, response_owner::slave, 5U), castle::status::ok);
    EXPECT_EQ(schedule.add(0x11U, response_owner::master, 2U), castle::status::ok);
    EXPECT_EQ(schedule.add(0x10U, response_owner::master, 7U), castle::status::ok);
    schedule_entry slot{};
    EXPECT_EQ(schedule.get(2U, slot), castle::status::ok);
    EXPECT_EQ(slot.identifier, 0x10U);
    EXPECT_EQ(slot.owner, response_owner::master);
    EXPECT_EQ(slot.slot_ticks, 7U);
    EXPECT_EQ(schedule.get(3U, slot), castle::status::out_of_range);
    EXPECT_EQ(schedule.next(slot), castle::status::ok);
    EXPECT_EQ(slot.identifier, 0x10U);
    EXPECT_EQ(schedule.next(slot), castle::status::ok);
    EXPECT_EQ(slot.identifier, 0x11U);
    EXPECT_EQ(schedule.next(slot), castle::status::ok);
    EXPECT_EQ(slot.identifier, 0x10U);
    EXPECT_EQ(schedule.next(slot), castle::status::ok);
    EXPECT_EQ(slot.identifier, 0x10U);
    schedule.reset();
    EXPECT_EQ(schedule.next(slot), castle::status::ok);
    EXPECT_EQ(slot.identifier, 0x10U);
    schedule.clear();
    EXPECT_TRUE(schedule.empty());
    EXPECT_EQ(schedule.next(slot), castle::status::empty);
}

TEST(LinSchedule, EntryValidChecksAllFields)
{
    schedule_entry entry{};
    EXPECT_TRUE(entry.valid());
    entry.identifier = 62U;
    EXPECT_FALSE(entry.valid());
    entry.identifier = 0U;
    entry.owner = static_cast<response_owner>(2U);
    EXPECT_FALSE(entry.valid());
    entry.owner = response_owner::master;
    entry.slot_ticks = 0U;
    EXPECT_FALSE(entry.valid());
}
} // namespace
