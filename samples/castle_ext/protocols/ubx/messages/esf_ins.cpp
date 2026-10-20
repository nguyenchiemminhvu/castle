#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/esf_ins.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Version 1, angular rates valid on all axes, only x/y acceleration valid.
    esf_ins dynamics{};
    dynamics.bitfield0 = 0x01U
        | esf_ins::BF0_X_ANG_RATE_VALID
        | esf_ins::BF0_Y_ANG_RATE_VALID
        | esf_ins::BF0_Z_ANG_RATE_VALID
        | esf_ins::BF0_X_ACCEL_VALID
        | esf_ins::BF0_Y_ACCEL_VALID;
    dynamics.i_tow = 123456U;
    dynamics.x_ang_rate = 1500;  // 1.5 deg/s
    dynamics.y_ang_rate = -250;  // -0.25 deg/s
    dynamics.z_ang_rate = 10000; // 10 deg/s
    dynamics.x_accel = 981;      // 9.81 m/s^2
    dynamics.y_accel = -50;      // -0.5 m/s^2
    dynamics.z_accel = 0;

    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(dynamics, frame, sizeof(frame), written) == castle::status::ok);

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(esf_ins::matches(raw));

    esf_ins decoded;
    CASTLE_SAMPLE_CHECK(esf_ins::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version() == 0x01U);
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(decoded.x_ang_rate_valid());
    CASTLE_SAMPLE_CHECK(decoded.y_ang_rate_valid());
    CASTLE_SAMPLE_CHECK(decoded.z_ang_rate_valid());
    CASTLE_SAMPLE_CHECK(decoded.x_accel_valid());
    CASTLE_SAMPLE_CHECK(decoded.y_accel_valid());
    CASTLE_SAMPLE_CHECK(!decoded.z_accel_valid());

    CASTLE_SAMPLE_CHECK((decoded.x_ang_rate_deg_s() > 1.4999) && (decoded.x_ang_rate_deg_s() < 1.5001));
    CASTLE_SAMPLE_CHECK((decoded.y_ang_rate_deg_s() > -0.2501) && (decoded.y_ang_rate_deg_s() < -0.2499));
    CASTLE_SAMPLE_CHECK((decoded.z_ang_rate_deg_s() > 9.9999) && (decoded.z_ang_rate_deg_s() < 10.0001));
    CASTLE_SAMPLE_CHECK((decoded.x_accel_m_s2() > 9.8099) && (decoded.x_accel_m_s2() < 9.8101));
    CASTLE_SAMPLE_CHECK((decoded.y_accel_m_s2() > -0.5001) && (decoded.y_accel_m_s2() < -0.4999));
    CASTLE_SAMPLE_CHECK(decoded.z_accel_m_s2() == 0.0);

    return 0;
}
