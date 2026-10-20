#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/rxm_rlm.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Short-RLM from Galileo E12 acknowledging beacon 0x0123456789ABCDE.
    rxm_rlm_short short_rlm{};
    short_rlm.sv_id = 12U;
    short_rlm.beacon = {{0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xABU, 0xCDU, 0xEFU}};
    short_rlm.message = 0x01U;
    short_rlm.params = {{0xA5U, 0x5AU}};

    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_rlm_long::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(short_rlm, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == UBX_FRAME_OVERHEAD + rxm_rlm_short::payload_length);

    // Each frame matches exactly one of the two layouts.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(rxm_rlm_short::matches(raw));
    CASTLE_SAMPLE_CHECK(!rxm_rlm_long::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload[1U] == RXM_RLM_TYPE_SHORT);

    rxm_rlm_short decoded_short{};
    CASTLE_SAMPLE_CHECK(rxm_rlm_short::decode(raw, decoded_short));
    CASTLE_SAMPLE_CHECK(decoded_short.sv_id == 12U);
    CASTLE_SAMPLE_CHECK(decoded_short.message == 0x01U);
    CASTLE_SAMPLE_CHECK(decoded_short.params[0U] == 0xA5U);
    // The top four bits of the first beacon byte are not part of the 60-bit ID.
    CASTLE_SAMPLE_CHECK(decoded_short.beacon_id() == 0x0123456789ABCDEFULL);

    // Long-RLM with 96 bits of parameters.
    rxm_rlm_long long_rlm{};
    long_rlm.sv_id = 30U;
    long_rlm.beacon = {{0xF0U, 0U, 0U, 0U, 0U, 0U, 0U, 0x01U}};
    long_rlm.message = 0x0FU;
    for (castle::size_type index = 0U; index < long_rlm.params.size(); ++index)
    {
        long_rlm.params[index] = static_cast<uint8_t>(index + 1U);
    }

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(long_rlm, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(rxm_rlm_long::matches(raw));
    CASTLE_SAMPLE_CHECK(!rxm_rlm_short::matches(raw));

    rxm_rlm_long decoded_long{};
    CASTLE_SAMPLE_CHECK(rxm_rlm_long::decode(raw, decoded_long));
    CASTLE_SAMPLE_CHECK(decoded_long.sv_id == 30U);
    CASTLE_SAMPLE_CHECK(decoded_long.params[11U] == 12U);
    CASTLE_SAMPLE_CHECK(decoded_long.beacon_id() == 0x0000000000000001ULL);

    return 0;
}
