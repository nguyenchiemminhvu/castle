#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_span.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using castle::protocols::ubx::messages::mon_span;
    using castle::protocols::ubx::messages::mon_span_rf_block;
    using castle::protocols::ubx::messages::MON_SPAN_SPECTRUM_BINS;

    // One RF path (L1) spectrum: 128 MHz span centred on 1575.42 MHz.
    mon_span<2U> span{};
    mon_span_rf_block l1{};
    for (castle::size_type bin = 0U; bin < MON_SPAN_SPECTRUM_BINS; ++bin)
    {
        l1.spectrum[bin] = static_cast<uint8_t>(bin);
    }
    l1.span = 128000000U;
    l1.res = 500000U;
    l1.center = 1575420000U;
    l1.pga = 54U;
    CASTLE_SAMPLE_CHECK(span.rf_blocks.push_back(l1) == castle::status::ok);

    // Frequency of bin i: center + span * (i - 128) / 256.
    CASTLE_SAMPLE_CHECK(l1.bin_frequency_hz(128U) == 1575420000.0);
    CASTLE_SAMPLE_CHECK(l1.bin_frequency_hz(0U) == 1511420000.0);

    uint8_t frame[mon_span<2U>::header_length + mon_span<2U>::block_length + UBX_FRAME_OVERHEAD];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(span, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_span<2U>::matches(raw));

    mon_span<2U> decoded{};
    CASTLE_SAMPLE_CHECK(mon_span<2U>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == 0U);
    CASTLE_SAMPLE_CHECK(decoded.num_rf_blocks == 1U); // taken from rf_blocks.size() on encode
    CASTLE_SAMPLE_CHECK(decoded.rf_blocks.size() == 1U);
    CASTLE_SAMPLE_CHECK(decoded.rf_blocks[0U].spectrum[255U] == 255U);
    CASTLE_SAMPLE_CHECK(decoded.rf_blocks[0U].span == 128000000U);
    CASTLE_SAMPLE_CHECK(decoded.rf_blocks[0U].res == 500000U);
    CASTLE_SAMPLE_CHECK(decoded.rf_blocks[0U].center == 1575420000U);
    CASTLE_SAMPLE_CHECK(decoded.rf_blocks[0U].pga == 54U);

    // Too small an output buffer is reported, not overrun.
    CASTLE_SAMPLE_CHECK(encode_message(span, frame, sizeof(frame) - 1U, written) == castle::status::full);

    // Request the message with an empty poll frame.
    CASTLE_SAMPLE_CHECK(encode_poll<mon_span<2U>>(frame, sizeof(frame), written) == castle::status::ok);
    return 0;
}
