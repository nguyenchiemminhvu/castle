#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/rxm_pmreq.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // 16-byte request: back up for 60 s even with USB attached, wake on UART RX or EXTINT0.
    rxm_pmreq_v1 request{};
    request.duration = 60000U;
    request.flags = rxm_pmreq_v1::FLAGS_BACKUP | rxm_pmreq_v1::FLAGS_FORCE;
    request.wakeup_sources = rxm_pmreq_v1::WAKEUP_UARTRX | rxm_pmreq_v1::WAKEUP_EXTINT0;

    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_pmreq_v1::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(request, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    // Loop back: only the 16-byte model matches a 16-byte frame.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(rxm_pmreq_v1::matches(raw));
    CASTLE_SAMPLE_CHECK(!rxm_pmreq_v0::matches(raw));

    rxm_pmreq_v1 decoded{};
    CASTLE_SAMPLE_CHECK(rxm_pmreq_v1::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.duration == 60000U);
    CASTLE_SAMPLE_CHECK(decoded.backup());
    CASTLE_SAMPLE_CHECK(decoded.force());
    CASTLE_SAMPLE_CHECK(decoded.wakeup_uartrx());
    CASTLE_SAMPLE_CHECK(decoded.wakeup_extint0());
    CASTLE_SAMPLE_CHECK(!decoded.wakeup_extint1());
    CASTLE_SAMPLE_CHECK(!decoded.wakeup_spics());

    // 8-byte request: infinite backup.
    rxm_pmreq_v0 legacy{};
    legacy.duration = RXM_PMREQ_DURATION_INFINITE;
    legacy.flags = rxm_pmreq_v0::FLAGS_BACKUP;

    uint8_t legacy_frame[UBX_FRAME_OVERHEAD + rxm_pmreq_v0::payload_length];
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(legacy, legacy_frame, sizeof(legacy_frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(legacy_frame, written), raw));
    CASTLE_SAMPLE_CHECK(rxm_pmreq_v0::matches(raw));
    CASTLE_SAMPLE_CHECK(!rxm_pmreq_v1::matches(raw));

    rxm_pmreq_v0 legacy_decoded{};
    CASTLE_SAMPLE_CHECK(rxm_pmreq_v0::decode(raw, legacy_decoded));
    CASTLE_SAMPLE_CHECK(legacy_decoded.duration == RXM_PMREQ_DURATION_INFINITE);
    CASTLE_SAMPLE_CHECK(legacy_decoded.backup());

    return 0;
}
