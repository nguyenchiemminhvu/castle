#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_txbuf.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using castle::protocols::ubx::messages::mon_txbuf;

    // Build a MON-TXBUF report: target 2 hit its limit and the TX buffer is full.
    mon_txbuf status{};
    for (castle::size_type target = 0U; target < mon_txbuf::num_targets; ++target)
    {
        status.pending[target] = static_cast<uint16_t>(100U + target);
        status.usage[target] = static_cast<uint8_t>(10U + target);
        status.peak_usage[target] = static_cast<uint8_t>(20U + target);
    }
    status.t_used = 40U;
    status.t_peak = 80U;
    status.errors = static_cast<uint8_t>((1U << 2U) | mon_txbuf::ERRORS_ALLOC);

    uint8_t frame[mon_txbuf::payload_length + UBX_FRAME_OVERHEAD];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_txbuf::matches(raw));

    mon_txbuf decoded{};
    CASTLE_SAMPLE_CHECK(mon_txbuf::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.pending[5U] == 105U);
    CASTLE_SAMPLE_CHECK(decoded.usage[0U] == 10U);
    CASTLE_SAMPLE_CHECK(decoded.peak_usage[3U] == 23U);
    CASTLE_SAMPLE_CHECK(decoded.t_used == 40U);
    CASTLE_SAMPLE_CHECK(decoded.t_peak == 80U);

    // Error bitmask helpers.
    CASTLE_SAMPLE_CHECK(decoded.limit() == 0x04U);
    CASTLE_SAMPLE_CHECK(decoded.limit_reached(2U));
    CASTLE_SAMPLE_CHECK(!decoded.limit_reached(0U));
    CASTLE_SAMPLE_CHECK(!decoded.mem_error());
    CASTLE_SAMPLE_CHECK(decoded.alloc_error());

    // Too small an output buffer is reported, not overrun.
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame) - 1U, written) == castle::status::full);

    // Request the message with an empty poll frame.
    CASTLE_SAMPLE_CHECK(encode_poll<mon_txbuf>(frame, sizeof(frame), written) == castle::status::ok);
    return 0;
}
