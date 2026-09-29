#include <gtest/gtest.h>

#include "castle_ext/protocols/timing/ptp_v2.hpp"

namespace
{

using castle::container::array_view;
using castle::status;
using castle::timing::ptp::message_type;
using castle::timing::ptp::port_identity;
using castle::timing::ptp::timestamp;

void write_be16(uint8_t* data, uint16_t value)
{
    data[0U] = static_cast<uint8_t>(value >> 8U);
    data[1U] = static_cast<uint8_t>(value);
}

void write_be64(uint8_t* data, uint64_t value)
{
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        data[7U - index] = static_cast<uint8_t>(value >> (index * 8U));
    }
}

void write_timestamp(uint8_t* data, uint64_t seconds, uint32_t nanoseconds)
{
    for (castle::size_type index = 0U; index < 6U; ++index)
    {
        data[5U - index] = static_cast<uint8_t>(seconds >> (index * 8U));
    }
    data[6U] = static_cast<uint8_t>(nanoseconds >> 24U);
    data[7U] = static_cast<uint8_t>(nanoseconds >> 16U);
    data[8U] = static_cast<uint8_t>(nanoseconds >> 8U);
    data[9U] = static_cast<uint8_t>(nanoseconds);
}

void write_header(
    uint8_t* packet,
    message_type type,
    uint16_t length,
    uint16_t flags,
    port_identity const& source,
    uint16_t sequence,
    uint8_t transport_specific = 0U,
    uint8_t domain = 0U)
{
    packet[0U] = static_cast<uint8_t>(
        ((transport_specific & 0x0FU) << 4U) | static_cast<uint8_t>(type));
    packet[1U] = 2U;
    write_be16(&packet[2U], length);
    packet[4U] = domain;
    write_be16(&packet[6U], flags);
    write_be64(&packet[8U], 0U);
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        packet[20U + index] = source.clock_identity[index];
    }
    write_be16(&packet[28U], source.port_number);
    write_be16(&packet[30U], sequence);
}

void write_delay_response(
    uint8_t* packet,
    port_identity const& master,
    port_identity const& requester,
    uint16_t sequence,
    timestamp receive_time,
    int64_t correction = 0LL)
{
    write_header(packet, message_type::delay_response, 54U, 0U, master, sequence);
    write_be64(&packet[8U], static_cast<uint64_t>(correction));
    write_timestamp(&packet[34U], receive_time.seconds, receive_time.nanoseconds);
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        packet[44U + index] = requester.clock_identity[index];
    }
    write_be16(&packet[52U], requester.port_number);
}


TEST(PtpV2, TimestampUtilitiesValidateBoundsAndDifference)
{
    EXPECT_TRUE(castle::timing::ptp::valid_timestamp(timestamp(1U, 999999999U)));
    EXPECT_FALSE(castle::timing::ptp::valid_timestamp(timestamp(0U, 1000000000U)));
    EXPECT_FALSE(castle::timing::ptp::valid_timestamp(
        timestamp(castle::timing::ptp::PTP_MAX_SECONDS + 1U, 0U)));

    port_identity a{};
    port_identity b{};
    EXPECT_TRUE(castle::timing::ptp::same_port(a, b));
    b.port_number = 2U;
    EXPECT_FALSE(castle::timing::ptp::same_port(a, b));
    b.port_number = 0U;
    b.clock_identity[7U] = 1U;
    EXPECT_FALSE(castle::timing::ptp::same_port(a, b));

    EXPECT_EQ(
        castle::timing::ptp::timestamp_difference(timestamp(2U, 50U), timestamp(1U, 75U)),
        999999975LL);

    int64_t difference = 0LL;
    EXPECT_TRUE(castle::timing::ptp::timestamp_difference_checked(
        timestamp(2U, 50U), timestamp(1U, 75U), difference));
    EXPECT_EQ(difference, 999999975LL);
    EXPECT_FALSE(castle::timing::ptp::timestamp_difference_checked(
        timestamp(castle::timing::ptp::PTP_MAX_SECONDS, 0U), timestamp(0U, 0U), difference));

    EXPECT_EQ(castle::timing::ptp::correction_to_nanoseconds(131072LL), 2LL);
    EXPECT_EQ(castle::timing::ptp::correction_to_nanoseconds(-131072LL), -2LL);
}

