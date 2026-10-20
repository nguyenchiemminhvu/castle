#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_comms.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Protocol slots shared by every port block.
    mon_comms<> report{};
    report.prot_ids[0U] = mon_comms_protocol::ubx;
    report.prot_ids[1U] = mon_comms_protocol::nmea;
    report.prot_ids[2U] = mon_comms_protocol::rtcm3;

    mon_comms_port uart1{};
    uart1.port_id = 0x0101U;
    uart1.tx_bytes = 123456U;
    uart1.rx_bytes = 4096U;
    uart1.tx_usage = 12U;
    uart1.tx_peak_usage = 40U;
    uart1.msgs[0U] = 250U; // UBX
    uart1.msgs[1U] = 10U;  // NMEA
    uart1.msgs[2U] = 75U;  // RTCM3
    CASTLE_SAMPLE_CHECK(report.ports.push_back(uart1) == castle::status::ok);

    uint8_t frame[96U] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(report, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(mon_comms<>::header_length + mon_comms<>::block_length));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_comms<>::matches(raw));

    mon_comms<> decoded{};
    CASTLE_SAMPLE_CHECK(mon_comms<>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.n_ports == 1U);
    CASTLE_SAMPLE_CHECK(decoded.ports.size() == 1U);
    CASTLE_SAMPLE_CHECK(!decoded.tx_error_mem());
    CASTLE_SAMPLE_CHECK(!decoded.tx_error_alloc());
    CASTLE_SAMPLE_CHECK(decoded.prot_ids[3U] == mon_comms_protocol::none);

    const mon_comms_port& port = decoded.ports[0U];
    CASTLE_SAMPLE_CHECK(port.port_id == 0x0101U);
    CASTLE_SAMPLE_CHECK(port.tx_bytes == 123456U);
    CASTLE_SAMPLE_CHECK(port.tx_peak_usage == 40U);

    // Per-protocol counters are looked up through prot_ids.
    uint16_t rtcm3 = 0U;
    CASTLE_SAMPLE_CHECK(decoded.parsed_messages(port, mon_comms_protocol::rtcm3, rtcm3));
    CASTLE_SAMPLE_CHECK(rtcm3 == 75U);
    uint16_t spartn = 0U;
    CASTLE_SAMPLE_CHECK(!decoded.parsed_messages(port, mon_comms_protocol::spartn, spartn));

    // TX error flags.
    decoded.tx_errors = mon_comms<>::TX_ERRORS_ALLOC;
    CASTLE_SAMPLE_CHECK(decoded.tx_error_alloc());

    CASTLE_SAMPLE_CHECK(encode_message(report, frame, frame_size(8U), written) == castle::status::full);
    return 0;
}
