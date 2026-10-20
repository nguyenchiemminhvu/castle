#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_posecef.hpp"

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

    // ECEF position near Zurich, in centimetres.
    nav_posecef position{};
    position.i_tow = 345600000U;
    position.ecef_x = 428242234;
    position.ecef_y = 64003318;
    position.ecef_z = 467233890;
    position.p_acc = 150U;

    uint8_t frame[UBX_FRAME_OVERHEAD + nav_posecef::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(position, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_posecef::matches(raw));

    nav_posecef decoded{};
    CASTLE_SAMPLE_CHECK(nav_posecef::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.ecef_x == 428242234);
    CASTLE_SAMPLE_CHECK(decoded.ecef_y == 64003318);
    CASTLE_SAMPLE_CHECK(decoded.ecef_z == 467233890);
    CASTLE_SAMPLE_CHECK(decoded.p_acc == 150U);

    // Metre helpers.
    CASTLE_SAMPLE_CHECK(near(decoded.ecef_x_m(), 4282422.34, 1e-6));
    CASTLE_SAMPLE_CHECK(near(decoded.ecef_y_m(), 640033.18, 1e-6));
    CASTLE_SAMPLE_CHECK(near(decoded.ecef_z_m(), 4672338.90, 1e-6));
    CASTLE_SAMPLE_CHECK(near(decoded.p_acc_m(), 1.5, 1e-9));

    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_HPPOSECEF;
    CASTLE_SAMPLE_CHECK(!nav_posecef::matches(other));

    return 0;
}
