#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/esf_alg.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Fine IMU-mount alignment running, yaw alignment error flagged.
    esf_alg alignment{};
    alignment.i_tow = 345600U;
    alignment.version = 0x01U;
    alignment.flags = static_cast<uint8_t>(esf_alg::FLAGS_AUTO_MNT_ALG_ON
                                           | (static_cast<uint8_t>(esf_alg_status::fine) << 1U));
    alignment.error = esf_alg::ERROR_YAW_ALG_ERROR;
    alignment.yaw = 18050U; // 180.50 deg
    alignment.pitch = -125; // -1.25 deg
    alignment.roll = 250;   // 2.50 deg

    uint8_t frame[32U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(alignment, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(esf_alg::payload_length));

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(esf_alg::matches(raw));

    esf_alg decoded;
    CASTLE_SAMPLE_CHECK(esf_alg::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600U);
    CASTLE_SAMPLE_CHECK(decoded.version == 0x01U);
    CASTLE_SAMPLE_CHECK(decoded.auto_mnt_alg_on());
    CASTLE_SAMPLE_CHECK(decoded.status() == esf_alg_status::fine);
    CASTLE_SAMPLE_CHECK(!decoded.tilt_alg_error());
    CASTLE_SAMPLE_CHECK(decoded.yaw_alg_error());
    CASTLE_SAMPLE_CHECK(!decoded.angle_error());
    CASTLE_SAMPLE_CHECK((decoded.yaw_deg() > 180.49) && (decoded.yaw_deg() < 180.51));
    CASTLE_SAMPLE_CHECK((decoded.pitch_deg() > -1.26) && (decoded.pitch_deg() < -1.24));
    CASTLE_SAMPLE_CHECK((decoded.roll_deg() > 2.49) && (decoded.roll_deg() < 2.51));

    // UBX-ESF-ALG is periodic/polled: an empty-payload poll request asks for one.
    CASTLE_SAMPLE_CHECK(encode_poll<esf_alg>(frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(0U));

    return 0;
}
