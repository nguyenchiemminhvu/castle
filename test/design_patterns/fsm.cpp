#include <gtest/gtest.h>

#include "castle/design_patterns/fsm.hpp"

namespace
{
namespace fsm = castle::design_patterns::fsm;

struct idle
{
};
struct active
{
};
struct fault
{
};
struct start_event
{
    unsigned source = 0U;
};
struct stop_event
{
};
struct fault_event
{
};
struct reset_event
{
};
struct unknown_event
{
};

struct test_context
{
    bool allow_start = true;
    int start_actions = 0;
    int stop_actions = 0;
    int fault_actions = 0;
};

struct start_guard
{
    bool operator()(test_context& context, start_event const& event) const noexcept
    {
        return context.allow_start && event.source == 1U;
    }
};

struct start_action
{
    void operator()(test_context& context, start_event const&) const noexcept
    {
        ++context.start_actions;
    }
};

struct stop_action
{
    void operator()(test_context& context, stop_event const&) const noexcept
    {
        ++context.stop_actions;
    }
};

struct fault_action
{
    void operator()(test_context& context, fault_event const&) const noexcept
    {
        ++context.fault_actions;
    }
};

using tested_machine = fsm::state_machine<
    idle,
    fsm::transition<idle, start_event, active, start_guard, start_action>,
    fsm::transition<active, stop_event, idle, fsm::always_guard, stop_action>,
    fsm::transition<idle, fault_event, fault, fsm::always_guard, fault_action>,
    fsm::transition<active, fault_event, fault, fsm::always_guard, fault_action>,
    fsm::transition<fault, reset_event, idle>
>;

TEST(FsmTest, CompileTimeMetadataAndInitialState)
{
    static_assert(tested_machine::state_count == 3U, "Unique states must be inferred from transition rows.");
    static_assert(tested_machine::transition_count == 5U, "All transition rows must be counted.");
    static_assert(sizeof(tested_machine) == sizeof(castle::size_type),
                  "An FSM instance should store only its current state index.");
    static_assert(tested_machine::has_transition<idle, start_event>(), "Declared transition must be found.");
    static_assert(tested_machine::has_transition<active, stop_event>(), "Declared transition must be found.");
    static_assert(tested_machine::has_transition<idle, start_event const&>(),
                  "Event cv/ref qualifiers must be removed for metadata queries.");
    static_assert(!tested_machine::has_transition<fault, start_event>(), "Missing transition must not be found.");

    tested_machine machine;
    EXPECT_TRUE(machine.is_in_state<idle>());
    EXPECT_FALSE(machine.is_in_state<active>());
    EXPECT_EQ(machine.current_state_id(), tested_machine::state_id<idle>());
    EXPECT_NE(tested_machine::state_id<idle>(), tested_machine::state_id<active>());
}

TEST(FsmTest, TransitionRunsGuardAndAction)
{
    tested_machine machine;
    test_context context{};

    EXPECT_EQ(machine.process_event(start_event{1U}, context), fsm::event_result::transitioned);
    EXPECT_TRUE(machine.is_in_state<active>());
    EXPECT_EQ(context.start_actions, 1);

    EXPECT_EQ(machine.process_event(stop_event{}, context), fsm::event_result::transitioned);
    EXPECT_TRUE(machine.is_in_state<idle>());
    EXPECT_EQ(context.stop_actions, 1);
}

TEST(FsmTest, RejectedGuardSkipsActionAndPreservesState)
{
    tested_machine machine;
    test_context context{};
    context.allow_start = false;

    EXPECT_EQ(machine.process_event(start_event{1U}, context), fsm::event_result::guard_rejected);
    context.allow_start = true;
    EXPECT_EQ(machine.process_event(start_event{2U}, context), fsm::event_result::guard_rejected);
    EXPECT_TRUE(machine.is_in_state<idle>());
    EXPECT_EQ(context.start_actions, 0);
}

TEST(FsmTest, UnknownEventAndWrongStateAreIgnored)
{
    tested_machine machine;
    test_context context{};

    EXPECT_EQ(machine.process_event(unknown_event{}, context), fsm::event_result::ignored);
    EXPECT_EQ(machine.process_event(stop_event{}, context), fsm::event_result::ignored);
    EXPECT_TRUE(machine.is_in_state<idle>());

    EXPECT_EQ(machine.process_event(start_event{1U}, context), fsm::event_result::transitioned);
    EXPECT_EQ(machine.process_event(start_event{1U}, context), fsm::event_result::ignored);
    EXPECT_TRUE(machine.is_in_state<active>());
}

TEST(FsmTest, SameEventCanHaveDifferentSourceStates)
{
    tested_machine machine;
    test_context context{};

    EXPECT_EQ(machine.process_event(fault_event{}, context), fsm::event_result::transitioned);
    EXPECT_TRUE(machine.is_in_state<fault>());
    EXPECT_EQ(context.fault_actions, 1);

    EXPECT_EQ(machine.process_event(reset_event{}, context), fsm::event_result::transitioned);
    EXPECT_TRUE(machine.is_in_state<idle>());
    EXPECT_EQ(machine.process_event(start_event{1U}, context), fsm::event_result::transitioned);
    EXPECT_TRUE(machine.is_in_state<active>());
    EXPECT_EQ(machine.process_event(fault_event{}, context), fsm::event_result::transitioned);
    EXPECT_TRUE(machine.is_in_state<fault>());
    EXPECT_EQ(context.fault_actions, 2);
}

TEST(FsmTest, ResetRestoresInitialStateWithoutRunningAnAction)
{
    tested_machine machine;
    test_context context{};

    EXPECT_EQ(machine.process_event(start_event{1U}, context), fsm::event_result::transitioned);
    EXPECT_TRUE(machine.is_in_state<active>());
    EXPECT_EQ(context.start_actions, 1);

    machine.reset();
    EXPECT_TRUE(machine.is_in_state<idle>());
    EXPECT_EQ(context.start_actions, 1);
    EXPECT_EQ(machine.current_state_id(), tested_machine::state_id<idle>());
}

TEST(FsmTest, EmptyTransitionTableAndNoContextOverload)
{
    struct dormant
    {
    };
    struct ping
    {
    };
    using empty_machine = fsm::state_machine<dormant>;
    using self_transition_machine = fsm::state_machine<
        dormant,
        fsm::transition<dormant, ping, dormant>
    >;

    empty_machine empty;
    EXPECT_EQ(empty_machine::state_count, 1U);
    EXPECT_EQ(empty_machine::transition_count, 0U);
    EXPECT_EQ(empty.process_event(ping{}), fsm::event_result::ignored);
    EXPECT_TRUE(empty.is_in_state<dormant>());

    self_transition_machine self;
    EXPECT_EQ(self.process_event(ping{}), fsm::event_result::transitioned);
    EXPECT_TRUE(self.is_in_state<dormant>());
}

} // namespace
