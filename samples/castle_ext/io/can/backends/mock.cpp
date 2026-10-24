#include "sample_support.hpp"

#include <errno.h>

#include "castle_ext/io/can/backends/mock.hpp"

using namespace castle::io::can;

int main()
{
    // The backend is used directly here (the bus facade would add argument validation on top).
    mock::backend node;
    static_assert(sizeof(mock::backend::endpoint_type) == sizeof(const char*), "endpoints are C strings");

    // Not open: every operation reports not_configured.
    frame scratch;
    bus_status status;
    filter_handle handle = invalid_filter;
    CASTLE_SAMPLE_CHECK(!node.is_open());
    CASTLE_SAMPLE_CHECK(node.send(frame(0x1U), no_wait, nullptr) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(node.receive(scratch, no_wait, nullptr) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(node.add_filter(filter::any(), handle, nullptr) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(node.query_status(status, nullptr) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(node.inject(frame(0x1U)) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(node.close(nullptr) == castle::status::not_configured);

    // open(): the endpoint must be a non-empty name; a second open() is rejected.
    CASTLE_SAMPLE_CHECK(node.open(nullptr, can_config(), nullptr) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(node.open("", can_config(), nullptr) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(node.open("mock0", can_config(bitrate_500k), nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.is_open());
    CASTLE_SAMPLE_CHECK(node.configuration().bitrate == bitrate_500k);
    CASTLE_SAMPLE_CHECK(node.open("mock0", can_config(), nullptr) == castle::status::invalid_config);
    CASTLE_SAMPLE_CHECK(node.query_status(status, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(status.state == bus_state::error_active);

    // Like real hardware, nothing is received before a filter is installed.
    CASTLE_SAMPLE_CHECK(node.filter_count() == 0U);
    CASTLE_SAMPLE_CHECK(node.inject(frame(0x123U)) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(node.receive(scratch, no_wait, nullptr) == castle::status::empty);

    // inject() plays the bus: only frames passing an installed filter are buffered.
    CASTLE_SAMPLE_CHECK(node.add_filter(filter::exact(0x123U), handle, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(handle != invalid_filter);
    CASTLE_SAMPLE_CHECK(node.filter_count() == 1U);
    frame incoming(0x123U);
    CASTLE_SAMPLE_CHECK(incoming.set_data({0xDEU, 0xADU}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.inject(incoming) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.inject(frame(0x124U)) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(node.receive(scratch, no_wait, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(scratch == incoming);
    CASTLE_SAMPLE_CHECK(node.receive(scratch, no_wait, nullptr) == castle::status::empty);

    // remove_filter(): unknown handles report not_found, removed filters stop accepting.
    CASTLE_SAMPLE_CHECK(node.remove_filter(handle, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.remove_filter(handle, nullptr) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(node.inject(incoming) == castle::status::not_found);

    // clear_filters() drops everything at once.
    filter_handle block_handle = invalid_filter;
    CASTLE_SAMPLE_CHECK(node.add_filter(filter(0x100U, 0x7F0U), block_handle, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.add_filter(filter::any(id_format::extended), handle, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.filter_count() == 2U);
    CASTLE_SAMPLE_CHECK(node.inject(frame(0x10FU)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.inject(frame(0x1FFFFFFFU, id_format::extended)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.clear_filters(nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.filter_count() == 0U);

    // Transmitted frames wait in a queue that the test reads back with pop_sent().
    frame out(0x321U);
    CASTLE_SAMPLE_CHECK(out.set_data({1U, 2U, 3U}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.sent_count() == 0U);
    CASTLE_SAMPLE_CHECK(node.send(out, no_wait, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.sent_count() == 1U);
    frame wire;
    CASTLE_SAMPLE_CHECK(node.pop_sent(wire) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(wire == out);
    CASTLE_SAMPLE_CHECK(node.pop_sent(wire) == castle::status::empty);

    // fail_next_send(): a one-shot scripted failure that is counted in tx_failures.
    node.fail_next_send(castle::status::full);
    CASTLE_SAMPLE_CHECK(node.send(out, no_wait, nullptr) == castle::status::full);
    CASTLE_SAMPLE_CHECK(node.send(out, no_wait, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.query_status(status, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(status.tx_failures == 1U);
    node.fail_next_send(castle::status::system_call_error);
    node.fail_next_send(castle::status::ok); // disarm again
    CASTLE_SAMPLE_CHECK(node.send(out, no_wait, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.sent_count() == 2U);

    // set_bus_state() scripts what query_status() reports; bus-off blocks transmission.
    node.set_bus_state(bus_state::error_passive, 130U, 12U);
    CASTLE_SAMPLE_CHECK(node.query_status(status, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(status.state == bus_state::error_passive);
    CASTLE_SAMPLE_CHECK(status.tx_error_counter == 130U && status.rx_error_counter == 12U);
    CASTLE_SAMPLE_CHECK(!is_operational(status.state));
    node.set_bus_state(bus_state::bus_off, 255U);
    int error = 0;
    CASTLE_SAMPLE_CHECK(node.send(out, no_wait, &error) == castle::status::system_call_error);
    CASTLE_SAMPLE_CHECK(error == ENETUNREACH);

    // close() discards queued frames and state; the node can then be reopened.
    CASTLE_SAMPLE_CHECK(node.close(nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!node.is_open());
    CASTLE_SAMPLE_CHECK(node.sent_count() == 0U);
    CASTLE_SAMPLE_CHECK(node.open("mock0", can_config(), nullptr) == castle::status::ok);

    // listen_only nodes never transmit.
    CASTLE_SAMPLE_CHECK(node.close(nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.open("mock0", can_config(bitrate_500k, can_mode::listen_only), nullptr)
                        == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.send(out, no_wait, nullptr) == castle::status::invalid_config);
    CASTLE_SAMPLE_CHECK(node.close(nullptr) == castle::status::ok);

    // loopback delivers transmitted frames straight to the node's own filters.
    CASTLE_SAMPLE_CHECK(node.open("mock0", can_config(0U, can_mode::loopback), nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.add_filter(filter::any(), handle, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.send(out, no_wait, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(node.sent_count() == 0U);
    CASTLE_SAMPLE_CHECK(node.receive(scratch, no_wait, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(scratch == out);
    CASTLE_SAMPLE_CHECK(node.close(nullptr) == castle::status::ok);

    // Queue and filter depths are template parameters; overflows are reported and counted.
    mock::basic_backend<2U, 2U, 2U> tiny;
    CASTLE_SAMPLE_CHECK(tiny.open("tiny0", can_config(), nullptr) == castle::status::ok);
    filter_handle tiny_handles[2] = {invalid_filter, invalid_filter};
    CASTLE_SAMPLE_CHECK(tiny.add_filter(filter::any(), tiny_handles[0], nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tiny.add_filter(filter::any(id_format::extended), tiny_handles[1], nullptr)
                        == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tiny.add_filter(filter::exact(1U), handle, nullptr) == castle::status::full);
    CASTLE_SAMPLE_CHECK(handle == invalid_filter);

    CASTLE_SAMPLE_CHECK(tiny.inject(frame(1U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tiny.inject(frame(2U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tiny.inject(frame(3U)) == castle::status::full);
    CASTLE_SAMPLE_CHECK(tiny.send(out, no_wait, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tiny.send(out, no_wait, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tiny.send(out, no_wait, nullptr) == castle::status::full);
    CASTLE_SAMPLE_CHECK(tiny.query_status(status, nullptr) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(status.rx_overruns == 1U);
    CASTLE_SAMPLE_CHECK(tiny.close(nullptr) == castle::status::ok);

    return 0;
}
