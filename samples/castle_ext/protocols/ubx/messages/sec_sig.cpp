#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/sec_sig.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    // Build a UBX-SEC-SIG payload.
    uint8_t payload[sec_sig::payload_length] = {};

    payload[0U] = 1U; // version

    // bytes 1..3 reserved

    // jam_flags:
    // bit0 = jam_det_enabled
    // bits1..2 = jamming_state
    payload[4U] = static_cast<uint8_t>((2U << 1U) | 1U);

    // bytes 5..7 reserved

    // spf_flags:
    // bit0 = spf_det_enabled
    // bits1..3 = spoofing_state
    payload[8U] = static_cast<uint8_t>((5U << 1U) | 1U);

    // bytes 9..11 reserved

    // Encode payload into a UBX frame.
    uint8_t frame[64U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        sec_sig::msg_class,
        sec_sig::msg_id,
        castle::container::array_view<const uint8_t>(payload, sizeof(payload)),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // Decode frame into a generic UBX message view.
    message_view raw;
    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);

    // Verify message identity.
    CASTLE_SAMPLE_CHECK(sec_sig::matches(raw));

    // Decode SEC-SIG payload.
    sec_sig sig;
    CASTLE_SAMPLE_CHECK(sec_sig::decode(raw, sig));

    // Verify decoded fields.
    CASTLE_SAMPLE_CHECK(sig.version == 1U);

    CASTLE_SAMPLE_CHECK(sig.jam_flags == ((2U << 1U) | 1U));
    CASTLE_SAMPLE_CHECK(sig.jam_det_enabled == 1U);
    CASTLE_SAMPLE_CHECK(sig.jamming_state == 2U);

    CASTLE_SAMPLE_CHECK(sig.spf_flags == ((5U << 1U) | 1U));
    CASTLE_SAMPLE_CHECK(sig.spf_det_enabled == 1U);
    CASTLE_SAMPLE_CHECK(sig.spoofing_state == 5U);

    // Different message type must not match SEC-SIG.
    CASTLE_SAMPLE_CHECK(!raw.is(UBX_CLASS_CFG, UBX_ID_CFG_VALSET));

    return 0;
}
