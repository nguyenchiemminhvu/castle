/**
 * Castle sample: <castle/filter/debounce_sampling.h>
 *
 * Scenarios:
 *   1. Button debounce (valid/cleared detection, incl. mechanical bounce)
 *   2. Hold detection
 *   3. Repeat / auto-repeat detection
 *   4. Callback registration and removal
 *   5. Integration into a periodic task / scheduler tick (GPIO polling)
 */
#include "sample_support.hpp"

#include "castle/filter/debounce_sampling.hpp"

#include <stdint.h>

namespace
{

// ---------------------------------------------------------------------------
// Scenario 1: button debounce with mechanical bounce before settling.
// ---------------------------------------------------------------------------
void run_button_debounce_scenario()
{
    using button_type = castle::filter::debounce_sampling<20U>;
    button_type button;

    uint32_t press_events = 0U;
    uint32_t release_events = 0U;
    button.on_valid([&press_events] { ++press_events; });
    button.on_cleared([&release_events] { ++release_events; });

    // Mechanical chatter while the contact settles - none of this reaches
    // ValidCount (20) consecutive identical samples, so nothing fires yet.
    CASTLE_CONST bool bounce_pattern[] = { true, false, true, false, true };
    for (bool bounced_sample : bounce_pattern)
    {
        CASTLE_SAMPLE_CHECK(!button.sample(bounced_sample));
    }

    // The contact has settled high. 19 more identical samples are still not
    // enough (last bounce sample already counts as the 1st of the run).
    for (uint32_t tick = 0U; tick < 18U; ++tick)
    {
        CASTLE_SAMPLE_CHECK(!button.sample(true));
    }
    CASTLE_SAMPLE_CHECK(!button.is_valid());

    // The 20th consecutive "pressed" sample confirms the press.
    CASTLE_SAMPLE_CHECK(button.sample(true));
    CASTLE_SAMPLE_CHECK(button.is_valid());
    CASTLE_SAMPLE_CHECK(press_events == 1U);

    // Release is confirmed with the same ValidCount-sample debounce delay.
    for (uint32_t tick = 0U; tick < 19U; ++tick)
    {
        CASTLE_SAMPLE_CHECK(!button.sample(false));
    }
    CASTLE_SAMPLE_CHECK(button.sample(false));
    CASTLE_SAMPLE_CHECK(!button.is_valid());
    CASTLE_SAMPLE_CHECK(release_events == 1U);
}

// ---------------------------------------------------------------------------
// Scenario 2: hold detection (key held down past the valid threshold).
// ---------------------------------------------------------------------------
void run_hold_detection_scenario()
{
    castle::filter::debounce_sampling<5U, 10U> key;

    uint32_t held_events = 0U;
    key.on_held([&held_events] { ++held_events; });

    for (uint32_t tick = 0U; tick < 4U; ++tick)
    {
        CASTLE_SAMPLE_CHECK(!key.sample(true));
    }
    CASTLE_SAMPLE_CHECK(key.sample(true)); // 5th identical sample -> valid
    CASTLE_SAMPLE_CHECK(key.is_valid());
    CASTLE_SAMPLE_CHECK(!key.is_held());

    for (uint32_t tick = 0U; tick < 9U; ++tick)
    {
        CASTLE_SAMPLE_CHECK(!key.sample(true));
    }
    CASTLE_SAMPLE_CHECK(key.sample(true)); // 10th sample since becoming valid -> held
    CASTLE_SAMPLE_CHECK(key.is_held());
    CASTLE_SAMPLE_CHECK(held_events == 1U);
}

// ---------------------------------------------------------------------------
// Scenario 3: repeat / auto-repeat detection while the key stays held.
// ---------------------------------------------------------------------------
void run_repeat_detection_scenario()
{
    castle::filter::debounce_sampling<5U, 10U, 3U> key;

    uint32_t repeat_events = 0U;
    key.on_repeating([&repeat_events] { ++repeat_events; });

    // Drive straight through valid (5) and hold (10) thresholds.
    for (uint32_t tick = 0U; tick < 15U; ++tick)
    {
        key.sample(true);
    }
    CASTLE_SAMPLE_CHECK(key.is_held());
    CASTLE_SAMPLE_CHECK(!key.is_repeating());

    for (uint32_t tick = 0U; tick < 2U; ++tick)
    {
        CASTLE_SAMPLE_CHECK(!key.sample(true));
    }
    CASTLE_SAMPLE_CHECK(key.sample(true)); // 3rd sample since held -> first repeat
    CASTLE_SAMPLE_CHECK(key.is_repeating());
    CASTLE_SAMPLE_CHECK(repeat_events == 1U);

    // Auto-repeat: fires again every RepeatCount samples while still pressed.
    for (uint32_t tick = 0U; tick < 2U; ++tick)
    {
        CASTLE_SAMPLE_CHECK(!key.sample(true));
    }
    CASTLE_SAMPLE_CHECK(key.sample(true));
    CASTLE_SAMPLE_CHECK(repeat_events == 2U);
}

// ---------------------------------------------------------------------------
// Scenario 4: callback registration and removal.
// ---------------------------------------------------------------------------
void run_callback_registration_scenario()
{
    using sensor_type = castle::filter::debounce_sampling<3U>;
    sensor_type sensor;

    uint32_t calls = 0U;
    sensor.on_valid([&calls] { ++calls; });

    for (uint32_t tick = 0U; tick < 2U; ++tick)
    {
        CASTLE_SAMPLE_CHECK(!sensor.sample(true));
    }
    CASTLE_SAMPLE_CHECK(sensor.sample(true));
    CASTLE_SAMPLE_CHECK(calls == 1U);

    // Remove the callback by assigning an empty one, then reset and retrigger.
    sensor.on_valid(sensor_type::callback_type{});
    sensor.reset(false);

    for (uint32_t tick = 0U; tick < 2U; ++tick)
    {
        CASTLE_SAMPLE_CHECK(!sensor.sample(true));
    }
    CASTLE_SAMPLE_CHECK(sensor.sample(true));
    CASTLE_SAMPLE_CHECK(sensor.is_valid());
    CASTLE_SAMPLE_CHECK(calls == 1U); // unchanged: callback was removed
}

// ---------------------------------------------------------------------------
// Scenario 5: integration into a periodic task / scheduler tick.
// A GPIO poll function is sampled once per tick, exactly as it would be from
// a timer ISR or an RTOS periodic task, and toggles an LED on each confirmed
// press.
// ---------------------------------------------------------------------------
CASTLE_CONST bool g_gpio_waveform[] = {
    false, true, false, true, true, true, true, true, true, true
};

bool poll_gpio_pin(uint32_t tick_index) noexcept
{
    return g_gpio_waveform[tick_index];
}

void run_scheduler_tick_scenario()
{
    castle::filter::debounce_sampling<5U> button_task;
    bool led_state = false;

    for (uint32_t tick = 0U;
         tick < (sizeof(g_gpio_waveform) / sizeof(g_gpio_waveform[0]));
         ++tick)
    {
        // This is the body a scheduler/tick-ISR would run once per period.
        if (button_task.sample(poll_gpio_pin(tick)) && button_task.is_valid())
        {
            led_state = !led_state;
        }
    }

    CASTLE_SAMPLE_CHECK(led_state);
}

} // namespace

int main()
{
    run_button_debounce_scenario();
    run_hold_detection_scenario();
    run_repeat_detection_scenario();
    run_callback_registration_scenario();
    run_scheduler_tick_scenario();
    return 0;
}
