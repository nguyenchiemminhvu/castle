#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_rxbuf.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using castle::protocols::ubx::messages::mon_rxbuf;

    // Build a MON-RXBUF report with distinct values per target.
    mon_rxbuf status{};
    for (castle::size_type target = 0U; target < mon_rxbuf::num_targets; ++target)
    {
        status.pending[target] = static_cast<uint16_t>(500U + target);
        status.usage[target] = static_cast<uint8_t>(30U + target);
        status.peak_usage[target] = static_cast<uint8_t>(60U + target);
    }

    uint8_t frame[mon_rxbuf::payload_length + UBX_FRAME_OVERHEAD];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_rxbuf::matches(raw));

    mon_rxbuf decoded{};
    CASTLE_SAMPLE_CHECK(mon_rxbuf::decode(raw, decoded));
    for (castle::size_type target = 0U; target < mon_rxbuf::num_targets; ++target)
    {
        CASTLE_SAMPLE_CHECK(decoded.pending[target] == status.pending[target]);
        CASTLE_SAMPLE_CHECK(decoded.usage[target] == status.usage[target]);
        CASTLE_SAMPLE_CHECK(decoded.peak_usage[target] == status.peak_usage[target]);
    }

    // Too small an output buffer is reported, not overrun.
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame) - 1U, written) == castle::status::full);

    // Request the message with an empty poll frame.
    CASTLE_SAMPLE_CHECK(encode_poll<mon_rxbuf>(frame, sizeof(frame), written) == castle::status::ok);
    return 0;
}
