#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_status.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // A settled RTK-float fix with map matching applied and no spoofing.
    nav_status status;
    status.i_tow = 123456U;
    status.gps_fix = nav_status_gps_fix::fix_3d;
    status.flags = nav_status::FLAGS_GPS_FIX_OK | nav_status::FLAGS_WKN_SET | nav_status::FLAGS_TOW_SET;
    status.fix_stat = static_cast<uint8_t>(
        nav_status::FIX_STAT_DIFF_CORR | nav_status::FIX_STAT_CARR_SOLN_VALID
        | (static_cast<uint8_t>(nav_status_map_matching::valid_used) << nav_status::FIX_STAT_MAP_MATCHING_SHIFT));
    status.flags2 = static_cast<uint8_t>(
        (static_cast<uint8_t>(nav_status_psm_state::tracking) << nav_status::FLAGS2_PSM_STATE_SHIFT)
        | (static_cast<uint8_t>(nav_status_spoof_det_state::no_spoofing) << nav_status::FLAGS2_SPOOF_DET_STATE_SHIFT)
        | (static_cast<uint8_t>(nav_status_carr_soln::floating) << nav_status::FLAGS2_CARR_SOLN_SHIFT));
    status.ttff = 29000U;
    status.msss = 456789U;

    uint8_t frame[frame_size(nav_status::payload_length)];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame), written) == castle::status::ok);

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_status::matches(raw));

    nav_status decoded;
    CASTLE_SAMPLE_CHECK(nav_status::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(decoded.gps_fix == nav_status_gps_fix::fix_3d);
    CASTLE_SAMPLE_CHECK(decoded.ttff == 29000U);
    CASTLE_SAMPLE_CHECK(decoded.msss == 456789U);

    CASTLE_SAMPLE_CHECK(decoded.fix_ok());
    CASTLE_SAMPLE_CHECK(!decoded.diff_soln());
    CASTLE_SAMPLE_CHECK(decoded.wkn_set() && decoded.tow_set());
    CASTLE_SAMPLE_CHECK(decoded.diff_corr());
    CASTLE_SAMPLE_CHECK(decoded.carr_soln_valid());
    CASTLE_SAMPLE_CHECK(decoded.map_matching() == nav_status_map_matching::valid_used);
    CASTLE_SAMPLE_CHECK(decoded.psm_state() == nav_status_psm_state::tracking);
    CASTLE_SAMPLE_CHECK(decoded.spoof_det_state() == nav_status_spoof_det_state::no_spoofing);
    CASTLE_SAMPLE_CHECK(decoded.carr_soln() == nav_status_carr_soln::floating);

    // A different message does not match NAV-STATUS.
    raw.header.msg_id = UBX_ID_NAV_PVT;
    CASTLE_SAMPLE_CHECK(!nav_status::matches(raw));

    return 0;
}
