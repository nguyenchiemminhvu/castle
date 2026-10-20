#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/tim_tp.hpp"

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

    // Next pulse at TOW 345600000 ms + 0.5 ms, UTC(USNO) time base, RAIM active.
    tim_tp pulse{};
    pulse.tow_ms = 345600000U;
    pulse.tow_sub_ms = 0x80000000U;
    pulse.q_err = -1500;
    pulse.week = 2300U;
    pulse.flags = static_cast<uint8_t>(tim_tp::FLAGS_TIME_BASE | tim_tp::FLAGS_UTC
                                       | (static_cast<uint8_t>(tim_tp_raim::active) << 2U));
    pulse.ref_info = static_cast<uint8_t>((static_cast<uint8_t>(nav_timeutc_utc_standard::usno) << 4U)
                                          | static_cast<uint8_t>(tim_tp_time_ref_gnss::unknown));

    uint8_t frame[UBX_FRAME_OVERHEAD + tim_tp::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(pulse, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(tim_tp::matches(raw));

    tim_tp decoded{};
    CASTLE_SAMPLE_CHECK(tim_tp::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.tow_ms == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.week == 2300U);
    CASTLE_SAMPLE_CHECK(decoded.time_base() == tim_tp_time_base::utc);
    CASTLE_SAMPLE_CHECK(decoded.utc_available());
    CASTLE_SAMPLE_CHECK(decoded.raim() == tim_tp_raim::active);
    CASTLE_SAMPLE_CHECK(!decoded.q_err_invalid());
    CASTLE_SAMPLE_CHECK(decoded.utc_standard() == nav_timeutc_utc_standard::usno);
    CASTLE_SAMPLE_CHECK(decoded.time_ref_gnss() == tim_tp_time_ref_gnss::unknown);

    // Scaled helpers.
    CASTLE_SAMPLE_CHECK(near(decoded.tow_sub_ms_ms(), 0.5, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.q_err_ns(), -1.5, 1e-12));

    // A wrong length is rejected.
    message_view shorter = raw;
    shorter.header.payload_length = 15U;
    CASTLE_SAMPLE_CHECK(!tim_tp::decode(shorter, decoded));

    return 0;
}
