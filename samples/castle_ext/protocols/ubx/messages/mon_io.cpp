#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_io.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using castle::protocols::ubx::messages::mon_io;
    using castle::protocols::ubx::messages::mon_io_port;

    // Build a two-port MON-IO message and encode it into a complete frame.
    mon_io<4U> statistics{};
    mon_io_port uart1{};
    uart1.rx_bytes = 1000U;
    uart1.tx_bytes = 2000U;
    uart1.parity_errs = 1U;
    uart1.framing_errs = 2U;
    uart1.overrun_errs = 3U;
    uart1.break_cond = 4U;
    CASTLE_SAMPLE_CHECK(statistics.ports.push_back(uart1) == castle::status::ok);
    mon_io_port usb{};
    usb.rx_bytes = 300U;
    usb.tx_bytes = 400U;
    CASTLE_SAMPLE_CHECK(statistics.ports.push_back(usb) == castle::status::ok);

    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(statistics, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(2U * mon_io<4U>::block_length));

    // Validate the frame and decode it back.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_io<4U>::matches(raw));

    mon_io<4U> decoded{};
    CASTLE_SAMPLE_CHECK(mon_io<4U>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.ports.size() == 2U);
    CASTLE_SAMPLE_CHECK(decoded.ports[0U].rx_bytes == 1000U);
    CASTLE_SAMPLE_CHECK(decoded.ports[0U].tx_bytes == 2000U);
    CASTLE_SAMPLE_CHECK(decoded.ports[0U].parity_errs == 1U);
    CASTLE_SAMPLE_CHECK(decoded.ports[0U].framing_errs == 2U);
    CASTLE_SAMPLE_CHECK(decoded.ports[0U].overrun_errs == 3U);
    CASTLE_SAMPLE_CHECK(decoded.ports[0U].break_cond == 4U);
    CASTLE_SAMPLE_CHECK(decoded.ports[1U].rx_bytes == 300U);
    CASTLE_SAMPLE_CHECK(decoded.ports[1U].tx_bytes == 400U);

    // A receiver with more ports than the storage capacity: extra ports are skipped.
    mon_io<1U> first_port_only{};
    CASTLE_SAMPLE_CHECK(mon_io<1U>::decode(raw, first_port_only));
    CASTLE_SAMPLE_CHECK(first_port_only.ports.size() == 1U);
    CASTLE_SAMPLE_CHECK(first_port_only.ports[0U].rx_bytes == 1000U);

    // Zero ports is a valid (empty) MON-IO payload.
    mon_io<4U> no_ports{};
    CASTLE_SAMPLE_CHECK(encode_message(no_ports, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_io<4U>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.ports.empty());

    // A length that is not a whole number of 20-byte blocks is rejected.
    uint8_t invalid_payload[21U] = {};
    CASTLE_SAMPLE_CHECK(encode_frame(UBX_CLASS_MON, UBX_ID_MON_IO,
                                     castle::container::array_view<const uint8_t>(invalid_payload, sizeof(invalid_payload)),
                                     frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(!mon_io<4U>::decode(raw, decoded));

    // Request the message with an empty poll frame.
    CASTLE_SAMPLE_CHECK(encode_poll<mon_io<4U>>(frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == UBX_FRAME_OVERHEAD);
    return 0;
}
