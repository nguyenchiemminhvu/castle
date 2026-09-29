#include <gtest/gtest.h>

#include "castle_ext/diag/diagnostic_manager.hpp"

namespace
{

using castle::diag::active;
using castle::diag::after_cycles;
using castle::diag::after_duration;
using castle::diag::all_of;
using castle::diag::any_of;
using castle::diag::diagnostic_event;
using castle::diag::diagnostic_group;
using castle::diag::diagnostic_manager;
using castle::diag::not_;
using castle::diag::one_shot;

struct immediate_fault_tag
{
};

struct cycle_fault_tag
{
};

struct timed_fault_tag
{
};

struct event_a_tag
{
};

struct event_b_tag
{
};

struct event_c_tag
{
};

struct group_both_tag
{
};

struct group_either_tag
{
};

struct group_not_b_tag
{
};

struct paused_fault_tag
{
};

struct shot_fault_tag
{
};

struct late_callback_fault_tag
{
};

using test_time_point = castle::chrono::steady_clock::time_point;
using test_duration = castle::chrono::steady_clock::duration;

const test_time_point test_epoch(test_duration(0));

void increment_recovery_counter(void* context) noexcept
{
    ++*static_cast<unsigned*>(context);
}

TEST(DiagnosticManager, RawFunctionPointerRecoveryReceivesContext)
{
    using manager_type = diagnostic_manager<diagnostic_event<immediate_fault_tag>>;
    manager_type manager;
    unsigned recoveries = 0U;

    EXPECT_EQ(manager.set_recovery<immediate_fault_tag>(
                  &increment_recovery_counter, &recoveries), castle::status::ok);
    manager.report<immediate_fault_tag>(true);
    const auto result = manager.update(test_epoch);
    EXPECT_EQ(result.recoveries_executed, 1U);
    EXPECT_EQ(recoveries, 1U);
}

TEST(DiagnosticManager, DiagnosticCountersSaturateInsteadOfWrapping)
{
    castle::size_type value = 0U;
    castle::diag::detail::increment_saturated(value);
    EXPECT_EQ(value, 1U);

    value = static_cast<castle::size_type>(~castle::size_type(0));
    castle::diag::detail::increment_saturated(value);
    EXPECT_EQ(value, static_cast<castle::size_type>(~castle::size_type(0)));
}

TEST(DiagnosticManager, ImmediateFaultInvokesRecoveryOnlyOncePerEpisode)
{
    using manager_type = diagnostic_manager<diagnostic_event<immediate_fault_tag>>;
    manager_type manager;
    unsigned recoveries = 0U;
    auto recovery = [&recoveries]() noexcept { ++recoveries; };

    EXPECT_EQ(manager_type::rule_count, 1U);
    EXPECT_EQ(manager.set_recovery<immediate_fault_tag>(recovery), castle::status::ok);
    EXPECT_EQ(manager.report<immediate_fault_tag>(true), castle::status::ok);

    auto result = manager.update(test_epoch);
    EXPECT_EQ(result.activated, 1U);
    EXPECT_EQ(result.cleared, 0U);
    EXPECT_EQ(result.recoveries_executed, 1U);
    EXPECT_TRUE(manager.is_active<immediate_fault_tag>());
    EXPECT_FALSE(manager.is_pending<immediate_fault_tag>());
    EXPECT_EQ(recoveries, 1U);

    result = manager.update(test_epoch + test_duration(1));
    EXPECT_EQ(result.activated, 0U);
    EXPECT_EQ(result.recoveries_executed, 0U);
    EXPECT_EQ(recoveries, 1U);

    manager.report<immediate_fault_tag>(false);
    result = manager.update(test_epoch + test_duration(2));
    EXPECT_EQ(result.cleared, 1U);
    EXPECT_FALSE(manager.is_active<immediate_fault_tag>());
    EXPECT_TRUE(manager.get_state<immediate_fault_tag>().recovery_armed);
    EXPECT_EQ(manager.get_state<immediate_fault_tag>().clear_count, 1U);

    manager.report<immediate_fault_tag>(true);
    result = manager.update(test_epoch + test_duration(3));
    EXPECT_EQ(result.activated, 1U);
    EXPECT_EQ(result.recoveries_executed, 1U);
    EXPECT_EQ(recoveries, 2U);
}

TEST(DiagnosticManager, ReportsBeforeUpdateCoalesceToLatestLevel)
{
    using manager_type = diagnostic_manager<diagnostic_event<immediate_fault_tag>>;
    manager_type manager;

    EXPECT_EQ(manager.report<immediate_fault_tag>(true), castle::status::ok);
    EXPECT_EQ(manager.report<immediate_fault_tag>(false), castle::status::ok);
    auto result = manager.update(test_epoch);
    EXPECT_EQ(result.activated, 0U);
    EXPECT_FALSE(manager.is_active<immediate_fault_tag>());

    manager.report<immediate_fault_tag>(true);
    result = manager.update(test_epoch + test_duration(1));
    EXPECT_EQ(result.activated, 1U);
    EXPECT_TRUE(manager.is_active<immediate_fault_tag>());
}

TEST(DiagnosticManager, CycleQualificationRequiresConsecutiveTrueUpdates)
{
    using manager_type = diagnostic_manager<diagnostic_event<cycle_fault_tag, after_cycles<3U>>>;
    manager_type manager;

    manager.report<cycle_fault_tag>(true);
    manager.update(test_epoch);
    EXPECT_TRUE(manager.is_pending<cycle_fault_tag>());
    EXPECT_EQ(manager.get_state<cycle_fault_tag>().consecutive_samples, 1U);

    manager.update(test_epoch + test_duration(1));
    EXPECT_FALSE(manager.is_active<cycle_fault_tag>());
    EXPECT_EQ(manager.get_state<cycle_fault_tag>().consecutive_samples, 2U);

    manager.report<cycle_fault_tag>(false);
    manager.update(test_epoch + test_duration(2));
    EXPECT_FALSE(manager.is_pending<cycle_fault_tag>());
    EXPECT_EQ(manager.get_state<cycle_fault_tag>().consecutive_samples, 0U);

    manager.report<cycle_fault_tag>(true);
    manager.update(test_epoch + test_duration(3));
    manager.update(test_epoch + test_duration(4));
    EXPECT_FALSE(manager.is_active<cycle_fault_tag>());
    manager.update(test_epoch + test_duration(5));
    EXPECT_TRUE(manager.is_active<cycle_fault_tag>());
    EXPECT_EQ(manager.get_state<cycle_fault_tag>().activation_count, 1U);
    EXPECT_EQ(manager.get_state<cycle_fault_tag>().consecutive_samples, 0U);
}

TEST(DiagnosticManager, PausingPendingMonitorDiscardsQualificationProgress)
{
    using manager_type = diagnostic_manager<diagnostic_event<cycle_fault_tag, after_cycles<2U>>>;
    manager_type manager;

    manager.report<cycle_fault_tag>(true);
    manager.update(test_epoch);
    EXPECT_TRUE(manager.is_pending<cycle_fault_tag>());
    EXPECT_EQ(manager.get_state<cycle_fault_tag>().consecutive_samples, 1U);

    EXPECT_EQ(manager.pause<cycle_fault_tag>(), castle::status::ok);
    manager.update(test_epoch + test_duration(1));
    EXPECT_FALSE(manager.is_pending<cycle_fault_tag>());
    EXPECT_EQ(manager.get_state<cycle_fault_tag>().consecutive_samples, 0U);

    EXPECT_EQ(manager.resume<cycle_fault_tag>(), castle::status::ok);
    manager.update(test_epoch + test_duration(2));
    EXPECT_FALSE(manager.is_active<cycle_fault_tag>());
    EXPECT_EQ(manager.get_state<cycle_fault_tag>().consecutive_samples, 1U);
    manager.update(test_epoch + test_duration(3));
    EXPECT_TRUE(manager.is_active<cycle_fault_tag>());
}

TEST(DiagnosticManager, DurationQualificationUsesElapsedSteadyTime)
{
    using manager_type = diagnostic_manager<
        diagnostic_event<timed_fault_tag, after_duration<castle::chrono::milliseconds, 100>>>;
    manager_type manager;

    manager.report<timed_fault_tag>(true);
    manager.update(test_epoch);
    manager.update(test_epoch + test_duration(99999999LL));
    EXPECT_FALSE(manager.is_active<timed_fault_tag>());
    EXPECT_TRUE(manager.is_pending<timed_fault_tag>());

    manager.update(test_epoch + test_duration(100000000LL));
    EXPECT_TRUE(manager.is_active<timed_fault_tag>());

    manager.report<timed_fault_tag>(false);
    manager.update(test_epoch + test_duration(110000000LL));
    EXPECT_FALSE(manager.is_active<timed_fault_tag>());

    // A new episode starts a new duration window.
    manager.report<timed_fault_tag>(true);
    manager.update(test_epoch + test_duration(200000000LL));
    manager.update(test_epoch + test_duration(299999999LL));
    EXPECT_FALSE(manager.is_active<timed_fault_tag>());
    manager.update(test_epoch + test_duration(300000000LL));
    EXPECT_TRUE(manager.is_active<timed_fault_tag>());
}

TEST(DiagnosticManager, GroupsCombineEventsAndCanBeChained)
{
    using manager_type = diagnostic_manager<
        diagnostic_event<event_a_tag>,
        diagnostic_event<event_b_tag>,
        diagnostic_event<event_c_tag>,
        diagnostic_group<group_both_tag, all_of<active<event_a_tag>, active<event_b_tag>>>,
        diagnostic_group<group_either_tag, any_of<active<group_both_tag>, active<event_c_tag>>>,
        diagnostic_group<group_not_b_tag, all_of<active<event_a_tag>, not_<active<event_b_tag>>>>
    >;
    manager_type manager;

    manager.report<event_a_tag>(true);
    auto result = manager.update(test_epoch);
    EXPECT_TRUE(manager.is_active<event_a_tag>());
    EXPECT_FALSE(manager.is_active<group_both_tag>());
    EXPECT_FALSE(manager.is_active<group_either_tag>());
    EXPECT_TRUE(manager.is_active<group_not_b_tag>());
    EXPECT_EQ(result.activated, 2U);

    manager.report<event_b_tag>(true);
    result = manager.update(test_epoch + test_duration(1));
    EXPECT_TRUE(manager.is_active<group_both_tag>());
    EXPECT_TRUE(manager.is_active<group_either_tag>());
    EXPECT_FALSE(manager.is_active<group_not_b_tag>());
    EXPECT_EQ(result.activated, 3U);
    EXPECT_EQ(result.cleared, 1U);

    manager.report<event_a_tag>(false);
    manager.report<event_b_tag>(false);
    manager.report<event_c_tag>(true);
    result = manager.update(test_epoch + test_duration(2));
    EXPECT_FALSE(manager.is_active<group_both_tag>());
    EXPECT_TRUE(manager.is_active<group_either_tag>());
    EXPECT_FALSE(manager.is_active<group_not_b_tag>());
    EXPECT_EQ(result.cleared, 3U);
    EXPECT_EQ(result.activated, 1U);

    manager.report<event_c_tag>(false);
    manager.update(test_epoch + test_duration(3));
    EXPECT_FALSE(manager.is_active<group_either_tag>());
}

TEST(DiagnosticManager, IndividualPauseHoldsConfirmedStateAndResumesFresh)
{
    using manager_type = diagnostic_manager<diagnostic_event<paused_fault_tag>>;
    manager_type manager;

    manager.report<paused_fault_tag>(true);
    manager.update(test_epoch);
    ASSERT_TRUE(manager.is_active<paused_fault_tag>());

    EXPECT_EQ(manager.pause<paused_fault_tag>(), castle::status::ok);
    EXPECT_FALSE(manager.is_enabled<paused_fault_tag>());
    manager.pause();
    EXPECT_FALSE(manager.is_enabled<paused_fault_tag>());
    manager.resume();
    EXPECT_FALSE(manager.is_enabled<paused_fault_tag>());
    manager.report<paused_fault_tag>(false);
    auto result = manager.update(test_epoch + test_duration(1));
    EXPECT_EQ(result.cleared, 0U);
    EXPECT_TRUE(manager.is_active<paused_fault_tag>());
    EXPECT_FALSE(manager.get_state<paused_fault_tag>().enabled);

    EXPECT_EQ(manager.resume<paused_fault_tag>(), castle::status::ok);
    EXPECT_TRUE(manager.is_enabled<paused_fault_tag>());
    result = manager.update(test_epoch + test_duration(2));
    EXPECT_EQ(result.cleared, 1U);
    EXPECT_FALSE(manager.is_active<paused_fault_tag>());
}

TEST(DiagnosticManager, GlobalPauseAcceptsReportsButFreezesEvaluation)
{
    struct global_a_tag
    {
    };
    struct global_b_tag
    {
    };
    using manager_type = diagnostic_manager<
        diagnostic_event<global_a_tag>, diagnostic_event<global_b_tag>>;
    manager_type manager;

    manager.report<global_a_tag>(true);
    manager.update(test_epoch);
    ASSERT_TRUE(manager.is_active<global_a_tag>());

    manager.pause();
    EXPECT_FALSE(manager.is_enabled<global_a_tag>());
    manager.report<global_a_tag>(false);
    manager.report<global_b_tag>(true);
    auto result = manager.update(test_epoch + test_duration(1));
    EXPECT_EQ(result.activated, 0U);
    EXPECT_EQ(result.cleared, 0U);
    EXPECT_TRUE(manager.is_active<global_a_tag>());
    EXPECT_FALSE(manager.is_active<global_b_tag>());
    EXPECT_FALSE(manager.get_state<global_b_tag>().enabled);

    manager.resume();
    EXPECT_TRUE(manager.is_enabled<global_a_tag>());
    result = manager.update(test_epoch + test_duration(2));
    EXPECT_EQ(result.activated, 1U);
    EXPECT_EQ(result.cleared, 1U);
    EXPECT_FALSE(manager.is_active<global_a_tag>());
    EXPECT_TRUE(manager.is_active<global_b_tag>());

    // A separate cycle-qualified monitor demonstrates that global pause drops
    // partial qualification and the resumed monitor starts at sample one.
    struct global_cycle_tag {};
    diagnostic_manager<diagnostic_event<global_cycle_tag, after_cycles<2U>>> cycle_manager;
    cycle_manager.report<global_cycle_tag>(true);
    cycle_manager.update(test_epoch);
    EXPECT_TRUE(cycle_manager.is_pending<global_cycle_tag>());
    cycle_manager.pause();
    cycle_manager.update(test_epoch + test_duration(1));
    EXPECT_FALSE(cycle_manager.is_pending<global_cycle_tag>());
    cycle_manager.resume();
    cycle_manager.update(test_epoch + test_duration(2));
    EXPECT_FALSE(cycle_manager.is_active<global_cycle_tag>());
    cycle_manager.update(test_epoch + test_duration(3));
    EXPECT_TRUE(cycle_manager.is_active<global_cycle_tag>());
}

TEST(DiagnosticManager, OneShotRecoveryRequiresManualRearm)
{
    using manager_type = diagnostic_manager<diagnostic_event<shot_fault_tag, castle::diag::immediate, one_shot>>;
    manager_type manager;
    unsigned recoveries = 0U;
    auto recovery = [&recoveries]() noexcept { ++recoveries; };
    EXPECT_EQ(manager.set_recovery<shot_fault_tag>(recovery), castle::status::ok);

    manager.report<shot_fault_tag>(true);
    manager.update(test_epoch);
    EXPECT_EQ(recoveries, 1U);
    EXPECT_EQ(manager.rearm<shot_fault_tag>(), castle::status::invalid_argument);

    manager.report<shot_fault_tag>(false);
    manager.update(test_epoch + test_duration(1));
    EXPECT_FALSE(manager.get_state<shot_fault_tag>().recovery_armed);

    // A new failure episode still activates the diagnostic but does not invoke
    // the one-shot recovery until it has been explicitly re-armed.
    manager.report<shot_fault_tag>(true);
    manager.update(test_epoch + test_duration(2));
    EXPECT_TRUE(manager.is_active<shot_fault_tag>());
    EXPECT_EQ(recoveries, 1U);
    EXPECT_EQ(manager.rearm<shot_fault_tag>(), castle::status::invalid_argument);

    manager.report<shot_fault_tag>(false);
    manager.update(test_epoch + test_duration(3));
    EXPECT_EQ(manager.rearm<shot_fault_tag>(), castle::status::ok);

    manager.report<shot_fault_tag>(true);
    manager.update(test_epoch + test_duration(4));
    EXPECT_EQ(recoveries, 2U);
}

TEST(DiagnosticManager, CallbackMayBeClearedAndLateBindingDoesNotFireCurrentEpisode)
{
    using manager_type = diagnostic_manager<diagnostic_event<late_callback_fault_tag>>;
    manager_type manager;
    unsigned recoveries = 0U;
    auto recovery = [&recoveries]() noexcept { ++recoveries; };

    manager.report<late_callback_fault_tag>(true);
    manager.update(test_epoch);
    EXPECT_EQ(recoveries, 0U);
    EXPECT_EQ(manager.set_recovery<late_callback_fault_tag>(recovery), castle::status::ok);
    manager.update(test_epoch + test_duration(1));
    EXPECT_EQ(recoveries, 0U);

    EXPECT_EQ(manager.clear_recovery<late_callback_fault_tag>(), castle::status::ok);
    manager.report<late_callback_fault_tag>(false);
    manager.update(test_epoch + test_duration(2));
    manager.report<late_callback_fault_tag>(true);
    manager.update(test_epoch + test_duration(3));
    EXPECT_EQ(recoveries, 0U);
}

TEST(DiagnosticManager, NoArgumentUpdateUsesSteadyClock)
{
    struct live_fault_tag
    {
    };
    diagnostic_manager<diagnostic_event<live_fault_tag>> manager;
    manager.report<live_fault_tag>(true);
    const auto result = manager.update();
    EXPECT_EQ(result.activated, 1U);
    EXPECT_TRUE(manager.is_active<live_fault_tag>());
}

} // namespace
