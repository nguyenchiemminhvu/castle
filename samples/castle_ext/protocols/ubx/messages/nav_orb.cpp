#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_orb.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a NAV-ORB value describing two satellites of the orbit database.
    nav_orb<4U> orb{};
    orb.i_tow = 345600U;
    orb.version = 1U;

    nav_orb_sv gps{};
    gps.gnss_id = 0U;
    gps.sv_id = 3U;
    gps.sv_flag = 1U /* healthy */ | (3U << 2U) /* above elevation mask */;
    gps.eph = static_cast<uint8_t>((1U << 5U) | 8U); // broadcast ephemeris, usable 105..120 min
    gps.alm = static_cast<uint8_t>((1U << 5U) | nav_orb_sv::USABILITY_MAX);
    gps.other_orb = static_cast<uint8_t>((2U << 5U) | nav_orb_sv::USABILITY_UNKNOWN); // AssistNow Autonomous
    CASTLE_SAMPLE_CHECK(castle::succeeded(orb.svs.push_back(gps)));

    nav_orb_sv glonass{};
    glonass.gnss_id = 6U;
    glonass.sv_id = 14U;
    glonass.sv_flag = 2U /* unhealthy */ | (1U << 2U) /* below horizon */;
    glonass.eph = static_cast<uint8_t>((2U << 5U) | nav_orb_sv::USABILITY_EXPIRED); // external aiding, expired
    CASTLE_SAMPLE_CHECK(castle::succeeded(orb.svs.push_back(glonass)));

    // Encode, then parse the frame back.
    uint8_t frame[32U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(orb, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + nav_orb<>::header_length + (2U * nav_orb<>::block_length)));

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_orb<>::matches(raw));

    nav_orb<> decoded;
    CASTLE_SAMPLE_CHECK(nav_orb<>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600U);
    CASTLE_SAMPLE_CHECK(decoded.num_sv == 2U);
    CASTLE_SAMPLE_CHECK(decoded.svs.size() == 2U);

    CASTLE_SAMPLE_CHECK(decoded.svs[0U].health() == nav_sv_health::healthy);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].visibility() == nav_orb_visibility::above_elevation_mask);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].eph_usability() == 8U);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].eph_source() == nav_orb_source::gnss_transmission);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].alm_usability() == nav_orb_sv::USABILITY_MAX);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].alm_source() == nav_orb_source::gnss_transmission);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].ano_aop_usability() == nav_orb_sv::USABILITY_UNKNOWN);
    CASTLE_SAMPLE_CHECK(decoded.svs[0U].other_orb_type() == nav_orb_other_type::assistnow_autonomous);

    CASTLE_SAMPLE_CHECK(decoded.svs[1U].health() == nav_sv_health::unhealthy);
    CASTLE_SAMPLE_CHECK(decoded.svs[1U].visibility() == nav_orb_visibility::below_horizon);
    CASTLE_SAMPLE_CHECK(decoded.svs[1U].eph_usability() == nav_orb_sv::USABILITY_EXPIRED);
    CASTLE_SAMPLE_CHECK(decoded.svs[1U].eph_source() == nav_orb_source::external_aiding);
    CASTLE_SAMPLE_CHECK(decoded.svs[1U].alm_source() == nav_orb_source::not_available);
    CASTLE_SAMPLE_CHECK(decoded.svs[1U].other_orb_type() == nav_orb_other_type::none);

    // A smaller capacity keeps the first block and still reports the wire count.
    nav_orb<1U> clipped;
    CASTLE_SAMPLE_CHECK(nav_orb<1U>::decode(raw, clipped));
    CASTLE_SAMPLE_CHECK(clipped.num_sv == 2U);
    CASTLE_SAMPLE_CHECK(clipped.svs.size() == 1U);

    raw.header.msg_id = UBX_ID_NAV_SAT;
    CASTLE_SAMPLE_CHECK(!nav_orb<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!nav_orb<>::decode(raw, decoded));

    return 0;
}
