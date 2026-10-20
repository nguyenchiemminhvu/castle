#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_timegal.hpp"

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

    // Galileo time: galTow 345618 s + 0.125 s, week 1333, GST-UTC 18 s.
    nav_timegal value{};
    value.i_tow = 345600000U;
    value.tow = 345618U;
    value.f_tow = 125000000;
    value.week = 1333;
    value.leap_s = 18;
    value.valid = static_cast<uint8_t>(nav_timegal::VALID_GAL_TOW | nav_timegal::VALID_GAL_WNO | nav_timegal::VALID_LEAP_S);
    value.t_acc = 20U;

    // Encode a complete UBX frame (header + payload + checksum) in place.
    uint8_t frame[UBX_FRAME_OVERHEAD + nav_timegal::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(value, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    // Validate the frame and identify it.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_timegal::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload_length() == nav_timegal::payload_length);

    // Decode the payload back into a typed model.
    nav_timegal decoded{};
    CASTLE_SAMPLE_CHECK(nav_timegal::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.tow == 345618U);
    CASTLE_SAMPLE_CHECK(decoded.f_tow == 125000000);
    CASTLE_SAMPLE_CHECK(decoded.week == 1333);
    CASTLE_SAMPLE_CHECK(decoded.leap_s == 18);
    CASTLE_SAMPLE_CHECK(decoded.t_acc == 20U);

    // Spec-named accessors read the shared generic fields.
    CASTLE_SAMPLE_CHECK(decoded.gal_tow() == decoded.tow);
    CASTLE_SAMPLE_CHECK(decoded.f_gal_tow() == decoded.f_tow);
    CASTLE_SAMPLE_CHECK(decoded.gal_wno() == decoded.week);

    // Validity flags, under both the spec and the generic names.
    CASTLE_SAMPLE_CHECK(decoded.gal_tow_valid());
    CASTLE_SAMPLE_CHECK(decoded.gal_wno_valid());
    CASTLE_SAMPLE_CHECK(decoded.tow_valid());
    CASTLE_SAMPLE_CHECK(decoded.week_valid());
    CASTLE_SAMPLE_CHECK(decoded.leap_s_valid());

    // Precise time of week = tow + f_tow * 1e-9.
    CASTLE_SAMPLE_CHECK(near(decoded.time_of_week_s(), 345618.0 + (125000000 * 1e-9), 1e-9));

    decoded.valid = 0U;
    CASTLE_SAMPLE_CHECK(!decoded.tow_valid() && !decoded.week_valid() && !decoded.leap_s_valid());

    // A different message is not NAV-TIMEGAL; a too-small buffer is reported as full.
    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_TIMEQZSS;
    CASTLE_SAMPLE_CHECK(!nav_timegal::matches(other));
    CASTLE_SAMPLE_CHECK(!nav_timegal::decode(other, decoded));
    CASTLE_SAMPLE_CHECK(encode_message(value, frame, sizeof(frame) - 1U, written) == castle::status::full);

    return 0;
}
