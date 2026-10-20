#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/sec_sig.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Jamming detection on with a warning; spoofing detection on, spoofing indicated.
    sec_sig sig{};
    sig.version = 0x01U;
    sig.jam_flags = static_cast<uint8_t>(sec_sig::JAM_FLAGS_JAM_DET_ENABLED
                                         | (static_cast<uint8_t>(sec_sig_jamming_state::warning) << 1U));
    sig.spf_flags = static_cast<uint8_t>(sec_sig::SPF_FLAGS_SPF_DET_ENABLED
                                         | (static_cast<uint8_t>(sec_sig_spoofing_state::indicated) << 1U));

    uint8_t frame[UBX_FRAME_OVERHEAD + sec_sig::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(sig, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(sec_sig::matches(raw));

    sec_sig decoded{};
    CASTLE_SAMPLE_CHECK(sec_sig::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == 0x01U);
    CASTLE_SAMPLE_CHECK(decoded.jam_det_enabled());
    CASTLE_SAMPLE_CHECK(decoded.jamming_state() == sec_sig_jamming_state::warning);
    CASTLE_SAMPLE_CHECK(decoded.spf_det_enabled());
    CASTLE_SAMPLE_CHECK(decoded.spoofing_state() == sec_sig_spoofing_state::indicated);

    // Newer versions append data after the 12-byte prefix; it is ignored.
    uint8_t extended[sec_sig::payload_length + 8U] = {};
    for (castle::size_type index = 0U; index < sec_sig::payload_length; ++index)
    {
        extended[index] = raw.payload[index];
    }
    message_view longer{{UBX_CLASS_SEC, UBX_ID_SEC_SIG, static_cast<uint16_t>(sizeof(extended))},
                        castle::container::array_view<const uint8_t>(extended, sizeof(extended)),
                        checksum{}};
    CASTLE_SAMPLE_CHECK(sec_sig::decode(longer, decoded));
    CASTLE_SAMPLE_CHECK(decoded.jamming_state() == sec_sig_jamming_state::warning);

    // SEC-CRC is not SEC-SIG.
    message_view other = raw;
    other.header.msg_id = UBX_ID_SEC_CRC;
    CASTLE_SAMPLE_CHECK(!sec_sig::matches(other));
    CASTLE_SAMPLE_CHECK(!sec_sig::decode(other, decoded));

    return 0;
}
