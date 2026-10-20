#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_sat.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a NAV-SAT value with two satellites.
    nav_sat<4U> sat{};
    sat.i_tow = 123456U;
    sat.version = 1U;

    nav_sat_sv gps{};
    gps.gnss_id = 0U; // GPS
    gps.sv_id = 10U;
    gps.cno = 45U;
    gps.elev = 30;
    gps.azim = 120;
    gps.pr_res = 25; // 2.5 m
    gps.flags = 5U /* code and carrier locked */ | nav_sat_sv::FLAGS_SV_USED | (1U << 4U) /* healthy */ |
                (1U << 8U) /* ephemeris orbit */ | nav_sat_sv::FLAGS_EPH_AVAIL | nav_sat_sv::FLAGS_SBAS_CORR_USED;
    CASTLE_SAMPLE_CHECK(castle::succeeded(sat.svs.push_back(gps)));

    nav_sat_sv galileo{};
    galileo.gnss_id = 2U; // Galileo
    galileo.sv_id = 20U;
    galileo.cno = 50U;
    galileo.elev = -15;
    galileo.azim = 270;
    galileo.pr_res = -10;
    galileo.flags = 1U /* searching */ | (2U << 8U) /* almanac orbit */ | nav_sat_sv::FLAGS_ALM_AVAIL;
    CASTLE_SAMPLE_CHECK(castle::succeeded(sat.svs.push_back(galileo)));

    // Encode into a complete UBX frame; numSvs comes from svs.size().
    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(sat, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + nav_sat<>::header_length + (2U * nav_sat<>::block_length)));

    // Parse the frame back.
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_sat<>::matches(raw));

    nav_sat<> decoded;
    CASTLE_SAMPLE_CHECK(nav_sat<>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(decoded.version == 1U);
    CASTLE_SAMPLE_CHECK(decoded.num_svs == 2U);
    CASTLE_SAMPLE_CHECK(decoded.svs.size() == 2U);

    CASTLE_SAMPLE_CHECK(decoded.svs[0U].sv_id == 10U);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].pr_res_m() > 2.49);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].pr_res_m() < 2.51);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].quality_ind() == nav_signal_quality::code_carrier_locked);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].sv_used());
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].health() == nav_sv_health::healthy);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].orbit_source() == nav_sat_orbit_source::ephemeris);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].eph_avail());
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].sbas_corr_used());
    CASTLE_SAMPLE_CHECK(!decoded.svs[0U].diff_corr());

    CASTLE_SAMPLE_CHECK(decoded.svs[1U].elev == -15);
    CASTLE_SAMPLE_CHECK(decoded.svs[1U].quality_ind() == nav_signal_quality::searching);
    CASTLE_SAMPLE_CHECK(!decoded.svs[1U].sv_used());
    CASTLE_SAMPLE_CHECK(decoded.svs[1U].health() == nav_sv_health::unknown);
    CASTLE_SAMPLE_CHECK(decoded.svs[1U].orbit_source() == nav_sat_orbit_source::almanac);
    CASTLE_SAMPLE_CHECK(decoded.svs[1U].alm_avail());

    // A smaller capacity keeps the first block and still reports the wire count.
    nav_sat<1U> clipped;
    CASTLE_SAMPLE_CHECK(nav_sat<1U>::decode(raw, clipped));
    CASTLE_SAMPLE_CHECK(clipped.num_svs == 2U);
    CASTLE_SAMPLE_CHECK(clipped.svs.size() == 1U);

    // A different NAV message ID does not match NAV-SAT.
    raw.header.msg_id = UBX_ID_NAV_STATUS;
    CASTLE_SAMPLE_CHECK(!nav_sat<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!nav_sat<>::decode(raw, decoded));

    return 0;
}
