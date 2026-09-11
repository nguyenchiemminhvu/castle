#include <gtest/gtest.h>
#include "castle/events/tick_timer.h"

namespace
{
volatile int timer_hits=0;
void timer_bump(){++timer_hits;}

TEST(TickTimerTest, ConfigurationStartPauseResetAndPeriodic)
{
    castle::events::tick_timer<2> t;
    EXPECT_EQ(t.set_period(0),castle::status::invalid_config);
    EXPECT_EQ(t.start(),castle::status::not_configured);
    EXPECT_EQ(t.resume(),castle::status::not_configured);
    t.on_tick(10);
    EXPECT_FALSE(t.is_running());
    EXPECT_EQ(t.remaining(),0U);
    EXPECT_EQ(t.set_period(3),castle::status::ok);
    EXPECT_EQ(t.period(),3U);
    EXPECT_EQ(t.start(castle::events::tick_timer_mode::n_repeat,0),castle::status::invalid_config); EXPECT_EQ(t.start(castle::events::tick_timer_mode::periodic),castle::status::ok);
    EXPECT_EQ(t.repeats_remaining(),0U);
    EXPECT_EQ(t.remaining(),3U);
    castle::callbacks::function<void()> cb(&timer_bump);
    auto sub=t.register_callback(&cb);
    EXPECT_TRUE(sub.valid());
    timer_hits=0;
    t.on_tick(2);
    EXPECT_EQ(timer_hits,0);
    EXPECT_EQ(t.elapsed(),2U);
    EXPECT_EQ(t.remaining(),1U);
    t.pause();
    t.on_tick(10);
    EXPECT_EQ(timer_hits,0);
    EXPECT_EQ(t.resume(),castle::status::ok);
    t.on_tick(10);
    EXPECT_EQ(timer_hits,4);
    t.reset();
    EXPECT_EQ(t.elapsed(),0U);
    t.stop();
    EXPECT_FALSE(t.is_running());
    EXPECT_EQ(t.elapsed(),0U);
    EXPECT_EQ(t.remaining(),0U);
}

TEST(TickTimerTest, OneShotRepeatCatchupAndRegistry)
{
    castle::events::tick_timer<2> t;
    castle::callbacks::function<void()> cb(&timer_bump);
    t.register_callback(&cb);
    t.set_period(4);
    timer_hits=0;
    EXPECT_EQ(t.start(castle::events::tick_timer_mode::one_shot),castle::status::ok);
    t.on_tick(20);
    EXPECT_EQ(timer_hits,1);
    EXPECT_FALSE(t.is_running());
    EXPECT_EQ(t.elapsed(),0U);
    EXPECT_EQ(t.start(castle::events::tick_timer_mode::n_repeat,3),castle::status::ok);
    EXPECT_EQ(t.repeats_remaining(),3U);
    t.on_tick(10);
    EXPECT_EQ(timer_hits,3);
    EXPECT_TRUE(t.is_running());
    EXPECT_EQ(t.repeats_remaining(),1U);
    t.on_tick(4);
    EXPECT_EQ(timer_hits,4);
    EXPECT_FALSE(t.is_running());
    EXPECT_EQ(t.repeats_remaining(),0U);
    EXPECT_EQ(t.callback_count(),1U);
    t.clear_callbacks();
    EXPECT_EQ(t.callback_count(),0U);
    auto &r=t.registry();
    EXPECT_TRUE(r.empty());
    EXPECT_TRUE(castle::events::tick_timer<2>::max_period()>0U);
}
}
