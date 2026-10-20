#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav2_pvt.hpp"

namespace
{
bool near(double actual, double expected)
{
    CASTLE_CONST double diff = actual - expected;
    return (diff < 1e-9) && (diff > -1e-9);
}
} // namespace

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // NAV2-PVT is the secondary-output instance of the NAV-PVT layout.
    nav2_pvt solution;
    solution.i_tow = 654321U;
    solution.year = 2026U;
    solution.month = 10U;
    solution.day = 20U;
    solution.hour = 8U;
    solution.minute = 15U;
    solution.second = 30U;
    solution.valid = nav2_pvt::VALID_DATE | nav2_pvt::VALID_TIME | nav2_pvt::VALID_MAG_DEC;
    solution.fix_type = nav_pvt_fix_type::gnss_dr;
    solution.flags = static_cast<uint8_t>(nav2_pvt::FLAGS_GNSS_FIX_OK | nav2_pvt::FLAGS_DIFF_SOLN
                                          | (static_cast<uint8_t>(nav_pvt_psm_state::tracking)
                                             << nav2_pvt::FLAGS_PSM_STATE_SHIFT));
    solution.flags3 = nav2_pvt::FLAGS3_INVALID_LLH;
    solution.num_sv = 9U;
    solution.lon = -1223456789;  // -122.3456789 deg
    solution.lat = 473456789;    // 47.3456789 deg
    solution.h_msl = -5000;      // -5 m
    solution.vel_d = -250;       // -0.25 m/s (climbing)
    solution.head_veh = 18000000; // 180 deg
    solution.mag_acc = 50U;      // 0.5 deg

    uint8_t frame[frame_size(nav2_pvt::payload_length)];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(solution, frame, sizeof(frame), written) == castle::status::ok);

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_NAV2, UBX_ID_NAV2_PVT));
    CASTLE_SAMPLE_CHECK(nav2_pvt::matches(raw));
    CASTLE_SAMPLE_CHECK(!nav_pvt::matches(raw)); // same ID, different class

    nav2_pvt pvt;
    CASTLE_SAMPLE_CHECK(nav2_pvt::decode(raw, pvt));
    CASTLE_SAMPLE_CHECK(pvt.i_tow == 654321U);
    CASTLE_SAMPLE_CHECK(pvt.minute == 15U);
    CASTLE_SAMPLE_CHECK(pvt.second == 30U);
    CASTLE_SAMPLE_CHECK(pvt.fix_type == nav_pvt_fix_type::gnss_dr);
    CASTLE_SAMPLE_CHECK(pvt.fix_ok() && pvt.diff_soln());
    CASTLE_SAMPLE_CHECK(pvt.valid_mag());
    CASTLE_SAMPLE_CHECK(pvt.psm_state() == nav_pvt_psm_state::tracking);
    CASTLE_SAMPLE_CHECK(pvt.carr_soln() == nav_pvt_carr_soln::none);
    CASTLE_SAMPLE_CHECK(!pvt.confirmed_time());
    CASTLE_SAMPLE_CHECK(pvt.invalid_llh());
    CASTLE_SAMPLE_CHECK(pvt.longitude_deg() < -122.34 && pvt.longitude_deg() > -122.35);
    CASTLE_SAMPLE_CHECK(near(pvt.h_msl_m(), -5.0));
    CASTLE_SAMPLE_CHECK(near(pvt.vel_d_m_s(), -0.25));
    CASTLE_SAMPLE_CHECK(near(pvt.head_veh_deg(), 180.0));
    CASTLE_SAMPLE_CHECK(near(pvt.mag_acc_deg(), 0.5));

    // A NAV-PVT frame is not accepted as NAV2-PVT.
    nav_pvt primary;
    CASTLE_SAMPLE_CHECK(encode_message(primary, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(!nav2_pvt::decode(raw, pvt));

    return 0;
}