TEST(PtpV2, EncodeDelayRequestAndDecodeHeader)
{
    port_identity source{};
    source.clock_identity[7U] = 2U;
    source.port_number = 1U;

    uint8_t packet[castle::timing::ptp::PTP_DELAY_REQUEST_SIZE] = {};
    castle::size_type written = 0U;
    EXPECT_EQ(
        castle::timing::ptp::encode_delay_request(
            7U, source, 19U, packet, sizeof(packet), written, 1U),
        status::ok);
    EXPECT_EQ(written, castle::timing::ptp::PTP_DELAY_REQUEST_SIZE);

    castle::timing::ptp::message_view message{};
    ASSERT_TRUE(castle::timing::ptp::decode_message(
        array_view<const uint8_t>(packet, written), message));
    EXPECT_EQ(message.header.transport_specific, 1U);
    EXPECT_TRUE(message.is(message_type::delay_request));
    EXPECT_EQ(message.header.domain_number, 7U);
    EXPECT_EQ(message.header.sequence_id, 19U);
    EXPECT_EQ(message.header.source_port.port_number, 1U);
    EXPECT_EQ(message.payload.size(), 10U);

    written = 123U;
    EXPECT_EQ(
        castle::timing::ptp::encode_delay_request(
            0U, source, 1U, nullptr, sizeof(packet), written),
        status::invalid_argument);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(
        castle::timing::ptp::encode_delay_request(
            0U, source, 1U, packet, 1U, written),
        status::full);
}

TEST(PtpV2, DecodeRejectsMalformedFrames)
{
    uint8_t packet[64U] = {};
    castle::timing::ptp::message_view message{};

    EXPECT_FALSE(castle::timing::ptp::decode_message(
        array_view<const uint8_t>(packet, 33U), message));

    packet[1U] = 1U;
    write_be16(&packet[2U], 34U);
    EXPECT_FALSE(castle::timing::ptp::decode_message(
        array_view<const uint8_t>(packet, 34U), message));

    packet[1U] = 2U;
    write_be16(&packet[2U], 35U);
    EXPECT_FALSE(castle::timing::ptp::decode_message(
        array_view<const uint8_t>(packet, 34U), message));

    packet[0U] = 0x05U;
    write_be16(&packet[2U], 34U);
    EXPECT_FALSE(castle::timing::ptp::decode_message(
        array_view<const uint8_t>(packet, 34U), message));

    packet[0U] = static_cast<uint8_t>(message_type::sync);
    write_be16(&packet[2U], 34U);
    EXPECT_TRUE(castle::timing::ptp::decode_message(
        array_view<const uint8_t>(packet, 34U), message));
    EXPECT_EQ(message.payload.size(), 0U);
    timestamp parsed{};
    EXPECT_FALSE(castle::timing::ptp::read_message_timestamp(message, parsed));
}

