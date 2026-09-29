#include <assert.h>

#include "castle_ext/io/gpio/gpio.hpp"
#include "castle_ext/io/gpio/backends/mock.hpp"

int main()
{
    using namespace castle::io::gpio;

    mock::pin_state pin_memory(false, level::low);
    pin<mock::backend> output(&pin_memory);
    const pin_config output_config(direction::output, level::low, pull::none, drive::push_pull, true);
    assert(output.configure(output_config) == castle::status::ok);
    assert(output.write(level::high) == castle::status::ok);
    bool pin_high = false;
    assert(output.read(pin_high) == castle::status::ok);
    assert(pin_high);

    mock::port_state port_memory(0U, 0U);
    port<mock::port_backend> port_output(&port_memory);
    assert(port_output.set(0x03U) == castle::status::ok);
    assert(port_output.write_masked(0x0CU, 0x08U) == castle::status::ok);
    mask_type port_value = 0U;
    assert(port_output.read(port_value) == castle::status::ok);
    assert(port_value == 0x0BU);
    assert(port_output.toggle(0x01U) == castle::status::ok);
    assert(port_output.clear(0x02U) == castle::status::ok);
    return 0;
}
