#include <gtest/gtest.h>

#include "castle_ext/protocols/can/node.hpp"

namespace
{
using namespace castle::protocols::can;

struct context
{
    uint32_t transmit_calls = 0U;
    uint32_t receive_calls = 0U;
    castle::status transmit_status = castle::status::ok;
    frame last_received{};
};

frame make_one_byte_frame()
{
    const uint8_t byte = 0xA5U;
    frame value{};
    (void)make_data_frame(0x120U, identifier_format::standard,
        frame_format::classical, &byte, 1U, value);
    return value;
}

TEST(CanNode, SenderConfigurationStatusAndCounters)
{
    frame value = make_one_byte_frame();
    sender_node<> sender;
    EXPECT_EQ(sender.send(value), castle::status::not_configured);
    EXPECT_EQ(sender.sent_count(), 0U);

    context state{};
    sender.configure([&state](const frame&) {
        ++state.transmit_calls;
        return state.transmit_status;
    });
    EXPECT_EQ(sender.send(value), castle::status::ok);
    EXPECT_EQ(sender.sent_count(), 1U);
    EXPECT_EQ(state.transmit_calls, 1U);

    state.transmit_status = castle::status::full;
    EXPECT_EQ(sender.send(value), castle::status::full);
    EXPECT_EQ(sender.sent_count(), 1U);
    EXPECT_EQ(state.transmit_calls, 2U);

    sender.clear();
    EXPECT_EQ(sender.send(value), castle::status::not_configured);
}

TEST(CanNode, SenderRejectsMalformedAndControllerSignalFrames)
{
    context state{};
    sender_node<> sender;
    sender.configure([&state](const frame&) {
        ++state.transmit_calls;
        return state.transmit_status;
    });
    frame value = make_one_byte_frame();
    value.data_length = 9U;
    EXPECT_EQ(sender.send(value), castle::status::invalid_argument);

    ASSERT_EQ(make_error_frame(0U, identifier_format::standard, value), castle::status::ok);
    EXPECT_EQ(sender.send(value), castle::status::invalid_argument);
    ASSERT_EQ(make_overload_frame(0U, identifier_format::standard, value), castle::status::ok);
    EXPECT_EQ(sender.send(value), castle::status::invalid_argument);
    EXPECT_EQ(state.transmit_calls, 0U);
}

TEST(CanNode, ReceiverDispatchesValidFrameAndTracksInvalid)
{
    context state{};
    receiver_node<> receiver;
    receiver.configure([&state](const frame& received) {
        ++state.receive_calls;
        state.last_received = received;
    });
    frame value = make_one_byte_frame();
    EXPECT_EQ(receiver.receive(value), castle::status::ok);
    EXPECT_EQ(receiver.received_count(), 1U);
    EXPECT_EQ(receiver.invalid_count(), 0U);
    EXPECT_EQ(state.receive_calls, 1U);
    EXPECT_EQ(state.last_received.identifier, value.identifier);

    value.data_length = 9U;
    EXPECT_EQ(receiver.receive(value), castle::status::invalid_argument);
    EXPECT_EQ(receiver.received_count(), 1U);
    EXPECT_EQ(receiver.invalid_count(), 1U);
    EXPECT_EQ(state.receive_calls, 1U);
}

TEST(CanNode, ReceiverCanAcceptSignalEventsAndUnconfiguredState)
{
    receiver_node<> receiver;
    frame signal{};
    ASSERT_EQ(make_error_frame(0U, identifier_format::standard, signal), castle::status::ok);
    EXPECT_EQ(receiver.receive(signal), castle::status::not_configured);
    EXPECT_EQ(receiver.received_count(), 1U);

    ASSERT_EQ(make_overload_frame(0U, identifier_format::standard, signal), castle::status::ok);
    context state{};
    receiver.configure([&state](const frame& received) {
        ++state.receive_calls;
        state.last_received = received;
    });
    EXPECT_EQ(receiver.receive(signal), castle::status::ok);
    EXPECT_EQ(receiver.received_count(), 2U);
    EXPECT_EQ(state.receive_calls, 1U);
}

castle::status always_ok(const frame&)
{
    return castle::status::ok;
}

TEST(CanNode, CallbacksAcceptFunctionPointersAndCustomStorage)
{
    frame value = make_one_byte_frame();
    sender_node<> sender(&always_ok);
    EXPECT_EQ(sender.send(value), castle::status::ok);
    EXPECT_EQ(sender.sent_count(), 1U);

    context state{};
    sender_node<16U, 8U> small;
    small.configure([&state](const frame&) {
        ++state.transmit_calls;
        return castle::status::ok;
    });
    EXPECT_EQ(small.send(value), castle::status::ok);
    EXPECT_EQ(state.transmit_calls, 1U);
}

} // namespace
