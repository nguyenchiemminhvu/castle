#include <gtest/gtest.h>

#include "castle_ext/protocols/lin/node.hpp"

namespace
{
using namespace castle::protocols::lin;

struct test_state
{
    uint32_t header_calls = 0U;
    uint32_t transmit_calls = 0U;
    uint32_t receive_calls = 0U;
    castle::status header_status = castle::status::ok;
    castle::status transmit_status = castle::status::ok;
    castle::status provider_status = castle::status::ok;
    frame last_frame{};
};

castle::status make_small_frame(uint8_t identifier, frame& output)
{
    const uint8_t bytes[2U] = {0x12U, 0x34U};
    return make_frame(identifier, bytes, 2U, checksum_type::enhanced, output);
}

TEST(LinMasterNode, HeaderConfigurationValidationAndCounters)
{
    master_node<> master;
    EXPECT_EQ(master.send_header(1U), castle::status::not_configured);
    EXPECT_EQ(master.header_count(), 0U);
    EXPECT_EQ(master.send_header(62U), castle::status::invalid_argument);
    EXPECT_EQ(master.send_header(256U), castle::status::invalid_argument);
    EXPECT_EQ(master.invalid_count(), 2U);

    test_state state{};
    master.configure_header_transmitter([&state](const header& request) {
        ++state.header_calls;
        EXPECT_TRUE(request.valid());
        return state.header_status;
    });
    EXPECT_EQ(master.send_header(0x12U), castle::status::ok);
    EXPECT_EQ(master.header_count(), 1U);
    state.header_status = castle::status::full;
    EXPECT_EQ(master.send_header(0x12U), castle::status::full);
    EXPECT_EQ(master.header_count(), 1U);
    EXPECT_EQ(state.header_calls, 2U);
    master.clear_header_transmitter();
    EXPECT_EQ(master.send_header(0x12U), castle::status::not_configured);
}

TEST(LinMasterNode, MasterResponseProviderAndTransmitterValidation)
{
    master_node<> master;
    EXPECT_EQ(master.send_master_response(0x12U), castle::status::not_configured);
    EXPECT_EQ(master.send_master_response(256U), castle::status::invalid_argument);

    test_state state{};
    master.configure_response_provider([&state](uint8_t identifier, frame& output) {
        if (state.provider_status != castle::status::ok)
        {
            return state.provider_status;
        }
        return make_small_frame(identifier, output);
    });
    EXPECT_EQ(master.send_master_response(0x12U), castle::status::not_configured);
    master.configure_response_transmitter([&state](const frame& response) {
        ++state.transmit_calls;
        state.last_frame = response;
        return state.transmit_status;
    });
    state.provider_status = castle::status::not_found;
    EXPECT_EQ(master.send_master_response(0x12U), castle::status::not_found);
    state.provider_status = castle::status::ok;
    state.transmit_status = castle::status::full;
    EXPECT_EQ(master.send_master_response(0x12U), castle::status::full);
    EXPECT_EQ(master.transmitted_response_count(), 0U);
    state.transmit_status = castle::status::ok;
    EXPECT_EQ(master.send_master_response(0x12U), castle::status::ok);
    EXPECT_EQ(master.transmitted_response_count(), 1U);
    EXPECT_EQ(state.transmit_calls, 2U);

    master_node<> malformed;
    malformed.configure_response_provider([](uint8_t, frame& output) {
        return make_small_frame(0x13U, output);
    });
    malformed.configure_response_transmitter([](const frame&) { return castle::status::ok; });
    EXPECT_EQ(malformed.send_master_response(0x12U), castle::status::invalid_argument);
    EXPECT_EQ(malformed.send_master_response(62U), castle::status::invalid_argument);

    master_node<> bad_checksum;
    bad_checksum.configure_response_provider([](uint8_t identifier, frame& output) {
        const castle::status result = make_small_frame(identifier, output);
        output.checksum ^= 0x80U;
        return result;
    });
    bad_checksum.configure_response_transmitter([](const frame&) { return castle::status::ok; });
    EXPECT_EQ(bad_checksum.send_master_response(0x12U), castle::status::invalid_argument);
}

TEST(LinMasterNode, TransmitSlotsAndReceiveResponses)
{
    test_state state{};
    master_node<> master;
    master.configure_header_transmitter([&state](const header&) {
        ++state.header_calls;
        return state.header_status;
    });
    master.configure_response_provider([](uint8_t id, frame& output) { return make_small_frame(id, output); });
    master.configure_response_transmitter([&state](const frame& response) {
        ++state.transmit_calls;
        state.last_frame = response;
        return state.transmit_status;
    });

    const schedule_entry bad{62U, response_owner::master, 1U};
    EXPECT_EQ(master.transmit_slot(bad), castle::status::invalid_argument);
    const schedule_entry slave_slot{0x12U, response_owner::slave, 3U};
    EXPECT_EQ(master.transmit_slot(slave_slot), castle::status::ok);
    EXPECT_EQ(state.header_calls, 1U);
    EXPECT_EQ(state.transmit_calls, 0U);
    const schedule_entry master_slot{0x12U, response_owner::master, 3U};
    EXPECT_EQ(master.transmit_slot(master_slot), castle::status::ok);
    EXPECT_EQ(state.header_calls, 2U);
    EXPECT_EQ(state.transmit_calls, 1U);

    EXPECT_EQ(master.receive_response(state.last_frame), castle::status::not_configured);
    EXPECT_EQ(master.received_response_count(), 1U);
    uint32_t handled = 0U;
    master.configure_response_handler([&handled](const frame&) { ++handled; });
    EXPECT_EQ(master.receive_response(state.last_frame), castle::status::ok);
    EXPECT_EQ(handled, 1U);
    frame invalid = state.last_frame;
    invalid.checksum ^= 0x01U;
    EXPECT_EQ(master.receive_response(invalid), castle::status::invalid_argument);
    invalid = state.last_frame;
    invalid.data_length = 9U;
    EXPECT_EQ(master.receive_response(invalid), castle::status::invalid_argument);
    EXPECT_EQ(master.invalid_count(), 3U); // invalid slot plus both invalid responses
    master.clear_response_handler();
    EXPECT_EQ(master.receive_response(state.last_frame), castle::status::not_configured);
    master.clear_response_provider();
    master.clear_response_transmitter();
}

TEST(LinSlaveNode, HeaderHandlingAndProviderResults)
{
    slave_node<> slave;
    header request{};
    ASSERT_EQ(make_header(0x12U, request), castle::status::ok);
    frame output{};
    EXPECT_EQ(slave.handle_header(request, output), castle::status::not_configured);

    header bad = request;
    bad.protected_identifier ^= 0x40U;
    EXPECT_EQ(slave.handle_header(bad, output), castle::status::invalid_argument);
    EXPECT_EQ(slave.invalid_count(), 1U);

    slave.configure_response_provider([](uint8_t id, frame& value) {
        if (id != 0x12U)
        {
            return castle::status::not_found;
        }
        return make_small_frame(id, value);
    });
    EXPECT_EQ(slave.handle_header(request, output), castle::status::ok);
    EXPECT_TRUE(validate_checksum(output));
    EXPECT_EQ(slave.prepared_response_count(), 1U);
    EXPECT_EQ(slave.handle_protected_identifier(request.protected_identifier, output), castle::status::ok);
    EXPECT_EQ(slave.prepared_response_count(), 2U);
    header other{};
    ASSERT_EQ(make_header(0x13U, other), castle::status::ok);
    EXPECT_EQ(slave.handle_header(other, output), castle::status::not_found);

    slave_node<> wrong_id;
    wrong_id.configure_response_provider([](uint8_t, frame& value) { return make_small_frame(0x13U, value); });
    EXPECT_EQ(wrong_id.handle_header(request, output), castle::status::invalid_argument);

    slave_node<> wrong_checksum;
    wrong_checksum.configure_response_provider([](uint8_t id, frame& value) {
        const castle::status result = make_small_frame(id, value);
        value.checksum ^= 1U;
        return result;
    });
    EXPECT_EQ(wrong_checksum.handle_header(request, output), castle::status::invalid_argument);
    EXPECT_EQ(slave.handle_protected_identifier(0x12U, output), castle::status::invalid_argument);
    EXPECT_EQ(slave.invalid_count(), 2U);
    slave.clear_response_provider();
    EXPECT_EQ(slave.handle_header(request, output), castle::status::not_configured);
}

TEST(LinSlaveNode, ReceiveResponsesAndCallbackStorage)
{
    slave_node<> slave;
    frame response{};
    ASSERT_EQ(make_small_frame(0x12U, response), castle::status::ok);
    EXPECT_EQ(slave.receive_response(response), castle::status::not_configured);
    EXPECT_EQ(slave.received_response_count(), 1U);
    uint32_t calls = 0U;
    slave.configure_response_handler([&calls](const frame&) { ++calls; });
    EXPECT_EQ(slave.receive_response(response), castle::status::ok);
    EXPECT_EQ(calls, 1U);
    frame invalid = response;
    invalid.checksum ^= 0x80U;
    EXPECT_EQ(slave.receive_response(invalid), castle::status::invalid_argument);
    invalid = response;
    invalid.identifier = 62U;
    EXPECT_EQ(slave.receive_response(invalid), castle::status::invalid_argument);
    slave.clear_response_handler();
    EXPECT_EQ(slave.receive_response(response), castle::status::not_configured);

    slave_node<32U, 8U> small_storage;
    small_storage.configure_response_provider(&make_small_frame);
    header request{};
    ASSERT_EQ(make_header(0x12U, request), castle::status::ok);
    EXPECT_EQ(small_storage.handle_header(request, response), castle::status::ok);
}
} // namespace
