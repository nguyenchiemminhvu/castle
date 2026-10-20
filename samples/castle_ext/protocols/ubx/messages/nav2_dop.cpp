#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav2_dop.hpp"

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

    // NAV2-DOP is the secondary-output instance of the NAV-DOP layout.
    nav2_dop dop;
    dop.i_tow = 98765U;
    dop.g_dop = 320U;
    dop.p_dop = 210U;
    dop.t_dop = 140U;
    dop.v_dop = 175U;
    dop.h_dop = 125U;
    dop.n_dop = 90U;
    dop.e_dop = 85U;

    uint8_t frame[frame_size(nav2_dop::payload_length)];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(dop, frame, sizeof(frame), written) == castle::status::ok);

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_NAV2, UBX_ID_NAV2_DOP));
    CASTLE_SAMPLE_CHECK(nav2_dop::matches(raw));
    CASTLE_SAMPLE_CHECK(!nav_dop::matches(raw));

    nav2_dop decoded;
    CASTLE_SAMPLE_CHECK(nav2_dop::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 98765U);
    CASTLE_SAMPLE_CHECK(decoded.e_dop == 85U);
    CASTLE_SAMPLE_CHECK(near(decoded.gdop_value(), 3.2));
    CASTLE_SAMPLE_CHECK(near(decoded.pdop_value(), 2.1));
    CASTLE_SAMPLE_CHECK(near(decoded.tdop_value(), 1.4));
    CASTLE_SAMPLE_CHECK(near(decoded.vdop_value(), 1.75));
    CASTLE_SAMPLE_CHECK(near(decoded.hdop_value(), 1.25));
    CASTLE_SAMPLE_CHECK(near(decoded.ndop_value(), 0.9));
    CASTLE_SAMPLE_CHECK(near(decoded.edop_value(), 0.85));

    // The NAV-class message with the same ID is not NAV2-DOP.
    raw.header.msg_class = UBX_CLASS_NAV;
    CASTLE_SAMPLE_CHECK(!nav2_dop::decode(raw, decoded));

    return 0;
}
