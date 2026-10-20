#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_att.hpp"

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

    // Angles and accuracies are transmitted in 1e-5 deg.
    nav_att attitude;
    attitude.i_tow = 123456U;
    attitude.version = 0U;
    attitude.roll = -150000;       // -1.5 deg
    attitude.pitch = 250000;       // 2.5 deg
    attitude.heading = 9000000;    // 90 deg
    attitude.acc_roll = 50000U;    // 0.5 deg
    attitude.acc_pitch = 60000U;   // 0.6 deg
    attitude.acc_heading = 0U;     // heading not available

    uint8_t frame[frame_size(nav_att::payload_length)];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(attitude, frame, sizeof(frame), written) == castle::status::ok);

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_att::matches(raw));

    nav_att decoded;
    CASTLE_SAMPLE_CHECK(nav_att::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(decoded.roll == -150000);
    CASTLE_SAMPLE_CHECK(near(decoded.roll_deg(), -1.5));
    CASTLE_SAMPLE_CHECK(near(decoded.pitch_deg(), 2.5));
    CASTLE_SAMPLE_CHECK(near(decoded.heading_deg(), 90.0));
    CASTLE_SAMPLE_CHECK(near(decoded.acc_roll_deg(), 0.5));
    CASTLE_SAMPLE_CHECK(near(decoded.acc_pitch_deg(), 0.6));
    CASTLE_SAMPLE_CHECK(decoded.acc_heading == 0U); // zero accuracy: heading unavailable
    CASTLE_SAMPLE_CHECK(near(decoded.acc_heading_deg(), 0.0));

    // A different message does not match NAV-ATT.
    raw.header.msg_id = UBX_ID_NAV_PVT;
    CASTLE_SAMPLE_CHECK(!nav_att::matches(raw));

    return 0;
}
