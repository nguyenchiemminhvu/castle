#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_relposned.hpp"

namespace
{
bool near(double value, double expected, double tolerance)
{
    const double difference = value - expected;
    return (difference <= tolerance) && (difference >= -tolerance);
}
} // namespace

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Moving-base heading solution: 1.2345 m baseline, RTK fixed.
    nav_relposned relative{};
    relative.version = 1U;
    relative.ref_station_id = 42U;
    relative.i_tow = 345600000U;
    relative.rel_pos_n = 87;
    relative.rel_pos_e = 87;
    relative.rel_pos_d = -2;
    relative.rel_pos_length = 123;
    relative.rel_pos_heading = 4512345; // 45.12345 deg
    relative.rel_pos_hp_n = 3;
    relative.rel_pos_hp_e = -4;
    relative.rel_pos_hp_d = 5;
    relative.rel_pos_hp_length = 45;
    relative.acc_n = 80U;
    relative.acc_e = 81U;
    relative.acc_d = 150U;
    relative.acc_length = 90U;
    relative.acc_heading = 25000U;      // 0.25 deg
    relative.flags = nav_relposned::FLAGS_GNSS_FIX_OK
        | nav_relposned::FLAGS_DIFF_SOLN
        | nav_relposned::FLAGS_REL_POS_VALID
        | (static_cast<uint32_t>(nav_relposned_carr_soln::fixed) << nav_relposned::FLAGS_CARR_SOLN_SHIFT)
        | nav_relposned::FLAGS_IS_MOVING
        | nav_relposned::FLAGS_REL_POS_HEADING_VALID;

    uint8_t frame[UBX_FRAME_OVERHEAD + nav_relposned::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(relative, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_relposned::matches(raw));

    nav_relposned decoded{};
    CASTLE_SAMPLE_CHECK(nav_relposned::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == 1U);
    CASTLE_SAMPLE_CHECK(decoded.ref_station_id == 42U);
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.rel_pos_n == 87);
    CASTLE_SAMPLE_CHECK(decoded.rel_pos_e == 87);
    CASTLE_SAMPLE_CHECK(decoded.rel_pos_d == -2);
    CASTLE_SAMPLE_CHECK(decoded.rel_pos_length == 123);
    CASTLE_SAMPLE_CHECK(decoded.rel_pos_heading == 4512345);
    CASTLE_SAMPLE_CHECK(decoded.rel_pos_hp_n == 3);
    CASTLE_SAMPLE_CHECK(decoded.rel_pos_hp_e == -4);
    CASTLE_SAMPLE_CHECK(decoded.rel_pos_hp_d == 5);
    CASTLE_SAMPLE_CHECK(decoded.rel_pos_hp_length == 45);
    CASTLE_SAMPLE_CHECK(decoded.acc_n == 80U);
    CASTLE_SAMPLE_CHECK(decoded.acc_e == 81U);
    CASTLE_SAMPLE_CHECK(decoded.acc_d == 150U);
    CASTLE_SAMPLE_CHECK(decoded.acc_length == 90U);
    CASTLE_SAMPLE_CHECK(decoded.acc_heading == 25000U);
    CASTLE_SAMPLE_CHECK(decoded.flags == relative.flags);

    // Flag accessors.
    CASTLE_SAMPLE_CHECK(decoded.gnss_fix_ok());
    CASTLE_SAMPLE_CHECK(decoded.diff_soln());
    CASTLE_SAMPLE_CHECK(decoded.rel_pos_valid());
    CASTLE_SAMPLE_CHECK(decoded.carr_soln() == nav_relposned_carr_soln::fixed);
    CASTLE_SAMPLE_CHECK(decoded.is_moving());
    CASTLE_SAMPLE_CHECK(!decoded.ref_pos_miss());
    CASTLE_SAMPLE_CHECK(!decoded.ref_obs_miss());
    CASTLE_SAMPLE_CHECK(decoded.rel_pos_heading_valid());
    CASTLE_SAMPLE_CHECK(!decoded.rel_pos_normalized());

    // Combined cm + 0.1 mm components and scaled accuracies.
    CASTLE_SAMPLE_CHECK(near(decoded.rel_pos_n_m(), 0.8703, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.rel_pos_e_m(), 0.8696, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.rel_pos_d_m(), -0.0195, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.rel_pos_length_m(), 1.2345, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.rel_pos_heading_deg(), 45.12345, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.acc_n_m(), 0.008, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.acc_e_m(), 0.0081, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.acc_d_m(), 0.015, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.acc_length_m(), 0.009, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.acc_heading_deg(), 0.25, 1e-12));

    // Float ambiguities are reported through the same accessor.
    decoded.flags = static_cast<uint32_t>(nav_relposned_carr_soln::floating) << nav_relposned::FLAGS_CARR_SOLN_SHIFT;
    CASTLE_SAMPLE_CHECK(decoded.carr_soln() == nav_relposned_carr_soln::floating);
    decoded.flags = 0U;
    CASTLE_SAMPLE_CHECK(decoded.carr_soln() == nav_relposned_carr_soln::none);

    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_PVT;
    CASTLE_SAMPLE_CHECK(!nav_relposned::matches(other));

    return 0;
}
