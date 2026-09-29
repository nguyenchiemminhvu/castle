#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/mon_io.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using castle::protocols::ubx::messages::mon_io;

    // Construct a UBX-MON-IO payload containing statistics for one port.
    uint8_t payload[20U] = {};

    castle::write_le32(&payload[0U], 1000U);   // rx_bytes
    castle::write_le32(&payload[4U], 2000U);   // tx_bytes
    castle::write_le16(&payload[8U], 1U);      // parity_errs
    castle::write_le16(&payload[10U], 2U);     // framing_errs
    castle::write_le16(&payload[12U], 3U);     // overrun_errs
    castle::write_le16(&payload[14U], 4U);     // break_cond
    payload[16U] = 50U;                // rx_busy
    payload[17U] = 75U;                // tx_busy
    payload[18U] = 0U;                 // reserved
    payload[19U] = 0U;                 // reserved

    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status encode_status = encode_frame(
        UBX_CLASS_MON,
        UBX_ID_MON_IO,
        castle::container::array_view<const uint8_t>(payload, sizeof(payload)),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    // Decode the raw UBX frame.
    message_view raw;
    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);

    // Verify message type.
    CASTLE_SAMPLE_CHECK(mon_io<>::matches(raw));

    // Decode into strongly typed MON-IO structure.
    mon_io<> message;
    CASTLE_SAMPLE_CHECK(mon_io<>::decode(raw, message));

    CASTLE_SAMPLE_CHECK(message.num_ports == 1U);

    CASTLE_SAMPLE_CHECK(message.ports[0U].rx_bytes == 1000U);
    CASTLE_SAMPLE_CHECK(message.ports[0U].tx_bytes == 2000U);
    CASTLE_SAMPLE_CHECK(message.ports[0U].parity_errs == 1U);
    CASTLE_SAMPLE_CHECK(message.ports[0U].framing_errs == 2U);
    CASTLE_SAMPLE_CHECK(message.ports[0U].overrun_errs == 3U);
    CASTLE_SAMPLE_CHECK(message.ports[0U].break_cond == 4U);
    CASTLE_SAMPLE_CHECK(message.ports[0U].rx_busy == 50U);
    CASTLE_SAMPLE_CHECK(message.ports[0U].tx_busy == 75U);

    // Multiple ports can be decoded from a single payload.
    uint8_t two_port_payload[40U] = {};

    // Port 0
    castle::write_le32(&two_port_payload[0U], 100U);
    castle::write_le32(&two_port_payload[4U], 200U);

    // Port 1
    castle::write_le32(&two_port_payload[20U], 300U);
    castle::write_le32(&two_port_payload[24U], 400U);

    castle::size_type two_port_written = 0U;

    encode_status = encode_frame(
        UBX_CLASS_MON,
        UBX_ID_MON_IO,
        castle::container::array_view<const uint8_t>(
            two_port_payload,
            sizeof(two_port_payload)),
        frame,
        sizeof(frame),
        two_port_written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    decoded = decode_frame(
        castle::container::array_view<const uint8_t>(
            frame,
            two_port_written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);

    mon_io<> two_ports;
    CASTLE_SAMPLE_CHECK(mon_io<>::decode(raw, two_ports));

    CASTLE_SAMPLE_CHECK(two_ports.num_ports == 2U);
    CASTLE_SAMPLE_CHECK(two_ports.ports[0U].rx_bytes == 100U);
    CASTLE_SAMPLE_CHECK(two_ports.ports[1U].rx_bytes == 300U);
    CASTLE_SAMPLE_CHECK(two_ports.ports[1U].tx_bytes == 400U);

    // Non-MON-IO messages must not match.
    uint8_t cfg_payload[1U] = {0U};

    castle::size_type cfg_written = 0U;
    encode_status = encode_frame(
        UBX_CLASS_CFG,
        UBX_ID_CFG_VALGET,
        castle::container::array_view<const uint8_t>(cfg_payload, 1U),
        frame,
        sizeof(frame),
        cfg_written);
    
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    message_view non_mon_io;
    CASTLE_SAMPLE_CHECK(
        decode_frame(
            castle::container::array_view<const uint8_t>(frame, cfg_written),
            non_mon_io));

    CASTLE_SAMPLE_CHECK(!mon_io<>::matches(non_mon_io));

    // Invalid payload length (not a multiple of 20 bytes) is rejected.
    uint8_t invalid_payload[21U] = {};

    castle::size_type invalid_written = 0U;
    encode_status = encode_frame(
        UBX_CLASS_MON,
        UBX_ID_MON_IO,
        castle::container::array_view<const uint8_t>(
            invalid_payload,
            sizeof(invalid_payload)),
        frame,
        sizeof(frame),
        invalid_written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    message_view invalid_raw;
    CASTLE_SAMPLE_CHECK(
        decode_frame(
            castle::container::array_view<const uint8_t>(
                frame,
                invalid_written),
            invalid_raw));

    mon_io<> invalid_message;
    CASTLE_SAMPLE_CHECK(!mon_io<>::decode(invalid_raw, invalid_message));

    return 0;
}