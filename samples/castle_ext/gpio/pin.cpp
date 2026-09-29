#include "sample_support.hpp"

#include "castle_ext/gpio/gpio.hpp"
#include "castle_ext/gpio/backends/mock.hpp"

using namespace castle::gpio;

// Generic code only ever depends on castle::gpio::pin<Backend>; it never needs to
// know whether Backend is castle::gpio::mock, castle::gpio::zephyr, or anything else.
template <typename Backend>
static void blink(pin<Backend>& led)
{
    CASTLE_SAMPLE_CHECK(castle::succeeded(led.write(true)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(led.toggle()));
}

int main()
{
    // basic_pin<Backend> adds no storage beyond the backend's own native handle.
    static_assert(sizeof(pin<mock::backend>) == sizeof(mock::backend::native_handle_type),
                  "basic_pin must not add hidden storage");

    // An active-high output pin, initially low.
    mock::pin_state led_state(/* active_low */ false, level::low);
    pin<mock::backend> led(&led_state);

    const pin_config output_cfg(direction::output, level::low, pull::none, drive::push_pull, true);
    CASTLE_SAMPLE_CHECK(castle::succeeded(led.configure(output_cfg)));

    CASTLE_SAMPLE_CHECK(castle::succeeded(led.write(level::high)));
    level observed = level::low;
    CASTLE_SAMPLE_CHECK(castle::succeeded(led.read(observed)));
    CASTLE_SAMPLE_CHECK(observed == level::high);

    CASTLE_SAMPLE_CHECK(castle::succeeded(led.toggle()));
    CASTLE_SAMPLE_CHECK(castle::succeeded(led.read(observed)));
    CASTLE_SAMPLE_CHECK(observed == level::low);

    // write(bool)/read(bool&) overloads for callers that prefer plain booleans.
    CASTLE_SAMPLE_CHECK(castle::succeeded(led.write(true)));
    bool observed_bool = false;
    CASTLE_SAMPLE_CHECK(castle::succeeded(led.read(observed_bool)));
    CASTLE_SAMPLE_CHECK(observed_bool);

    // Backend-independent helper, exercised against the mock backend here.
    blink(led);
    CASTLE_SAMPLE_CHECK(castle::succeeded(led.read(observed)));
    CASTLE_SAMPLE_CHECK(observed == level::low);

    // An active-low button: the backend flips logical vs. physical level, but
    // write_raw()/read_raw() always address the physical pin directly.
    mock::pin_state button_state(/* active_low */ true, level::high);
    pin<mock::backend> button(&button_state);
    CASTLE_SAMPLE_CHECK(castle::succeeded(button.configure(pin_config(direction::input))));

    level physical = level::low;
    CASTLE_SAMPLE_CHECK(castle::succeeded(button.read_raw(physical)));
    CASTLE_SAMPLE_CHECK(physical == level::high);

    level logical = level::low;
    CASTLE_SAMPLE_CHECK(castle::succeeded(button.read(logical)));
    CASTLE_SAMPLE_CHECK(logical == level::low); // active-low: physical high -> logical low (not pressed)

    return 0;
}

