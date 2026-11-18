#include <gtest/gtest.h>
#include "castle/events/tick_timer.hpp"

namespace
{
TEST(InplaceTickTimerTest, AllModesAndOwnedCallbacks)
{
    castle::events::tick_timer<2,32,8> t;
    EXPECT_EQ(t.set_period(0),castle::status::invalid_config);
    EXPECT_EQ(t.resume(),castle::status::not_configured);
    int calls=0;
    auto s=t.register_callback([&]{++calls;});
    EXPECT_TRUE(s.valid());
    EXPECT_EQ(t.start(),castle::status::not_configured);
    t.set_period(3);
    EXPECT_EQ(t.start(castle::events::tick_timer_mode::periodic),castle::status::ok);
    EXPECT_EQ(t.period(),3U); t.on_tick(7);
    EXPECT_EQ(calls,2);
    EXPECT_EQ(t.elapsed(),1U);
    t.pause();
    t.on_tick(9);
    EXPECT_EQ(calls,2);
    EXPECT_EQ(t.resume(),castle::status::ok);
    t.on_tick(2);
    EXPECT_EQ(calls,3);
    t.reset();
    EXPECT_EQ(t.elapsed(),0U);
    EXPECT_EQ(t.start(castle::events::tick_timer_mode::one_shot),castle::status::ok);
    t.on_tick(10);
    EXPECT_EQ(calls,4);
    EXPECT_FALSE(t.is_running());
    EXPECT_EQ(t.start(castle::events::tick_timer_mode::n_repeat,0),castle::status::invalid_config);
    EXPECT_EQ(t.start(castle::events::tick_timer_mode::n_repeat,2),castle::status::ok);
    EXPECT_EQ(t.repeats_remaining(),2U);
    t.on_tick(9);
    EXPECT_EQ(calls,6);
    EXPECT_FALSE(t.is_running());
    EXPECT_EQ(t.repeats_remaining(),0U);
    EXPECT_EQ(t.callback_count(),1U);
    t.clear_callbacks();
    EXPECT_EQ(t.callback_count(),0U);
}

TEST(InplaceTickTimerTest, RegisterCallbackWithOutErrorCoversBranch)
{
    castle::events::tick_timer<1,32,8> t;

    castle::status err = castle::status::unknown_error;

    auto sub = t.register_callback([]{}, &err);

    EXPECT_TRUE(sub.valid());
    EXPECT_EQ(err, castle::status::ok);
}

TEST(InplaceTickTimerTest, NRepeatRemainingGreaterThanZeroBranch)
{
    castle::events::tick_timer<1,32,8> t;

    int calls = 0;
    t.register_callback([&]{ ++calls; });

    ASSERT_EQ(t.set_period(5), castle::status::ok);
    ASSERT_EQ(
    t.start(castle::events::tick_timer_mode::n_repeat, 3),
    castle::status::ok);

    t.on_tick(5);

    EXPECT_EQ(calls, 1);
    EXPECT_TRUE(t.is_running());
    EXPECT_EQ(t.repeats_remaining(), 2U); // covers repeat_remaining_ > 0 path

    t.on_tick(5);

    EXPECT_EQ(calls, 2);
    EXPECT_TRUE(t.is_running());
    EXPECT_EQ(t.repeats_remaining(), 1U);

    t.on_tick(5);

    EXPECT_EQ(calls, 3);
    EXPECT_FALSE(t.is_running());
    EXPECT_EQ(t.repeats_remaining(), 0U);
}

TEST(InplaceTickTimerTest, RepeatsRemainingReturnsZeroForNonNRepeatMode)
{
    castle::events::tick_timer<1,32,8> t;

    EXPECT_EQ(t.repeats_remaining(), 0U);

    ASSERT_EQ(t.set_period(3), castle::status::ok);
    ASSERT_EQ(
    t.start(castle::events::tick_timer_mode::periodic),
    castle::status::ok);

    EXPECT_EQ(t.repeats_remaining(), 0U); // covers false side of ternary
}
}
