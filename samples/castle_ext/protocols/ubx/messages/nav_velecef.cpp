#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_velecef.hpp"

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

    // ECEF velocity in cm/s.
    nav_velecef velocity{};
    velocity.i_tow = 345600000U;
    velocity.ecef_vx = -1250;
    velocity.ecef_vy = 380;
    velocity.ecef_vz = 1105;
    velocity.s_acc = 45U;

    uint8_t frame[UBX_FRAME_OVERHEAD + nav_velecef::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(velocity, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_velecef::matches(raw));

    nav_velecef decoded{};
    CASTLE_SAMPLE_CHECK(nav_velecef::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.ecef_vx == -1250);
    CASTLE_SAMPLE_CHECK(decoded.ecef_vy == 380);
    CASTLE_SAMPLE_CHECK(decoded.ecef_vz == 1105);
    CASTLE_SAMPLE_CHECK(decoded.s_acc == 45U);

    CASTLE_SAMPLE_CHECK(near(decoded.ecef_vx_m_s(), -12.50, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.ecef_vy_m_s(), 3.80, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.ecef_vz_m_s(), 11.05, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.s_acc_m_s(), 0.45, 1e-9));

    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_VELNED;
    CASTLE_SAMPLE_CHECK(!nav_velecef::matches(other));

    return 0;
}