TEST(PtpV2, CompletesTwoStepExchange)
{
    port_identity local{};
    local.clock_identity[7U] = 2U;
    local.port_number = 1U;
    port_identity master{};
    master.clock_identity[7U] = 1U;
    master.port_number = 1U;

    castle::timing::ptp::slave engine(local);
    uint8_t packet[64U] = {};
    castle::timing::ptp::slave_action action{};

    write_header(packet, message_type::sync, 44U,
                 castle::timing::ptp::PTP_FLAG_TWO_STEP, master, 55U);
    write_be64(&packet[8U], 2U * 65536U);
    ASSERT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(100U, 110U), action),
        status::ok);
    ASSERT_TRUE(action.send_delay_request);
    EXPECT_EQ(action.delay_request_sequence, 0U);
    EXPECT_TRUE(engine.pending());

    ASSERT_EQ(engine.delay_request_transmitted(timestamp(100U, 200U)), status::ok);

    write_header(packet, message_type::follow_up, 44U, 0U, master, 55U);
    write_timestamp(&packet[34U], 100U, 10U);
    ASSERT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(100U, 120U), action),
        status::ok);
    EXPECT_FALSE(action.measurement_ready);

    write_delay_response(packet, master, local, action.delay_request_sequence,
                         timestamp(100U, 260U));
    ASSERT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 54U), timestamp(100U, 300U), action),
        status::ok);
    ASSERT_TRUE(action.measurement_ready);
    EXPECT_EQ(action.sample.sync_sequence_id, 55U);
    EXPECT_EQ(action.sample.offset_nanoseconds, 19LL);
    EXPECT_EQ(action.sample.path_delay_nanoseconds, 79LL);
    EXPECT_EQ(action.sample.correction_nanoseconds, 2LL);
    EXPECT_TRUE(action.sample.two_step);
    EXPECT_EQ(engine.last_measurement().offset_nanoseconds, 19LL);
    EXPECT_FALSE(engine.pending());
}

TEST(PtpV2, CompletesWhenDelayResponseArrivesBeforeTxTimestamp)
{
    port_identity local{};
    local.port_number = 1U;
    port_identity master{};
    master.clock_identity[7U] = 1U;
    master.port_number = 1U;

    castle::timing::ptp::slave engine(local);
    uint8_t packet[64U] = {};
    castle::timing::ptp::slave_action action{};

    write_header(packet, message_type::sync, 44U, 0U, master, 7U);
    write_timestamp(&packet[34U], 10U, 100U);
    ASSERT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(10U, 150U), action),
        status::ok);
    const uint16_t delay_sequence = action.delay_request_sequence;

    write_delay_response(packet, master, local, delay_sequence, timestamp(10U, 250U));
    ASSERT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 54U), timestamp(10U, 300U), action),
        status::ok);
    EXPECT_FALSE(action.measurement_ready);
    EXPECT_TRUE(engine.pending());

    ASSERT_EQ(
        engine.delay_request_transmitted(timestamp(10U, 200U), action),
        status::ok);
    ASSERT_TRUE(action.measurement_ready);
    EXPECT_EQ(action.sample.offset_nanoseconds, 0LL);
    EXPECT_EQ(action.sample.path_delay_nanoseconds, 50LL);
    EXPECT_FALSE(action.sample.two_step);
}

TEST(PtpV2, FiltersDomainTransportIdentityAndSequence)
{
    port_identity local{};
    local.port_number = 1U;
    port_identity master{};
    master.clock_identity[7U] = 1U;
    master.port_number = 1U;
    port_identity other = master;
    other.clock_identity[7U] = 9U;

    castle::timing::ptp::slave_config config{};
    config.domain_number = 4U;
    config.transport_specific = 1U;
    castle::timing::ptp::slave engine(local, config);
    uint8_t packet[64U] = {};
    castle::timing::ptp::slave_action action{};

    write_header(packet, message_type::sync, 44U, 0U, master, 1U, 0U, 4U);
    write_timestamp(&packet[34U], 1U, 0U);
    EXPECT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(1U, 1U), action),
        status::not_found);

    write_header(packet, message_type::sync, 44U, 0U, master, 1U, 1U, 3U);
    write_timestamp(&packet[34U], 1U, 0U);
    EXPECT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(1U, 1U), action),
        status::not_found);

    write_header(packet, message_type::sync, 44U, 0U, master, 1U, 1U, 4U);
    write_timestamp(&packet[34U], 1U, 0U);
    EXPECT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(1U, 1U), action),
        status::ok);
    EXPECT_TRUE(engine.pending());

    write_header(packet, message_type::delay_response, 54U, 0U, other,
                 action.delay_request_sequence, 1U, 4U);
    write_timestamp(&packet[34U], 1U, 2U);
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        packet[44U + index] = local.clock_identity[index];
    }
    write_be16(&packet[52U], local.port_number);
    EXPECT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 54U), timestamp(1U, 3U), action),
        status::not_found);
    EXPECT_TRUE(engine.pending());

    EXPECT_EQ(engine.delay_request_transmitted(timestamp(1U, 2U)), status::ok);
    EXPECT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 53U), timestamp(1U, 3U), action),
        status::invalid_argument);
}

