#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_hw.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    mon_hw status{};
    status.pin_sel = 0x0001EFFFU;
    status.noise_per_ms = 82U;
    status.agc_cnt = 5600U;
    status.a_status = mon_antenna_status::ok;
    status.a_power = mon_antenna_power::on;
    status.flags = static_cast<uint8_t>(mon_hw::FLAGS_RTC_CALIB | (2U << 2U)); // jammingState = warning
    status.vp[0U] = 0x0AU;
    status.vp[mon_hw::VP_COUNT - 1U] = 0x1BU;
    status.jam_ind = 12U;

    uint8_t frame[96U] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(mon_hw::payload_length));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_hw::matches(raw));

    mon_hw decoded{};
    CASTLE_SAMPLE_CHECK(mon_hw::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.pin_sel == 0x0001EFFFU);
    CASTLE_SAMPLE_CHECK(decoded.noise_per_ms == 82U);
    CASTLE_SAMPLE_CHECK(decoded.agc_cnt == 5600U);
    CASTLE_SAMPLE_CHECK(decoded.a_status == mon_antenna_status::ok);
    CASTLE_SAMPLE_CHECK(decoded.a_power == mon_antenna_power::on);
    CASTLE_SAMPLE_CHECK(decoded.rtc_calib());
    CASTLE_SAMPLE_CHECK(!decoded.safe_boot());
    CASTLE_SAMPLE_CHECK(!decoded.xtal_absent());
    CASTLE_SAMPLE_CHECK(decoded.jamming_state() == mon_jamming_state::warning);
    CASTLE_SAMPLE_CHECK(decoded.vp[0U] == 0x0AU);
    CASTLE_SAMPLE_CHECK(decoded.vp[mon_hw::VP_COUNT - 1U] == 0x1BU);
    CASTLE_SAMPLE_CHECK(decoded.jam_ind == 12U);

    CASTLE_SAMPLE_CHECK(encode_message(status, frame, frame_size(59U), written) == castle::status::full);
    return 0;
}
