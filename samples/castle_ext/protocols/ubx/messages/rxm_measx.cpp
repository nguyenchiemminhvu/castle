#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/rxm_measx.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    // Build a minimal RXM-MEASX payload containing one satellite measurement.
    uint8_t payload[rxm_measx<>::header_length + rxm_measx<>::block_length] = {};

    castle::size_type offset = 0U;

    payload[offset++] = 0x01U; // version

    offset += 3U; // reserved

    write_le32(&payload[offset], 1000U); offset += 4U; // gps_tow
    write_le32(&payload[offset], 2000U); offset += 4U; // glo_tow
    write_le32(&payload[offset], 3000U); offset += 4U; // bds_tow

    offset += 4U; // reserved

    write_le32(&payload[offset], 4000U); offset += 4U; // qzss_tow

    write_le16(&payload[offset], 10U); offset += 2U; // gps_tow_acc
    write_le16(&payload[offset], 20U); offset += 2U; // glo_tow_acc
    write_le16(&payload[offset], 30U); offset += 2U; // bds_tow_acc

    offset += 2U; // reserved

    write_le16(&payload[offset], 40U); offset += 2U; // qzss_tow_acc

    payload[offset++] = 1U;      // num_svs
    payload[offset++] = 0xAAU;   // flags

    offset += 8U; // reserved

    // Satellite measurement block.
    payload[offset++] = 0U;      // gnss_id (GPS)
    payload[offset++] = 22U;     // sv_id
    payload[offset++] = 45U;     // cno
    payload[offset++] = 1U;      // mpath_indic

    write_le32(&payload[offset], static_cast<uint32_t>(-100)); offset += 4U;
    write_le32(&payload[offset], static_cast<uint32_t>(-200)); offset += 4U;

    write_le16(&payload[offset], 123U); offset += 2U; // whole_chips
    write_le16(&payload[offset], 456U); offset += 2U; // frac_chips

    write_le32(&payload[offset], 789U); offset += 4U; // code_phase

    payload[offset++] = 5U;  // int_code_phase
    payload[offset++] = 6U;  // pseu_range_rms_err

    offset += 2U; // reserved

    CASTLE_SAMPLE_CHECK(offset == sizeof(payload));

    // Encode payload into a UBX frame.
    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        UBX_CLASS_RXM,
        UBX_ID_RXM_MEASX,
        castle::container::array_view<const uint8_t>(payload, sizeof(payload)),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // Decode frame into a zero-copy UBX message view.
    message_view raw;
    bool ok = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(ok);

    // Verify message type.
    CASTLE_SAMPLE_CHECK(rxm_measx<>::matches(raw));

    // Decode RXM-MEASX payload.
    rxm_measx<> measx;
    CASTLE_SAMPLE_CHECK(rxm_measx<>::decode(raw, measx));

    // Header fields.
    CASTLE_SAMPLE_CHECK(measx.version == 1U);
    CASTLE_SAMPLE_CHECK(measx.gps_tow == 1000U);
    CASTLE_SAMPLE_CHECK(measx.glo_tow == 2000U);
    CASTLE_SAMPLE_CHECK(measx.bds_tow == 3000U);
    CASTLE_SAMPLE_CHECK(measx.qzss_tow == 4000U);

    CASTLE_SAMPLE_CHECK(measx.gps_tow_acc == 10U);
    CASTLE_SAMPLE_CHECK(measx.glo_tow_acc == 20U);
    CASTLE_SAMPLE_CHECK(measx.bds_tow_acc == 30U);
    CASTLE_SAMPLE_CHECK(measx.qzss_tow_acc == 40U);

    CASTLE_SAMPLE_CHECK(measx.num_svs == 1U);
    CASTLE_SAMPLE_CHECK(measx.flags == 0xAAU);

    // Satellite block.
    CASTLE_SAMPLE_CHECK(measx.svs[0U].gnss_id == 0U);
    CASTLE_SAMPLE_CHECK(measx.svs[0U].sv_id == 22U);
    CASTLE_SAMPLE_CHECK(measx.svs[0U].cno == 45U);
    CASTLE_SAMPLE_CHECK(measx.svs[0U].mpath_indic == 1U);

    CASTLE_SAMPLE_CHECK(measx.svs[0U].doppler_ms == -100);
    CASTLE_SAMPLE_CHECK(measx.svs[0U].doppler_hz == -200);

    CASTLE_SAMPLE_CHECK(measx.svs[0U].whole_chips == 123U);
    CASTLE_SAMPLE_CHECK(measx.svs[0U].frac_chips == 456U);
    CASTLE_SAMPLE_CHECK(measx.svs[0U].code_phase == 789U);

    CASTLE_SAMPLE_CHECK(measx.svs[0U].int_code_phase == 5U);
    CASTLE_SAMPLE_CHECK(measx.svs[0U].pseu_range_rms_err == 6U);

    return 0;
}
