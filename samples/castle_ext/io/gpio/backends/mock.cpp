#include <assert.h>

#include "castle_ext/io/gpio/backends/mock.hpp"

int main()
{
    using namespace castle::io::gpio;

    mock::pin_state pin_state(true, level::low);
    const pin_config config(direction::output, level::low, pull::up, drive::open_drain, true);
    assert(mock::backend::configure(&pin_state, config) == castle::status::ok);
    assert(pin_state.configured);
    assert(mock::backend::write(&pin_state, level::high) == castle::status::ok);
    level logical = level::low;
    assert(mock::backend::read(&pin_state, logical) == castle::status::ok);
    assert(logical == level::high);
    assert(mock::backend::write_raw(&pin_state, level::high) == castle::status::ok);
    assert(mock::backend::read_raw(&pin_state, logical) == castle::status::ok);
    assert(logical == level::high);
    assert(mock::backend::toggle(&pin_state) == castle::status::ok);

    mock::port_state port_state(0U, 0U);
    assert(mock::port_backend::set(&port_state, 0x03U) == castle::status::ok);
    assert(mock::port_backend::write_masked(&port_state, 0x0CU, 0x08U) == castle::status::ok);
    mask_type port_value = 0U;
    assert(mock::port_backend::read(&port_state, port_value) == castle::status::ok);
    assert(port_value == 0x0BU);
    assert(mock::port_backend::read_raw(&port_state, port_value) == castle::status::ok);
    assert(port_value == 0x0BU);
    assert(mock::port_backend::clear(&port_state, 0x02U) == castle::status::ok);
    assert(mock::port_backend::toggle(&port_state, 0x01U) == castle::status::ok);
    return 0;
}
