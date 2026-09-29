#include "castle_ext/diag/diagnostic_manager.hpp"

#include <assert.h>

struct can_timeout_tag
{
};

struct low_battery_tag
{
};

struct external_mcu_missing_tag
{
};

struct communication_degraded_tag
{
};

struct system_restart_tag
{
};

struct timeout_without_mcu_loss_tag
{
};

struct thermal_trip_tag
{
};

using diagnostic_type = castle::diag::diagnostic_manager<
    castle::diag::diagnostic_event<
        can_timeout_tag,
        castle::diag::after_duration<castle::chrono::milliseconds, 250>>,
    castle::diag::diagnostic_event<
        low_battery_tag,
        castle::diag::after_cycles<3U>>,
    castle::diag::diagnostic_event<external_mcu_missing_tag>,
    castle::diag::diagnostic_group<
        communication_degraded_tag,
        castle::diag::any_of<
            castle::diag::active<can_timeout_tag>,
            castle::diag::active<external_mcu_missing_tag>>>,
    castle::diag::diagnostic_group<
        system_restart_tag,
        castle::diag::all_of<
            castle::diag::active<communication_degraded_tag>,
            castle::diag::active<low_battery_tag>>>,
    castle::diag::diagnostic_group<
        timeout_without_mcu_loss_tag,
        castle::diag::all_of<
            castle::diag::active<can_timeout_tag>,
            castle::diag::not_<castle::diag::active<external_mcu_missing_tag>>>>
>;

using one_shot_diagnostic_type = castle::diag::diagnostic_manager<
    castle::diag::diagnostic_event<
        thermal_trip_tag,
        castle::diag::immediate,
        castle::diag::one_shot>>;

void increment_counter(void* context) noexcept
{
    ++*static_cast<unsigned*>(context);
}

