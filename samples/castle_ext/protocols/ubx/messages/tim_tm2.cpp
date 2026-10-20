#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/tim_tm2.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // A running time mark on EXTINT0 with a new rising edge, GNSS time base, valid time.
    tim_tm2 mark{};
    mark.ch = 0U;
    mark.flags = static_cast<uint8_t>(tim_tm2::FLAGS_MODE | tim_tm2::FLAGS_NEW_RISING_EDGE | tim_tm2::FLAGS_TIME
                                      | (static_cast<uint8_t>(tim_tm2_time_base::gnss) << 3U));
    mark.count = 17U;
    mark.wn_r = 2300U;
    mark.wn_f = 2300U;
    mark.tow_ms_r = 345600000U;
    mark.tow_sub_ms_r = 125000U;
    mark.tow_ms_f = 345600010U;
    mark.tow_sub_ms_f = 250000U;
    mark.acc_est = 20U;

    uint8_t frame[UBX_FRAME_OVERHEAD + tim_tm2::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(mark, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(tim_tm2::matches(raw));

    tim_tm2 decoded{};
    CASTLE_SAMPLE_CHECK(tim_tm2::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.count == 17U);
    CASTLE_SAMPLE_CHECK(decoded.tow_ms_r == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.tow_sub_ms_f == 250000U);
    CASTLE_SAMPLE_CHECK(decoded.acc_est == 20U);
    CASTLE_SAMPLE_CHECK(decoded.running());
    CASTLE_SAMPLE_CHECK(!decoded.stopped());
    CASTLE_SAMPLE_CHECK(decoded.new_rising_edge());
    CASTLE_SAMPLE_CHECK(!decoded.new_falling_edge());
    CASTLE_SAMPLE_CHECK(decoded.time_base() == tim_tm2_time_base::gnss);
    CASTLE_SAMPLE_CHECK(!decoded.utc_available());
    CASTLE_SAMPLE_CHECK(decoded.time_valid());

    // Poll the latest time mark (empty payload).
    uint8_t poll[UBX_FRAME_OVERHEAD];
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_poll<tim_tm2>(poll, sizeof(poll), written)));

    return 0;
}
