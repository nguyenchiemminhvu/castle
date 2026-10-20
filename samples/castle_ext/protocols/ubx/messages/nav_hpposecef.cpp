#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_hpposecef.hpp"

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

    // X = 4282422.3456 m: 428242234 cm + 56 * 0.1 mm.
    nav_hpposecef position{};
    position.version = 0U;
    position.i_tow = 345600000U;
    position.ecef_x = 428242234;
    position.ecef_y = 64003318;
    position.ecef_z = -467233890;
    position.ecef_x_hp = 56;
    position.ecef_y_hp = -7;
    position.ecef_z_hp = -99;
    position.flags = 0U;
    position.p_acc = 142U; // 14.2 mm

    uint8_t frame[UBX_FRAME_OVERHEAD + nav_hpposecef::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(position, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_hpposecef::matches(raw));

    nav_hpposecef decoded{};
    CASTLE_SAMPLE_CHECK(nav_hpposecef::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == 0U);
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.ecef_x == 428242234);
    CASTLE_SAMPLE_CHECK(decoded.ecef_y == 64003318);
    CASTLE_SAMPLE_CHECK(decoded.ecef_z == -467233890);
    CASTLE_SAMPLE_CHECK(decoded.ecef_x_hp == 56);
    CASTLE_SAMPLE_CHECK(decoded.ecef_y_hp == -7);
    CASTLE_SAMPLE_CHECK(decoded.ecef_z_hp == -99);
    CASTLE_SAMPLE_CHECK(decoded.p_acc == 142U);

    // Combined cm + 0.1 mm coordinates in metres.
    CASTLE_SAMPLE_CHECK(!decoded.invalid_ecef());
    CASTLE_SAMPLE_CHECK(near(decoded.ecef_x_m(), 4282422.3456, 1e-7));
    CASTLE_SAMPLE_CHECK(near(decoded.ecef_y_m(), 640033.1793, 1e-7));
    CASTLE_SAMPLE_CHECK(near(decoded.ecef_z_m(), -4672338.9099, 1e-7));
    CASTLE_SAMPLE_CHECK(near(decoded.p_acc_m(), 0.0142, 1e-12));

    // The invalidEcef flag marks the coordinates unusable.
    decoded.flags = nav_hpposecef::FLAGS_INVALID_ECEF;
    CASTLE_SAMPLE_CHECK(decoded.invalid_ecef());

    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_POSECEF;
    CASTLE_SAMPLE_CHECK(!nav_hpposecef::matches(other));

    return 0;
}
