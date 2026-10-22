#include "sample_support.hpp"

#include "castle_ext/comm/can/backends/mock.hpp"
#include "castle_ext/comm/can/bus.hpp"

using namespace castle::comm::can;

// Application code depends only on bus<Backend>; it neither knows nor cares whether the
// backend is SocketCAN, Zephyr or the host-side mock.
template <typename Backend>
static castle::status send_heartbeat(bus<Backend>& can, uint8_t counter)
{
    return can.send(0x701U, &counter, 1U);
}

// Drains everything the filters let through and returns how many frames arrived.
template <typename Backend>
static castle::size_type drain(bus<Backend>& can)
{
    castle::size_type received = 0U;
    frame rx;
    while (can.receive(rx, no_wait) == castle::status::ok)
    {
        ++received;
    }
    return received;
}

int main()
{
    // The facade adds no storage beyond the backend it owns.
    static_assert(sizeof(bus<mock::backend>) == sizeof(mock::backend), "basic_bus adds no hidden storage");
    static_assert(castle::meta::is_same<bus<mock::backend>::backend_type, mock::backend>::value, "");
    static_assert(castle::meta::is_same<bus<mock::backend>::endpoint_type, const char*>::value, "");
    static_assert(detail::has_bus_backend_contract<mock::backend>::value, "mock satisfies the backend contract");
    static_assert(!detail::has_bus_backend_contract<int>::value, "arbitrary types do not");

    bus<mock::backend> can;
    CASTLE_SAMPLE_CHECK(!can.is_open());

    // Closed bus: every operation reports not_configured (open() is the only way in).
    frame rx;
    bus_status status;
    CASTLE_SAMPLE_CHECK(can.send(frame(0x1U)) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.receive(rx) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.accept_all() == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.query_status(status) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(can.close() == castle::status::not_configured);

    // The facade validates the configuration before the backend is touched.
    CASTLE_SAMPLE_CHECK(can.open("can0", can_config(max_bitrate + 1U)) == castle::status::invalid_config);
    CASTLE_SAMPLE_CHECK(!can.is_open());

    // Open with a real configuration; the optional system_error output is cleared on entry.
    int error = 42;
    CASTLE_SAMPLE_CHECK(can.open("can0", can_config(bitrate_500k), &error) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(error == 0);
    CASTLE_SAMPLE_CHECK(can.is_open());
    CASTLE_SAMPLE_CHECK(can.open("can0") == castle::status::invalid_config); // already open
    CASTLE_SAMPLE_CHECK(can.backend().configuration().bitrate == bitrate_500k);

    // No filter is installed after open(), so nothing is received yet.
    CASTLE_SAMPLE_CHECK(can.backend().inject(frame(0x123U)) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(can.receive(rx) == castle::status::empty);

    // Targeted filters: add_filter() reports a handle for remove_filter().
    filter_handle engine = invalid_filter;
    filter_handle gearbox = invalid_filter;
    CASTLE_SAMPLE_CHECK(can.add_filter(filter::exact(0x123U), &engine) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.add_filter(filter(0x200U, 0x7F0U), &gearbox) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(engine != invalid_filter && gearbox != invalid_filter && engine != gearbox);
    CASTLE_SAMPLE_CHECK(can.add_filter(filter::exact(0x7FFU)) == castle::status::ok); // handle is optional
    CASTLE_SAMPLE_CHECK(can.backend().filter_count() == 3U);

    // Invalid filters are rejected by the facade, and the handle output is reset.
    filter_handle rejected = 7;
    CASTLE_SAMPLE_CHECK(can.add_filter(filter(max_standard_id + 1U, 0U), &rejected) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(rejected == invalid_filter);

    // Frame in -> frame out through the receive path.
    frame wire(0x123U);
    CASTLE_SAMPLE_CHECK(wire.set_data({0x11U, 0x22U}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.backend().inject(wire) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.backend().inject(frame(0x20AU)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.backend().inject(frame(0x300U)) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(can.receive(rx, timeout_type(10)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(rx == wire);
    CASTLE_SAMPLE_CHECK(drain(can) == 1U);
    CASTLE_SAMPLE_CHECK(can.receive(rx, timeout_type(10)) == castle::status::empty);
    CASTLE_SAMPLE_CHECK(can.receive(rx, timeout_type(-1)) == castle::status::invalid_argument);

    // send(): frame overload...
    frame command(0x321U);
    CASTLE_SAMPLE_CHECK(command.set_data({0xCAU, 0xFEU}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.send(command) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.send(command, timeout_type(5), &error) == castle::status::ok);
    frame sent;
    CASTLE_SAMPLE_CHECK(can.backend().pop_sent(sent) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(sent == command);

    // ...and the convenience overload that builds the frame from id + bytes (standard or extended).
    const uint8_t bytes[3] = {7U, 8U, 9U};
    CASTLE_SAMPLE_CHECK(can.send(0x18FF0001U, bytes, sizeof(bytes), id_format::extended) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.send(0x0A0U, nullptr, 0U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(send_heartbeat(can, 5U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.backend().sent_count() == 4U);
    CASTLE_SAMPLE_CHECK(can.backend().pop_sent(sent) == castle::status::ok); // second copy of command
    CASTLE_SAMPLE_CHECK(sent == command);
    CASTLE_SAMPLE_CHECK(can.backend().pop_sent(sent) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(sent.is_extended() && sent.id == 0x18FF0001U && sent.payload().size() == 3U);
    CASTLE_SAMPLE_CHECK(sent.payload()[2] == 9U);

    // Bad arguments are caught before the backend sees them.
    CASTLE_SAMPLE_CHECK(can.send(frame(max_standard_id + 1U)) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(can.send(command, timeout_type(-5)) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(can.send(0x1U, bytes, max_data_length + 1U) == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(can.send(0x1U, nullptr, 2U) == castle::status::invalid_argument);

    // Backend failures surface as statuses; the controller state is visible via query_status().
    can.backend().fail_next_send(castle::status::full);
    CASTLE_SAMPLE_CHECK(can.send(command) == castle::status::full);
    can.backend().set_bus_state(bus_state::error_warning, 100U, 0U);
    CASTLE_SAMPLE_CHECK(can.query_status(status) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(status.state == bus_state::error_warning && is_operational(status.state));
    CASTLE_SAMPLE_CHECK(status.tx_error_counter == 100U);
    CASTLE_SAMPLE_CHECK(status.tx_failures == 1U);

    // Removing filters: by handle, then all at once.
    CASTLE_SAMPLE_CHECK(can.remove_filter(engine) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.remove_filter(engine) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(can.backend().inject(wire) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(can.clear_filters() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.backend().filter_count() == 0U);

    // accept_all() installs one catch-all per identifier format (two slots).
    CASTLE_SAMPLE_CHECK(can.accept_all() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.backend().filter_count() == 2U);
    CASTLE_SAMPLE_CHECK(can.backend().inject(frame(0x5AU)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(can.backend().inject(frame(0x1ABCDEF0U, id_format::extended)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(drain(can) == 2U);

    // If the second slot is missing, accept_all() rolls the first one back.
    basic_bus<mock::basic_backend<4U, 4U, 1U>> narrow;
    CASTLE_SAMPLE_CHECK(narrow.open("can1") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(narrow.accept_all() == castle::status::full);
    CASTLE_SAMPLE_CHECK(narrow.backend().filter_count() == 0U);
    CASTLE_SAMPLE_CHECK(narrow.close() == castle::status::ok);

    // close() releases the controller; queued frames and filters are gone.
    CASTLE_SAMPLE_CHECK(can.close(&error) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!can.is_open());
    CASTLE_SAMPLE_CHECK(can.receive(rx) == castle::status::not_configured);

    // const access to the backend for read-only inspection.
    const bus<mock::backend>& view = can;
    CASTLE_SAMPLE_CHECK(view.backend().filter_count() == 0U);
    CASTLE_SAMPLE_CHECK(!view.is_open());

    return 0;
}
