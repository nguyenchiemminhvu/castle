#include <stdint.h>

#include "sample_support.hpp"
#include "castle_ext/protocols/timing/ptp_v2.hpp"

namespace
{

using castle::container::array_view;
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
    uint16_t sequence)
{
    packet[0U] = static_cast<uint8_t>(type);
    packet[1U] = 2U;
    write_be16(&packet[2U], length);
    write_be16(&packet[6U], flags);
    write_be64(&packet[8U], 0U);
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        packet[20U + index] = source.clock_identity[index];
    }
    write_be16(&packet[28U], source.port_number);
    write_be16(&packet[30U], sequence);
}

} // namespace

int main()
{
    port_identity local{};
    local.clock_identity[7U] = 2U;
    local.port_number = 1U;

    port_identity master{};
    master.clock_identity[7U] = 1U;
    master.port_number = 1U;

    castle::timing::ptp::slave_config config{};
    config.initial_delay_request_sequence = 41U;
    castle::timing::ptp::slave engine(local, config);
    CASTLE_SAMPLE_CHECK(engine.local_port().port_number == 1U);
    CASTLE_SAMPLE_CHECK(engine.domain_number() == 0U);
    CASTLE_SAMPLE_CHECK(castle::timing::ptp::same_port(local, local));
    CASTLE_SAMPLE_CHECK(castle::timing::ptp::valid_timestamp(timestamp(0U, 0U)));
    CASTLE_SAMPLE_CHECK(!castle::timing::ptp::valid_timestamp(
        timestamp(castle::timing::ptp::PTP_MAX_SECONDS, 1000000000U)));

    int64_t difference = 0LL;
    CASTLE_SAMPLE_CHECK(castle::timing::ptp::timestamp_difference_checked(
        timestamp(10U, 20U), timestamp(10U, 5U), difference));
    CASTLE_SAMPLE_CHECK(difference == 15LL);
    CASTLE_SAMPLE_CHECK(castle::timing::ptp::timestamp_difference(
        timestamp(11U, 0U), timestamp(10U, 0U)) == 1000000000LL);
    CASTLE_SAMPLE_CHECK(castle::timing::ptp::correction_to_nanoseconds(131072LL) == 2LL);

    uint8_t packet[64U] = {};
    castle::timing::ptp::slave_action action{};

    write_header(
        packet,
        message_type::sync,
        44U,
        castle::timing::ptp::PTP_FLAG_TWO_STEP,
        master,
        7U);
    write_be64(&packet[8U], static_cast<uint64_t>(2U * 65536U));

    CASTLE_SAMPLE_CHECK(engine.receive(
        array_view<const uint8_t>(packet, 44U),
        timestamp(100U, 110U),
        action) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(action.send_delay_request);
    CASTLE_SAMPLE_CHECK(action.delay_request_sequence == 41U);
    CASTLE_SAMPLE_CHECK(!action.measurement_ready);

    castle::timing::ptp::message_view decoded{};
    CASTLE_SAMPLE_CHECK(castle::timing::ptp::decode_message(
        array_view<const uint8_t>(action.delay_request, action.delay_request_size),
        decoded));
    CASTLE_SAMPLE_CHECK(decoded.is(message_type::delay_request));
    CASTLE_SAMPLE_CHECK(decoded.header.sequence_id == 41U);
    CASTLE_SAMPLE_CHECK(decoded.header.source_port.port_number == local.port_number);

    CASTLE_SAMPLE_CHECK(engine.delay_request_transmitted(timestamp(100U, 200U)) == castle::status::ok);

    write_header(packet, message_type::follow_up, 44U, 0U, master, 7U);
    write_timestamp(&packet[34U], 100U, 10U);
    timestamp unused_timestamp{};
    CASTLE_SAMPLE_CHECK(!castle::timing::ptp::read_message_timestamp(decoded, unused_timestamp));
    CASTLE_SAMPLE_CHECK(engine.receive(
        array_view<const uint8_t>(packet, 44U),
        timestamp(100U, 120U),
        action) == castle::status::ok);

    write_header(packet, message_type::delay_response, 54U, 0U, master, 41U);
    write_timestamp(&packet[34U], 100U, 260U);
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        packet[44U + index] = local.clock_identity[index];
    }
    write_be16(&packet[52U], local.port_number);

    castle::timing::ptp::message_view response_view{};
    CASTLE_SAMPLE_CHECK(castle::timing::ptp::decode_message(
        array_view<const uint8_t>(packet, 54U), response_view));
    castle::timing::ptp::timestamp master_rx{};
    CASTLE_SAMPLE_CHECK(castle::timing::ptp::read_message_timestamp(response_view, master_rx));
    CASTLE_SAMPLE_CHECK(master_rx.nanoseconds == 260U);

    CASTLE_SAMPLE_CHECK(engine.receive(
        array_view<const uint8_t>(packet, 54U),
        timestamp(100U, 300U),
        action) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(action.measurement_ready);
    CASTLE_SAMPLE_CHECK(action.sample.offset_nanoseconds == 19LL);
    CASTLE_SAMPLE_CHECK(action.sample.path_delay_nanoseconds == 79LL);
    CASTLE_SAMPLE_CHECK(action.sample.correction_nanoseconds == 2LL);
    CASTLE_SAMPLE_CHECK(!engine.pending());
    CASTLE_SAMPLE_CHECK(engine.last_measurement().offset_nanoseconds == 19LL);

    engine.reset();
    CASTLE_SAMPLE_CHECK(!engine.pending());

    return 0;
}
