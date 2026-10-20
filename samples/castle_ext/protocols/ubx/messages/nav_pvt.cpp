#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_pvt.hpp"

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

    // Build a NAV-PVT solution: a confirmed 3D fix driving north-east.
    nav_pvt solution;
    solution.i_tow = 123456U;
    solution.year = 2026U;
    solution.month = 10U;
    solution.day = 2U;
    solution.hour = 12U;
    solution.minute = 34U;
    solution.second = 56U;
    solution.valid = nav_pvt::VALID_DATE | nav_pvt::VALID_TIME | nav_pvt::VALID_FULLY;
    solution.fix_type = nav_pvt_fix_type::fix_3d;
    solution.flags = static_cast<uint8_t>(nav_pvt::FLAGS_GNSS_FIX_OK | nav_pvt::FLAGS_HEAD_VEH_VALID
                                          | (static_cast<uint8_t>(nav_pvt_carr_soln::floating)
                                             << nav_pvt::FLAGS_CARR_SOLN_SHIFT));
    solution.flags2 = nav_pvt::FLAGS2_CONFIRMED_AVAI | nav_pvt::FLAGS2_CONFIRMED_DATE
                    | nav_pvt::FLAGS2_CONFIRMED_TIME;
    solution.num_sv = 18U;
    solution.lon = 139876543;   // 13.9876543 deg
    solution.lat = 358765432;   // 35.8765432 deg
    solution.height = 12345;    // 12.345 m
    solution.vel_n = 1500;      // 1.5 m/s
    solution.g_speed = 2000;    // 2.0 m/s
    solution.head_mot = 4500000; // 45 deg
    solution.p_dop = 145U;      // 1.45
    solution.mag_dec = -250;    // -2.5 deg

    // Encode the full UBX frame directly from the model.
    uint8_t frame[frame_size(nav_pvt::payload_length)];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(solution, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    // Decode the frame and the NAV-PVT payload.
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_pvt::matches(raw));

    nav_pvt pvt;
    CASTLE_SAMPLE_CHECK(nav_pvt::decode(raw, pvt));
    CASTLE_SAMPLE_CHECK(pvt.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(pvt.year == 2026U);
    CASTLE_SAMPLE_CHECK(pvt.minute == 34U);
    CASTLE_SAMPLE_CHECK(pvt.second == 56U);
    CASTLE_SAMPLE_CHECK(pvt.fix_type == nav_pvt_fix_type::fix_3d);
    CASTLE_SAMPLE_CHECK(pvt.num_sv == 18U);

    // Bit accessors.
    CASTLE_SAMPLE_CHECK(pvt.valid_date() && pvt.valid_time() && pvt.fully_resolved());
    CASTLE_SAMPLE_CHECK(!pvt.valid_mag());
    CASTLE_SAMPLE_CHECK(pvt.fix_ok());
    CASTLE_SAMPLE_CHECK(!pvt.diff_soln());
    CASTLE_SAMPLE_CHECK(pvt.head_veh_valid());
    CASTLE_SAMPLE_CHECK(pvt.psm_state() == nav_pvt_psm_state::not_active);
    CASTLE_SAMPLE_CHECK(pvt.carr_soln() == nav_pvt_carr_soln::floating);
    CASTLE_SAMPLE_CHECK(pvt.confirmed_avai() && pvt.confirmed_date() && pvt.confirmed_time());
    CASTLE_SAMPLE_CHECK(!pvt.invalid_llh());

    // Scaled helpers.
    CASTLE_SAMPLE_CHECK(pvt.longitude_deg() > 13.98 && pvt.longitude_deg() < 13.99);
    CASTLE_SAMPLE_CHECK(pvt.latitude_deg() > 35.87 && pvt.latitude_deg() < 35.88);
    CASTLE_SAMPLE_CHECK(near(pvt.height_m(), 12.345));
    CASTLE_SAMPLE_CHECK(near(pvt.vel_n_m_s(), 1.5));
    CASTLE_SAMPLE_CHECK(near(pvt.g_speed_m_s(), 2.0));
    CASTLE_SAMPLE_CHECK(near(pvt.head_mot_deg(), 45.0));
    CASTLE_SAMPLE_CHECK(near(pvt.pdop_value(), 1.45));
    CASTLE_SAMPLE_CHECK(near(pvt.mag_dec_deg(), -2.5));

    // A different message does not match, and a truncated payload is rejected.
    raw.header.msg_id = UBX_ID_NAV_STATUS;
    CASTLE_SAMPLE_CHECK(!nav_pvt::matches(raw));
    raw.header.msg_id = UBX_ID_NAV_PVT;
    raw.header.payload_length = static_cast<uint16_t>(nav_pvt::payload_length - 1U);
    CASTLE_SAMPLE_CHECK(!nav_pvt::decode(raw, pvt));

    return 0;
}
