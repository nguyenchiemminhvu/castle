#include <assert.h>
#include <stdint.h>

#include "castle_ext/protocols/lin/node.hpp"

int main()
{
    using namespace castle::protocols::lin;
    uint32_t headers = 0U;
    uint32_t transmitted = 0U;
    uint32_t master_received = 0U;
    const uint8_t payload[2U] = {0x21U, 0x43U};
    frame last_transmitted{};

    master_node<> master;
    master.configure_header_transmitter([&headers](const header& request) {
        assert(request.valid());
        ++headers;
        return castle::status::ok;
    });
    master.configure_response_provider([&payload](uint8_t identifier, frame& output) {
        if (identifier != 0x12U)
        {
            return castle::status::not_found;
        }
        return make_frame(identifier, payload, 2U, checksum_type::enhanced, output);
    });
    master.configure_response_transmitter([&transmitted, &last_transmitted](const frame& response) {
        assert(validate_checksum(response));
        ++transmitted;
        last_transmitted = response;
        return castle::status::ok;
    });
    master.configure_response_handler([&master_received](const frame& response) {
        assert(validate_checksum(response));
        ++master_received;
    });

    const schedule_entry master_slot{0x12U, response_owner::master, 10U};
    assert(castle::succeeded(master.transmit_slot(master_slot)));
    assert(headers == 1U && transmitted == 1U);
    assert(master.header_count() == 1U && master.transmitted_response_count() == 1U);
    assert(castle::succeeded(master.receive_response(last_transmitted)));
    assert(master_received == 1U && master.received_response_count() == 1U);
    assert(castle::succeeded(master.send_header(0x12U)));
    assert(castle::succeeded(master.send_master_response(0x12U)));
    assert(master.header_count() == 2U && master.transmitted_response_count() == 2U);

    slave_node<> slave;
    slave.configure_response_provider([&payload](uint8_t identifier, frame& output) {
        if (identifier != 0x13U)
        {
            return castle::status::not_found;
        }
        return make_frame(identifier, payload, 2U, checksum_type::enhanced, output);
    });
    header request{};
    assert(castle::succeeded(make_header(0x13U, request)));
    frame response{};
    assert(castle::succeeded(slave.handle_header(request, response)));
    assert(slave.prepared_response_count() == 1U && validate_checksum(response));
    assert(castle::succeeded(slave.handle_protected_identifier(request.protected_identifier, response)));
    assert(slave.prepared_response_count() == 2U);

    uint32_t slave_received = 0U;
    slave.configure_response_handler([&slave_received](const frame&) { ++slave_received; });
    assert(castle::succeeded(slave.receive_response(response)));
    assert(slave_received == 1U && slave.received_response_count() == 1U);

    master.clear_response_handler();
    master.clear_response_provider();
    master.clear_response_transmitter();
    master.clear_header_transmitter();
    assert(master.receive_response(last_transmitted) == castle::status::not_configured);
    assert(master.send_header(0x12U) == castle::status::not_configured);
    assert(master.send_master_response(0x12U) == castle::status::not_configured);
    slave.clear_response_handler();
    slave.clear_response_provider();
    assert(slave.handle_header(request, response) == castle::status::not_configured);
    assert(slave.receive_response(response) == castle::status::not_configured);
    return 0;
}
