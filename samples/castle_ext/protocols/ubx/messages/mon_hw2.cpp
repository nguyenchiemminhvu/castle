#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_hw2.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using castle::protocols::ubx::messages::mon_hw2;
    using castle::protocols::ubx::messages::mon_hw2_cfg_source;

    mon_hw2 status{};
    status.ofs_i = -3;
    status.mag_i = 140U;
    status.ofs_q = 2;
    status.mag_q = 138U;
    status.cfg_source = mon_hw2_cfg_source::otp;
    status.post_status = 0x00000001U;

    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(mon_hw2::payload_length));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_hw2::matches(raw));

    mon_hw2 decoded{};
    CASTLE_SAMPLE_CHECK(mon_hw2::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.ofs_i == -3);
    CASTLE_SAMPLE_CHECK(decoded.mag_i == 140U);
    CASTLE_SAMPLE_CHECK(decoded.ofs_q == 2);
    CASTLE_SAMPLE_CHECK(decoded.mag_q == 138U);
    CASTLE_SAMPLE_CHECK(decoded.cfg_source == mon_hw2_cfg_source::otp);
    CASTLE_SAMPLE_CHECK(decoded.cfg_source != mon_hw2_cfg_source::rom);
    CASTLE_SAMPLE_CHECK(decoded.low_lev_cfg == 0U);
    CASTLE_SAMPLE_CHECK(decoded.post_status == 0x00000001U);

    CASTLE_SAMPLE_CHECK(encode_message(status, frame, frame_size(27U), written) == castle::status::full);
    return 0;
}
