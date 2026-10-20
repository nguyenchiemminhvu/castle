#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_velned.hpp"

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

    // Vehicle heading north-east at about 20 m/s.
    nav_velned velocity{};
    velocity.i_tow = 345600000U;
    velocity.vel_n = 1414;
    velocity.vel_e = 1414;
    velocity.vel_d = -12;
    velocity.speed = 2000U;
    velocity.g_speed = 1999U;
    velocity.heading = 4500000;  // 45.00000 deg
    velocity.s_acc = 30U;
    velocity.c_acc = 125000U;    // 1.25 deg

    uint8_t frame[UBX_FRAME_OVERHEAD + nav_velned::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(velocity, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_velned::matches(raw));

    nav_velned decoded{};
    CASTLE_SAMPLE_CHECK(nav_velned::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.vel_n == 1414);
    CASTLE_SAMPLE_CHECK(decoded.vel_e == 1414);
    CASTLE_SAMPLE_CHECK(decoded.vel_d == -12);
    CASTLE_SAMPLE_CHECK(decoded.speed == 2000U);
    CASTLE_SAMPLE_CHECK(decoded.g_speed == 1999U);
    CASTLE_SAMPLE_CHECK(decoded.heading == 4500000);
    CASTLE_SAMPLE_CHECK(decoded.s_acc == 30U);
    CASTLE_SAMPLE_CHECK(decoded.c_acc == 125000U);

    CASTLE_SAMPLE_CHECK(near(decoded.vel_n_m_s(), 14.14, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.vel_e_m_s(), 14.14, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.vel_d_m_s(), -0.12, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.speed_m_s(), 20.0, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.g_speed_m_s(), 19.99, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.heading_deg(), 45.0, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.s_acc_m_s(), 0.30, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.c_acc_deg(), 1.25, 1e-9));

    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_VELECEF;
    CASTLE_SAMPLE_CHECK(!nav_velned::matches(other));

    return 0;
}
