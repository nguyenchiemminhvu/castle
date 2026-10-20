#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_msgpp.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using castle::protocols::ubx::messages::mon_msgpp;

    // msg[port][protocol]: spec field msg<port + 1>, element <protocol>.
    mon_msgpp status{};
    for (castle::size_type port = 0U; port < mon_msgpp::num_ports; ++port)
    {
        for (castle::size_type protocol = 0U; protocol < mon_msgpp::num_protocols; ++protocol)
        {
            status.msg[port][protocol] = static_cast<uint16_t>((port * 100U) + protocol);
        }
        status.skipped[port] = static_cast<uint32_t>(1000U + port);
    }

    uint8_t frame[mon_msgpp::payload_length + UBX_FRAME_OVERHEAD];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_msgpp::matches(raw));

    mon_msgpp decoded{};
    CASTLE_SAMPLE_CHECK(mon_msgpp::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.msg[0U][0U] == 0U);
    CASTLE_SAMPLE_CHECK(decoded.msg[2U][7U] == 207U); // msg3, protocol 7
    CASTLE_SAMPLE_CHECK(decoded.msg[5U][1U] == 501U); // msg6, protocol 1
    CASTLE_SAMPLE_CHECK(decoded.skipped[4U] == 1004U);

    // Too small an output buffer is reported, not overrun.
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame) - 1U, written) == castle::status::full);

    // Request the message with an empty poll frame.
    CASTLE_SAMPLE_CHECK(encode_poll<mon_msgpp>(frame, sizeof(frame), written) == castle::status::ok);
    return 0;
}