int main()
{
    diagnostic_type diagnostics;
    unsigned restart_requests = 0U;
    auto restart_node = [&restart_requests]() noexcept
    {
        ++restart_requests;
    };
    assert(diagnostics.set_recovery<system_restart_tag>(restart_node) == castle::status::ok);
    assert(diagnostic_type::rule_count == 6U);

    using time_point = diagnostic_type::time_point;
    using clock_duration = diagnostic_type::duration;
    const time_point start(clock_duration(0));

    assert(diagnostics.report<can_timeout_tag>(true) == castle::status::ok);
    assert(diagnostics.report<low_battery_tag>(true) == castle::status::ok);
    assert(diagnostics.report<external_mcu_missing_tag>(false) == castle::status::ok);

    castle::diag::update_result result = diagnostics.update(start);
    assert(result.activated == 0U);
    assert(diagnostics.is_pending<can_timeout_tag>());
    const castle::diag::diagnostic_snapshot low_battery_pending =
        diagnostics.get_state<low_battery_tag>();
    assert(low_battery_pending.reported);
    assert(low_battery_pending.pending);
    assert(!low_battery_pending.active);
    assert(low_battery_pending.enabled);
    assert(low_battery_pending.consecutive_samples == 1U);

    result = diagnostics.update(start + clock_duration(250000000LL));
    assert(diagnostics.is_active<can_timeout_tag>());
    assert(!diagnostics.is_active<low_battery_tag>());
    assert(diagnostics.is_active<communication_degraded_tag>());
    assert(diagnostics.is_active<timeout_without_mcu_loss_tag>());
    const castle::diag::diagnostic_snapshot timeout_state =
        diagnostics.get_state<can_timeout_tag>();
    assert(timeout_state.reported);
    assert(timeout_state.active);
    assert(!timeout_state.pending);
    assert(timeout_state.enabled);
    assert(!timeout_state.recovery_armed);
    assert(timeout_state.activation_count == 1U);
    assert(timeout_state.clear_count == 0U);
    assert(timeout_state.last_transition == start + clock_duration(250000000LL));

    result = diagnostics.update(start + clock_duration(300000000LL));
    assert(diagnostics.is_active<low_battery_tag>());
    assert(diagnostics.is_active<system_restart_tag>());
    assert(restart_requests == 1U);
    assert(result.activated >= 2U);
    assert(result.recoveries_executed == 1U);

    // A persistent fault does not repeatedly invoke the recovery action.
    result = diagnostics.update(start + clock_duration(350000000LL));
    assert(restart_requests == 1U);
    assert(diagnostics.is_enabled<system_restart_tag>());

    // Pausing one source monitor holds its qualified state while accepting reports.
    assert(diagnostics.pause<external_mcu_missing_tag>() == castle::status::ok);
    assert(!diagnostics.is_enabled<external_mcu_missing_tag>());
    assert(!diagnostics.get_state<external_mcu_missing_tag>().enabled);
    diagnostics.report<external_mcu_missing_tag>(true);
    diagnostics.update(start + clock_duration(400000000LL));
    assert(!diagnostics.is_active<external_mcu_missing_tag>());
    assert(diagnostics.resume<external_mcu_missing_tag>() == castle::status::ok);
    assert(diagnostics.is_enabled<external_mcu_missing_tag>());
    diagnostics.update(start + clock_duration(450000000LL));
    assert(diagnostics.is_active<external_mcu_missing_tag>());

    // A global pause freezes all confirmed states and clears partial qualification.
    diagnostics.pause();
    assert(!diagnostics.is_enabled<can_timeout_tag>());
    diagnostics.report<can_timeout_tag>(false);
    diagnostics.report<low_battery_tag>(false);
    result = diagnostics.update(start + clock_duration(500000000LL));
    assert(result.cleared == 0U);
    assert(diagnostics.is_active<system_restart_tag>());
    diagnostics.resume();
    result = diagnostics.update(start + clock_duration(550000000LL));
    assert(result.cleared >= 2U);
    assert(!diagnostics.is_active<system_restart_tag>());
    assert(diagnostics.get_state<can_timeout_tag>().clear_count == 1U);
    assert(restart_requests == 1U);

    // A second failure episode re-arms the default recovery policy.
    diagnostics.report<can_timeout_tag>(true);
    diagnostics.report<low_battery_tag>(true);
    diagnostics.update(start + clock_duration(600000000LL));
    diagnostics.update(start + clock_duration(850000000LL));
    diagnostics.update(start + clock_duration(900000000LL));
    assert(restart_requests == 2U);
    assert(diagnostics.get_state<can_timeout_tag>().activation_count == 2U);

    assert(diagnostics.clear_recovery<system_restart_tag>() == castle::status::ok);

    // one_shot recovery requires an explicit rearm after the fault clears.
    one_shot_diagnostic_type one_shot_diagnostics;
    unsigned thermal_recoveries = 0U;
    const one_shot_diagnostic_type::recovery_callback thermal_callback =
        &increment_counter;
    assert(one_shot_diagnostics.set_recovery<thermal_trip_tag>(
               thermal_callback, &thermal_recoveries) == castle::status::ok);
    assert(one_shot_diagnostics.report<thermal_trip_tag>(true) == castle::status::ok);
    one_shot_diagnostics.update(start);
    assert(thermal_recoveries == 1U);
    assert(one_shot_diagnostics.rearm<thermal_trip_tag>() == castle::status::invalid_argument);
    one_shot_diagnostics.report<thermal_trip_tag>(false);
    one_shot_diagnostics.update(start + clock_duration(1LL));
    assert(one_shot_diagnostics.rearm<thermal_trip_tag>() == castle::status::ok);
    one_shot_diagnostics.report<thermal_trip_tag>(true);
    one_shot_diagnostics.update(start + clock_duration(2LL));
    assert(thermal_recoveries == 2U);

    // The convenience overload samples Castle's steady clock directly.
    using live_manager = castle::diag::diagnostic_manager<
        castle::diag::diagnostic_event<thermal_trip_tag>>;
    live_manager live;
    live.report<thermal_trip_tag>(true);
    result = live.update();
    assert(live.is_active<thermal_trip_tag>());
    assert(result.activated == 1U);

    return 0;
}
