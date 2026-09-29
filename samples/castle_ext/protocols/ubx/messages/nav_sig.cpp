#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_sig.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    // Build a NAV-SIG payload containing two signal entries.
    uint8_t payload[nav_sig<>::header_length + (2U * nav_sig<>::block_length)] = {};

    write_le32(&payload[0U], 654321U); // iTOW
    payload[4U] = 0U;                  // version
    payload[5U] = 2U;                  // numSigs

    // Signal #1
    payload[8U] = 0U;                  // gnssId (GPS)
    payload[9U] = 5U;                  // svId
    payload[10U] = 0U;                 // sigId
    payload[11U] = 0U;                 // freqId
    write_le16(&payload[12U], 12U);    // prRes
    payload[14U] = 45U;                // cno
    payload[15U] = 4U;                 // qualInd
    payload[16U] = 1U;                 // corrSource
    payload[17U] = 2U;                 // ionoModel
    write_le16(&payload[18U], 0x0003U); // sigFlags

    // Signal #2
    payload[24U] = 2U;                 // gnssId (Galileo)
    payload[25U] = 12U;                // svId
    payload[26U] = 1U;                 // sigId
    payload[27U] = 0U;                 // freqId
    write_le16(&payload[28U], static_cast<uint16_t>(-8));
    payload[30U] = 50U;                // cno
    payload[31U] = 5U;                 // qualInd
    payload[32U] = 2U;                 // corrSource
    payload[33U] = 1U;                 // ionoModel
    write_le16(&payload[34U], 0x0010U); // sigFlags

    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        nav_sig<>::msg_class,
        nav_sig<>::msg_id,
        castle::container::array_view<const uint8_t>(
            payload,
            sizeof(payload)),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // Decode the UBX frame.
    message_view raw;
    bool frame_ok = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(frame_ok);
    CASTLE_SAMPLE_CHECK(nav_sig<>::matches(raw));

    // Decode the NAV-SIG message.
    nav_sig<> sig;
    CASTLE_SAMPLE_CHECK(nav_sig<>::decode(raw, sig));

    CASTLE_SAMPLE_CHECK(sig.i_tow == 654321U);
    CASTLE_SAMPLE_CHECK(sig.version == 0U);
    CASTLE_SAMPLE_CHECK(sig.num_sigs == 2U);

    CASTLE_SAMPLE_CHECK(sig.signals[0U].gnss_id == 0U);
    CASTLE_SAMPLE_CHECK(sig.signals[0U].sv_id == 5U);
    CASTLE_SAMPLE_CHECK(sig.signals[0U].sig_id == 0U);
    CASTLE_SAMPLE_CHECK(sig.signals[0U].freq_id == 0U);
    CASTLE_SAMPLE_CHECK(sig.signals[0U].pr_res == 12);
    CASTLE_SAMPLE_CHECK(sig.signals[0U].cno == 45U);
    CASTLE_SAMPLE_CHECK(sig.signals[0U].qual_ind == 4U);
    CASTLE_SAMPLE_CHECK(sig.signals[0U].corr_source == 1U);
    CASTLE_SAMPLE_CHECK(sig.signals[0U].iono_model == 2U);
    CASTLE_SAMPLE_CHECK(sig.signals[0U].sig_flags == 0x0003U);

    CASTLE_SAMPLE_CHECK(sig.signals[1U].gnss_id == 2U);
    CASTLE_SAMPLE_CHECK(sig.signals[1U].sv_id == 12U);
    CASTLE_SAMPLE_CHECK(sig.signals[1U].sig_id == 1U);
    CASTLE_SAMPLE_CHECK(sig.signals[1U].freq_id == 0U);
    CASTLE_SAMPLE_CHECK(sig.signals[1U].pr_res == -8);
    CASTLE_SAMPLE_CHECK(sig.signals[1U].cno == 50U);
    CASTLE_SAMPLE_CHECK(sig.signals[1U].qual_ind == 5U);
    CASTLE_SAMPLE_CHECK(sig.signals[1U].corr_source == 2U);
    CASTLE_SAMPLE_CHECK(sig.signals[1U].iono_model == 1U);
    CASTLE_SAMPLE_CHECK(sig.signals[1U].sig_flags == 0x0010U);

    // A different NAV message ID does not match NAV-SIG.
    raw.header.msg_id = UBX_ID_NAV_PVT;
    CASTLE_SAMPLE_CHECK(!nav_sig<>::matches(raw));

    return 0;
}
