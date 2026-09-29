#include <assert.h>
#include <stdint.h>

#include "castle_ext/protocols/can/node.hpp"

int main()
{
    using namespace castle::protocols::can;
    uint32_t transmit_calls = 0U;
    uint32_t receive_calls = 0U;

    sender_node<> sender;
    sender.configure([&transmit_calls](const frame&) {
        ++transmit_calls;
        return castle::status::ok;
    });
    receiver_node<> receiver;
    receiver.configure([&receive_calls](const frame&) { ++receive_calls; });

    const uint8_t data_byte = 0xA5U;
    frame value{};
    assert(castle::succeeded(make_data_frame(0x120U, identifier_format::standard,
        frame_format::classical, &data_byte, 1U, value)));
    assert(castle::succeeded(sender.send(value)));
    assert(sender.sent_count() == 1U && transmit_calls == 1U);
    assert(castle::succeeded(receiver.receive(value)));
    assert(receiver.received_count() == 1U && receive_calls == 1U);

    sender_node<> unconfigured_sender;
    assert(unconfigured_sender.send(value) == castle::status::not_configured);
    frame bad = value;
    bad.data_length = 9U;
    assert(receiver.receive(bad) == castle::status::invalid_argument);
    assert(receiver.invalid_count() == 1U);
    return 0;
}
