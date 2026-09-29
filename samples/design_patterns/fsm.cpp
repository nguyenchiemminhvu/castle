#include "sample_support.hpp"

#include "castle/design_patterns/fsm.hpp"

namespace fsm = castle::design_patterns::fsm;

struct locked
{
};
struct unlocked
{
};
struct alarm
{
};

struct key_turned
{
    unsigned key_code;
};
struct door_closed
{
};
struct forced_entry
{
};
struct alarm_reset
{
};

struct door_context
{
    bool key_is_valid = false;
    unsigned unlock_count = 0U;
    unsigned forced_entry_count = 0U;
};

struct valid_key
{
    bool operator()(door_context& context, key_turned const& event) const noexcept
    {
        return context.key_is_valid && event.key_code == 42U;
    }
};

struct count_unlock
{
    void operator()(door_context& context, key_turned const&) const noexcept
    {
        ++context.unlock_count;
    }
};

struct count_forced_entry
{
    void operator()(door_context& context, forced_entry const&) const noexcept
    {
        ++context.forced_entry_count;
    }
};

using door_machine = fsm::state_machine<
    locked,
    fsm::transition<locked, key_turned, unlocked, valid_key, count_unlock>,
    fsm::transition<unlocked, door_closed, locked>,
    fsm::transition<locked, forced_entry, alarm, fsm::always_guard, count_forced_entry>,
    fsm::transition<unlocked, forced_entry, alarm, fsm::always_guard, count_forced_entry>,
    fsm::transition<alarm, alarm_reset, locked>
>;

using heartbeat_machine = fsm::state_machine<
    locked,
    fsm::transition<locked, door_closed, locked>
>;

int main()
{
    door_machine door;
    door_context context{};

    CASTLE_SAMPLE_CHECK(door_machine::state_count == 3U);
    CASTLE_SAMPLE_CHECK(door_machine::transition_count == 5U);
    CASTLE_SAMPLE_CHECK((door_machine::has_transition<locked, key_turned>()));
    CASTLE_SAMPLE_CHECK((!door_machine::has_transition<alarm, key_turned>()));
    CASTLE_SAMPLE_CHECK(door.current_state_id() == door_machine::state_id<locked>());
    CASTLE_SAMPLE_CHECK(door.is_in_state<locked>());

    // A matching row whose guard rejects the event leaves the state unchanged.
    CASTLE_SAMPLE_CHECK(door.process_event(key_turned{42U}, context) == fsm::event_result::guard_rejected);
    CASTLE_SAMPLE_CHECK(door.is_in_state<locked>());
    CASTLE_SAMPLE_CHECK(context.unlock_count == 0U);

    // The same event succeeds after the application context permits it.
    context.key_is_valid = true;
    CASTLE_SAMPLE_CHECK(door.process_event(key_turned{7U}, context) == fsm::event_result::guard_rejected);
    CASTLE_SAMPLE_CHECK(context.unlock_count == 0U);
    CASTLE_SAMPLE_CHECK(door.process_event(key_turned{42U}, context) == fsm::event_result::transitioned);
    CASTLE_SAMPLE_CHECK(door.is_in_state<unlocked>());
    CASTLE_SAMPLE_CHECK(context.unlock_count == 1U);

    // An event which has no row for the current state is ignored.
    CASTLE_SAMPLE_CHECK(door.process_event(key_turned{42U}, context) == fsm::event_result::ignored);
    CASTLE_SAMPLE_CHECK(door.is_in_state<unlocked>());

    CASTLE_SAMPLE_CHECK(door.process_event(forced_entry{}, context) == fsm::event_result::transitioned);
    CASTLE_SAMPLE_CHECK(door.is_in_state<alarm>());
    CASTLE_SAMPLE_CHECK(context.forced_entry_count == 1U);
    CASTLE_SAMPLE_CHECK(door.process_event(alarm_reset{}, context) == fsm::event_result::transitioned);
    CASTLE_SAMPLE_CHECK(door.is_in_state<locked>());

    // reset() is unconditional and does not invoke transition actions.
    CASTLE_SAMPLE_CHECK(door.process_event(key_turned{42U}, context) == fsm::event_result::transitioned);
    door.reset();
    CASTLE_SAMPLE_CHECK(door.is_in_state<locked>());

    // The one-argument overload supplies empty_context to default guard/action policies.
    heartbeat_machine heartbeat;
    CASTLE_SAMPLE_CHECK(heartbeat.process_event(door_closed{}) == fsm::event_result::transitioned);
    CASTLE_SAMPLE_CHECK(heartbeat.is_in_state<locked>());

    return 0;
}
