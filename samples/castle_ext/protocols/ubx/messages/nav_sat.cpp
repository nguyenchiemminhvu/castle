#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_sat.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a NAV-SAT payload containing two satellite entries.
    uint8_t payload[nav_sat<>::header_length + (2U * nav_sat<>::block_length)] = {};

    castle::write_le32(&payload[0U], 123456U); // iTOW
    payload[4U] = 1U;                  // version
    payload[5U] = 2U;                  // numSvs

    // Satellite #1
    payload[8U] = 0U;                  // gnssId (GPS)
    payload[9U] = 10U;                 // svId
    payload[10U] = 45U;                // cno
    payload[11U] = 30U;                // elev
    castle::write_le16(&payload[12U], 120U);   // azim
    castle::write_le16(&payload[14U], 25U);    // prRes
    castle::write_le32(&payload[16U], 0x00000001U);

    // Satellite #2
    payload[20U] = 2U;                 // gnssId (Galileo)
    payload[21U] = 20U;                // svId
    payload[22U] = 50U;                // cno
    payload[23U] = static_cast<uint8_t>(-15);
    castle::write_le16(&payload[24U], 270U);   // azim
    castle::write_le16(&payload[26U], static_cast<uint16_t>(-10));
    castle::write_le32(&payload[28U], 0x00000005U);

    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        nav_sat<>::msg_class,
        nav_sat<>::msg_id,
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
    CASTLE_SAMPLE_CHECK(nav_sat<>::matches(raw));

    // Decode the NAV-SAT message.
    nav_sat<> sat;
    CASTLE_SAMPLE_CHECK(nav_sat<>::decode(raw, sat));

    CASTLE_SAMPLE_CHECK(sat.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(sat.version == 1U);
    CASTLE_SAMPLE_CHECK(sat.num_svs == 2U);

    CASTLE_SAMPLE_CHECK(sat.svs[0U].gnss_id == 0U);
    CASTLE_SAMPLE_CHECK(sat.svs[0U].sv_id == 10U);
    CASTLE_SAMPLE_CHECK(sat.svs[0U].cno == 45U);
    CASTLE_SAMPLE_CHECK(sat.svs[0U].elev == 30);
    CASTLE_SAMPLE_CHECK(sat.svs[0U].azim == 120);
    CASTLE_SAMPLE_CHECK(sat.svs[0U].pr_res == 25);
    CASTLE_SAMPLE_CHECK(sat.svs[0U].flags == 0x00000001U);

    CASTLE_SAMPLE_CHECK(sat.svs[1U].gnss_id == 2U);
    CASTLE_SAMPLE_CHECK(sat.svs[1U].sv_id == 20U);
    CASTLE_SAMPLE_CHECK(sat.svs[1U].cno == 50U);
    CASTLE_SAMPLE_CHECK(sat.svs[1U].elev == -15);
    CASTLE_SAMPLE_CHECK(sat.svs[1U].azim == 270);
    CASTLE_SAMPLE_CHECK(sat.svs[1U].pr_res == -10);
    CASTLE_SAMPLE_CHECK(sat.svs[1U].flags == 0x00000005U);

    // A different NAV message ID does not match NAV-SAT.
    raw.header.msg_id = UBX_ID_NAV_STATUS;
    CASTLE_SAMPLE_CHECK(!nav_sat<>::matches(raw));

    return 0;
}
