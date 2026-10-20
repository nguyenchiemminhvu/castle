#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_eell.hpp"

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

    // Build a NAV-EELL payload at the spec offsets:
    // iTOW U4@0, version U1@4, reserved0 U1@5, errEllipseOrient U2@6,
    // errEllipseMajor U4@8, errEllipseMinor U4@12.
    uint8_t payload[nav_eell::payload_length] = {};
    castle::write_le32(&payload[0U], 123456U);   // iTOW
    payload[4U] = 0U;                            // version
    castle::write_le16(&payload[6U], 4500U);     // errEllipseOrient: 45.00 deg
    castle::write_le32(&payload[8U], 125000U);   // errEllipseMajor: 125 m (needs U4)
    castle::write_le32(&payload[12U], 2500U);    // errEllipseMinor: 2.5 m

    uint8_t frame[frame_size(nav_eell::payload_length)];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_frame(
        nav_eell::msg_class, nav_eell::msg_id,
        castle::container::array_view<const uint8_t>(payload, nav_eell::payload_length),
        frame, sizeof(frame), written)));

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_eell::matches(raw));

    nav_eell ellipse;
    CASTLE_SAMPLE_CHECK(nav_eell::decode(raw, ellipse));
    CASTLE_SAMPLE_CHECK(ellipse.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(ellipse.version == 0U);
    CASTLE_SAMPLE_CHECK(ellipse.err_orient == 4500U);
    CASTLE_SAMPLE_CHECK(ellipse.err_maj == 125000U);
    CASTLE_SAMPLE_CHECK(ellipse.err_min == 2500U);
    CASTLE_SAMPLE_CHECK(near(ellipse.err_orient_deg(), 45.0));
    CASTLE_SAMPLE_CHECK(near(ellipse.err_maj_m(), 125.0));
    CASTLE_SAMPLE_CHECK(near(ellipse.err_min_m(), 2.5));

    // Re-encoding the decoded model reproduces the original frame.
    uint8_t again[sizeof(frame)];
    castle::size_type again_written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(ellipse, again, sizeof(again), again_written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(again_written == written);
    for (castle::size_type i = 0U; i < written; ++i)
    {
        CASTLE_SAMPLE_CHECK(again[i] == frame[i]);
    }

    // Different message types do not match NAV-EELL.
    message_view different{};
    different.header.msg_class = UBX_CLASS_CFG;
    different.header.msg_id = UBX_ID_CFG_VALSET;
    CASTLE_SAMPLE_CHECK(!nav_eell::matches(different));

    return 0;
}
