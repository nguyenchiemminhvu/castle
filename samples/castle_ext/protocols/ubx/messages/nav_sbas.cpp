#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_sbas.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a NAV-SBAS value: EGNOS GEO 123 in integrity mode, two monitored SVs.
    nav_sbas<4U> sbas{};
    sbas.i_tow = 500000U;
    sbas.geo = 123U;
    sbas.mode = nav_sbas_mode::enabled_integrity;
    sbas.sys = nav_sbas_system::egnos;
    sbas.service = nav_sbas<>::SERVICE_RANGING | nav_sbas<>::SERVICE_CORRECTIONS | nav_sbas<>::SERVICE_INTEGRITY;
    sbas.status_flags = static_cast<uint8_t>(nav_sbas_integrity_used::gps_only);

    nav_sbas_sv gps{};
    gps.svid = 7U;
    gps.udre = 3U;
    gps.sv_sys = nav_sbas_system::gps;
    gps.sv_service = nav_sbas_sv::SV_SERVICE_CORRECTIONS;
    gps.prc = -152; // -1.52 m
    gps.ic = 245;   //  2.45 m
    CASTLE_SAMPLE_CHECK(castle::succeeded(sbas.svs.push_back(gps)));

    nav_sbas_sv geo{};
    geo.svid = 123U;
    geo.sv_sys = nav_sbas_system::egnos;
    geo.sv_service = nav_sbas_sv::SV_SERVICE_RANGING | nav_sbas_sv::SV_SERVICE_BAD;
    CASTLE_SAMPLE_CHECK(castle::succeeded(sbas.svs.push_back(geo)));

    // Encode, then parse the frame back.
    uint8_t frame[48U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(sbas, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + nav_sbas<>::header_length + (2U * nav_sbas<>::block_length)));

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_sbas<>::matches(raw));

    nav_sbas<> decoded;
    CASTLE_SAMPLE_CHECK(nav_sbas<>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.geo == 123U);
    CASTLE_SAMPLE_CHECK(decoded.mode == nav_sbas_mode::enabled_integrity);
    CASTLE_SAMPLE_CHECK(decoded.sys == nav_sbas_system::egnos);
    CASTLE_SAMPLE_CHECK(decoded.ranging());
    CASTLE_SAMPLE_CHECK(decoded.corrections());
    CASTLE_SAMPLE_CHECK(decoded.integrity());
    CASTLE_SAMPLE_CHECK(!decoded.test_mode());
    CASTLE_SAMPLE_CHECK(!decoded.bad());
    CASTLE_SAMPLE_CHECK(decoded.integrity_used() == nav_sbas_integrity_used::gps_only);
    CASTLE_SAMPLE_CHECK(decoded.cnt == 2U);
    CASTLE_SAMPLE_CHECK(decoded.svs.size() == 2U);

    CASTLE_SAMPLE_CHECK(decoded.svs[0U].sv_sys == nav_sbas_system::gps);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].corrections());
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].prc_m() < -1.51);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].prc_m() > -1.53);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].ic_m() > 2.44);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].ic_m() < 2.46);
    CASTLE_SAMPLE_CHECK(decoded.svs[1U].ranging());
    CASTLE_SAMPLE_CHECK(decoded.svs[1U].bad());
    CASTLE_SAMPLE_CHECK(!decoded.svs[1U].integrity());
    CASTLE_SAMPLE_CHECK(!decoded.svs[1U].test_mode());

    // A smaller capacity keeps the first block and still reports the wire count.
    nav_sbas<1U> clipped;
    CASTLE_SAMPLE_CHECK(nav_sbas<1U>::decode(raw, clipped));
    CASTLE_SAMPLE_CHECK(clipped.cnt == 2U);
    CASTLE_SAMPLE_CHECK(clipped.svs.size() == 1U);

    raw.header.msg_id = UBX_ID_NAV_SLAS;
    CASTLE_SAMPLE_CHECK(!nav_sbas<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!nav_sbas<>::decode(raw, decoded));

    return 0;
}
