#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav2_eell.hpp"

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

    // NAV2-EELL is the secondary-output instance of the NAV-EELL layout.
    nav2_eell ellipse;
    ellipse.i_tow = 777000U;
    ellipse.version = 0U;
    ellipse.err_orient = 9000U;  // 90.00 deg: semi-major axis points east
    ellipse.err_maj = 80000U;    // 80 m
    ellipse.err_min = 1500U;     // 1.5 m

    uint8_t frame[frame_size(nav2_eell::payload_length)];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(ellipse, frame, sizeof(frame), written) == castle::status::ok);

    // Wire check: orientation is a U2 at offset 6, the axes are U4 at 8 and 12.
    CASTLE_SAMPLE_CHECK(castle::read_le16(&frame[UBX_FRAME_INDEX_PAYLOAD + 6U]) == 9000U);
    CASTLE_SAMPLE_CHECK(castle::read_le32(&frame[UBX_FRAME_INDEX_PAYLOAD + 8U]) == 80000U);
    CASTLE_SAMPLE_CHECK(castle::read_le32(&frame[UBX_FRAME_INDEX_PAYLOAD + 12U]) == 1500U);

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_NAV2, UBX_ID_NAV2_EELL));
    CASTLE_SAMPLE_CHECK(nav2_eell::matches(raw));
    CASTLE_SAMPLE_CHECK(!nav_eell::matches(raw));

    nav2_eell decoded;
    CASTLE_SAMPLE_CHECK(nav2_eell::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 777000U);
    CASTLE_SAMPLE_CHECK(decoded.err_orient == 9000U);
    CASTLE_SAMPLE_CHECK(decoded.err_maj == 80000U);
    CASTLE_SAMPLE_CHECK(decoded.err_min == 1500U);
    CASTLE_SAMPLE_CHECK(near(decoded.err_orient_deg(), 90.0));
    CASTLE_SAMPLE_CHECK(near(decoded.err_maj_m(), 80.0));
    CASTLE_SAMPLE_CHECK(near(decoded.err_min_m(), 1.5));

    return 0;
}
