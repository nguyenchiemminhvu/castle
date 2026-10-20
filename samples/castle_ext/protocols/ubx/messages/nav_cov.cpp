#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_cov.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Upper triangles of the NED position and velocity covariance matrices.
    nav_cov covariance{};
    covariance.i_tow = 345600000U;
    covariance.version = 0U;
    covariance.pos_cov_valid = 1U;
    covariance.vel_cov_valid = 1U;
    covariance.pos_cov_nn = 0.25F;
    covariance.pos_cov_ne = 0.015625F;
    covariance.pos_cov_nd = -0.03125F;
    covariance.pos_cov_ee = 0.5F;
    covariance.pos_cov_ed = 0.0625F;
    covariance.pos_cov_dd = 1.5F;
    covariance.vel_cov_nn = 0.0078125F;
    covariance.vel_cov_ne = -0.00390625F;
    covariance.vel_cov_nd = 0.001953125F;
    covariance.vel_cov_ee = 0.0078125F;
    covariance.vel_cov_ed = 0.0009765625F;
    covariance.vel_cov_dd = 0.015625F;

    uint8_t frame[UBX_FRAME_OVERHEAD + nav_cov::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(covariance, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_cov::matches(raw));

    nav_cov decoded{};
    CASTLE_SAMPLE_CHECK(nav_cov::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.version == 0U);
    CASTLE_SAMPLE_CHECK(decoded.pos_cov_valid == 1U);
    CASTLE_SAMPLE_CHECK(decoded.vel_cov_valid == 1U);

    // R4 values travel bit-exactly, so dyadic test values compare equal.
    CASTLE_SAMPLE_CHECK(decoded.pos_cov_nn == 0.25F);
    CASTLE_SAMPLE_CHECK(decoded.pos_cov_ne == 0.015625F);
    CASTLE_SAMPLE_CHECK(decoded.pos_cov_nd == -0.03125F);
    CASTLE_SAMPLE_CHECK(decoded.pos_cov_ee == 0.5F);
    CASTLE_SAMPLE_CHECK(decoded.pos_cov_ed == 0.0625F);
    CASTLE_SAMPLE_CHECK(decoded.pos_cov_dd == 1.5F);
    CASTLE_SAMPLE_CHECK(decoded.vel_cov_nn == 0.0078125F);
    CASTLE_SAMPLE_CHECK(decoded.vel_cov_ne == -0.00390625F);
    CASTLE_SAMPLE_CHECK(decoded.vel_cov_nd == 0.001953125F);
    CASTLE_SAMPLE_CHECK(decoded.vel_cov_ee == 0.0078125F);
    CASTLE_SAMPLE_CHECK(decoded.vel_cov_ed == 0.0009765625F);
    CASTLE_SAMPLE_CHECK(decoded.vel_cov_dd == 0.015625F);

    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_EELL;
    CASTLE_SAMPLE_CHECK(!nav_cov::matches(other));

    return 0;
}
