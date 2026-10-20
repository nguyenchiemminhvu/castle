#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_timeqzss.hpp"

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

    // QZSS time: qzssTow 345618 s - 0.5 s, week 2437, QZSST-UTC 18 s.
    nav_timeqzss value{};
    value.i_tow = 345600000U;
    value.tow = 345618U;
    value.f_tow = -500000000;
    value.week = 2437;
    value.leap_s = 18;
    value.valid = static_cast<uint8_t>(nav_timeqzss::VALID_QZSS_TOW | nav_timeqzss::VALID_QZSS_WNO | nav_timeqzss::VALID_LEAP_S);
    value.t_acc = 20U;

    // Encode a complete UBX frame (header + payload + checksum) in place.
    uint8_t frame[UBX_FRAME_OVERHEAD + nav_timeqzss::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(value, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    // Validate the frame and identify it.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_timeqzss::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload_length() == nav_timeqzss::payload_length);

    // Decode the payload back into a typed model.
    nav_timeqzss decoded{};
    CASTLE_SAMPLE_CHECK(nav_timeqzss::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.tow == 345618U);
    CASTLE_SAMPLE_CHECK(decoded.f_tow == -500000000);
    CASTLE_SAMPLE_CHECK(decoded.week == 2437);
    CASTLE_SAMPLE_CHECK(decoded.leap_s == 18);
    CASTLE_SAMPLE_CHECK(decoded.t_acc == 20U);

    // Spec-named accessors read the shared generic fields.
    CASTLE_SAMPLE_CHECK(decoded.qzss_tow() == decoded.tow);
    CASTLE_SAMPLE_CHECK(decoded.f_qzss_tow() == decoded.f_tow);
    CASTLE_SAMPLE_CHECK(decoded.qzss_wno() == decoded.week);

    // Validity flags, under both the spec and the generic names.
    CASTLE_SAMPLE_CHECK(decoded.qzss_tow_valid());
    CASTLE_SAMPLE_CHECK(decoded.qzss_wno_valid());
    CASTLE_SAMPLE_CHECK(decoded.tow_valid());
    CASTLE_SAMPLE_CHECK(decoded.week_valid());
    CASTLE_SAMPLE_CHECK(decoded.leap_s_valid());

    // Precise time of week = tow + f_tow * 1e-9.
    CASTLE_SAMPLE_CHECK(near(decoded.time_of_week_s(), 345618.0 + (-500000000 * 1e-9), 1e-9));

    decoded.valid = 0U;
    CASTLE_SAMPLE_CHECK(!decoded.tow_valid() && !decoded.week_valid() && !decoded.leap_s_valid());

    // A different message is not NAV-TIMEQZSS; a too-small buffer is reported as full.
    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_TIMEBDS;
    CASTLE_SAMPLE_CHECK(!nav_timeqzss::matches(other));
    CASTLE_SAMPLE_CHECK(!nav_timeqzss::decode(other, decoded));
    CASTLE_SAMPLE_CHECK(encode_message(value, frame, sizeof(frame) - 1U, written) == castle::status::full);

    return 0;
}