TEST(PtpV2, RejectsInvalidTimestampsAndMismatchedFollowUp)
{
    port_identity local{};
    local.port_number = 1U;
    port_identity master{};
    master.clock_identity[7U] = 1U;
    master.port_number = 1U;

    castle::timing::ptp::slave engine(local);
    uint8_t packet[64U] = {};
    castle::timing::ptp::slave_action action{};

    write_header(packet, message_type::sync, 44U,
                 castle::timing::ptp::PTP_FLAG_TWO_STEP, master, 22U);
    EXPECT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(1U, 1000000000U), action),
        status::invalid_argument);

    write_timestamp(&packet[34U], 1U, 0U);
    port_identity other = master;
    other.clock_identity[7U] = 2U;
    write_header(packet, message_type::follow_up, 44U, 0U, other, 22U);
    write_timestamp(&packet[34U], 1U, 0U);
    EXPECT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(1U, 2U), action),
        status::not_found);

    write_header(packet, message_type::sync, 44U,
                 castle::timing::ptp::PTP_FLAG_TWO_STEP, master, 22U);
    write_timestamp(&packet[34U], 1U, 0U);
    ASSERT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(1U, 2U), action),
        status::ok);

    write_header(packet, message_type::follow_up, 44U, 0U, master, 99U);
    EXPECT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(1U, 3U), action),
        status::not_found);

    write_header(packet, message_type::follow_up, 44U, 0U, master, 22U);
    write_timestamp(&packet[34U], 1U, 1000000000U);
    EXPECT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(1U, 3U), action),
        status::invalid_argument);

    engine.reset();
    EXPECT_FALSE(engine.pending());
    EXPECT_EQ(engine.delay_request_transmitted(timestamp(1U, 1U)), status::not_configured);
}

TEST(PtpV2, HandlesDelayResponseIdentityLengthAndSignedCorrection)
{
    port_identity local{};
    local.port_number = 1U;
    port_identity master{};
    master.clock_identity[7U] = 1U;
    master.port_number = 1U;

    castle::timing::ptp::slave engine(local);
    uint8_t packet[64U] = {};
    castle::timing::ptp::slave_action action{};

    write_header(packet, message_type::sync, 44U, 0U, master, 3U);
    write_timestamp(&packet[34U], 10U, 100U);
    ASSERT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 44U), timestamp(10U, 150U), action),
        status::ok);

    write_header(packet, message_type::delay_response, 54U, 0U, master,
                 action.delay_request_sequence);
    write_timestamp(&packet[34U], 10U, 250U);
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        packet[44U + index] = local.clock_identity[index];
    }
    write_be16(&packet[52U], 9U);
    EXPECT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 54U), timestamp(10U, 300U), action),
        status::not_found);

    write_be16(&packet[52U], local.port_number);
    write_be16(&packet[2U], 53U);
    EXPECT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 53U), timestamp(10U, 300U), action),
        status::invalid_argument);

    write_be16(&packet[2U], 54U);
    write_be64(&packet[8U], static_cast<uint64_t>(-65536LL));
    ASSERT_EQ(
        engine.receive(array_view<const uint8_t>(packet, 54U), timestamp(10U, 300U), action),
        status::ok);
    EXPECT_FALSE(action.measurement_ready);
    ASSERT_EQ(engine.delay_request_transmitted(timestamp(10U, 200U), action), status::ok);
    ASSERT_TRUE(action.measurement_ready);
    EXPECT_EQ(action.sample.correction_nanoseconds, -1LL);
}

} // namespace
