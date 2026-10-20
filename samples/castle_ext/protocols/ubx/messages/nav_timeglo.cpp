#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_timeglo.hpp"

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

    // GLONASS time: 12:00:00.25 on day 1024 of the 2024..2027 interval (N4 = 8).
    nav_timeglo value{};
    value.i_tow = 345600000U;
    value.tod = 43200U;
    value.f_tod = 250000000;
    value.nt = 1024U;
    value.n4 = 8U;
    value.valid = static_cast<uint8_t>(nav_timeglo::VALID_TOD | nav_timeglo::VALID_DATE);
    value.t_acc = 30U;

    // Encode a complete UBX frame (header + payload + checksum) in place.
    uint8_t frame[UBX_FRAME_OVERHEAD + nav_timeglo::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(value, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    // Validate the frame and identify it.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_timeglo::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload_length() == nav_timeglo::payload_length);

    // Decode the payload back into a typed model.
    nav_timeglo decoded{};
    CASTLE_SAMPLE_CHECK(nav_timeglo::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.tod == 43200U);
    CASTLE_SAMPLE_CHECK(decoded.f_tod == 250000000);
    CASTLE_SAMPLE_CHECK(decoded.nt == 1024U);
    CASTLE_SAMPLE_CHECK(decoded.n4 == 8U);
    CASTLE_SAMPLE_CHECK(decoded.t_acc == 30U);

    // Validity flags.
    CASTLE_SAMPLE_CHECK(decoded.tod_valid());
    CASTLE_SAMPLE_CHECK(decoded.date_valid());
    decoded.valid = nav_timeglo::VALID_TOD;
    CASTLE_SAMPLE_CHECK(decoded.tod_valid() && !decoded.date_valid());

    // Precise time of day = tod + f_tod * 1e-9.
    CASTLE_SAMPLE_CHECK(near(decoded.time_of_day_s(), 43200.25, 1e-9));

    // A different message is not NAV-TIMEGLO; a too-small buffer is reported as full.
    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_TIMEGPS;
    CASTLE_SAMPLE_CHECK(!nav_timeglo::matches(other));
    CASTLE_SAMPLE_CHECK(!nav_timeglo::decode(other, decoded));
    CASTLE_SAMPLE_CHECK(encode_message(value, frame, sizeof(frame) - 1U, written) == castle::status::full);

    return 0;
}
