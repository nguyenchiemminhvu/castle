#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_dop.hpp"

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

    // DOP values are transmitted scaled by 100.
    nav_dop dop;
    dop.i_tow = 123456U;
    dop.g_dop = 250U;
    dop.p_dop = 180U;
    dop.t_dop = 120U;
    dop.v_dop = 150U;
    dop.h_dop = 100U;
    dop.n_dop = 70U;
    dop.e_dop = 60U;

    uint8_t frame[frame_size(nav_dop::payload_length)];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(dop, frame, sizeof(frame), written) == castle::status::ok);

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_dop::matches(raw));

    nav_dop decoded;
    CASTLE_SAMPLE_CHECK(nav_dop::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(decoded.h_dop == 100U);
    CASTLE_SAMPLE_CHECK(near(decoded.gdop_value(), 2.5));
    CASTLE_SAMPLE_CHECK(near(decoded.pdop_value(), 1.8));
    CASTLE_SAMPLE_CHECK(near(decoded.tdop_value(), 1.2));
    CASTLE_SAMPLE_CHECK(near(decoded.vdop_value(), 1.5));
    CASTLE_SAMPLE_CHECK(near(decoded.hdop_value(), 1.0));
    CASTLE_SAMPLE_CHECK(near(decoded.ndop_value(), 0.7));
    CASTLE_SAMPLE_CHECK(near(decoded.edop_value(), 0.6));

    // A frame shorter than 18 payload bytes is rejected.
    raw.header.payload_length = 17U;
    CASTLE_SAMPLE_CHECK(!nav_dop::decode(raw, decoded));

    return 0;
}
