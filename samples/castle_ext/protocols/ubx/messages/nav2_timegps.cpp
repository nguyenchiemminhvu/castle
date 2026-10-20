#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav2_timegps.hpp"

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

    // GPS time: week 2437, iTOW 345600.000 s - 250 ns, GPS-UTC 18 s.
    nav2_timegps value{};
    value.i_tow = 345600000U;
    value.f_tow = -250;
    value.week = 2437;
    value.leap_s = 18;
    value.valid = static_cast<uint8_t>(nav2_timegps::VALID_TOW | nav2_timegps::VALID_WEEK | nav2_timegps::VALID_LEAP_S);
    value.t_acc = 50U;

    // Encode a complete UBX frame (header + payload + checksum) in place.
    uint8_t frame[UBX_FRAME_OVERHEAD + nav2_timegps::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(value, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    // Validate the frame and identify it.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav2_timegps::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload_length() == nav2_timegps::payload_length);

    // Decode the payload back into a typed model.
    nav2_timegps decoded{};
    CASTLE_SAMPLE_CHECK(nav2_timegps::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(raw.msg_class() == UBX_CLASS_NAV2 && raw.msg_id() == UBX_ID_NAV2_TIMEGPS);
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.f_tow == -250);
    CASTLE_SAMPLE_CHECK(decoded.week == 2437);
    CASTLE_SAMPLE_CHECK(decoded.leap_s == 18);
    CASTLE_SAMPLE_CHECK(decoded.t_acc == 50U);

    // Validity flags.
    CASTLE_SAMPLE_CHECK(decoded.tow_valid());
    CASTLE_SAMPLE_CHECK(decoded.week_valid());
    CASTLE_SAMPLE_CHECK(decoded.leap_s_valid());
    decoded.valid = nav2_timegps::VALID_WEEK;
    CASTLE_SAMPLE_CHECK(!decoded.tow_valid() && decoded.week_valid() && !decoded.leap_s_valid());

    // Precise time of week = iTOW * 1e-3 + fTOW * 1e-9.
    CASTLE_SAMPLE_CHECK(near(decoded.time_of_week_s(), 345599.99999975, 1e-9));

    // A different message is not NAV2-TIMEGPS; a too-small buffer is reported as full.
    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV2_PVT;
    CASTLE_SAMPLE_CHECK(!nav2_timegps::matches(other));
    CASTLE_SAMPLE_CHECK(!nav2_timegps::decode(other, decoded));
    CASTLE_SAMPLE_CHECK(encode_message(value, frame, sizeof(frame) - 1U, written) == castle::status::full);

    // Same ID and layout as NAV-TIMEGPS, but the NAV2 class keeps them apart.
    message_view primary = raw;
    primary.header.msg_class = UBX_CLASS_NAV;
    CASTLE_SAMPLE_CHECK(!nav2_timegps::matches(primary));
    CASTLE_SAMPLE_CHECK(nav_timegps::matches(primary));

    return 0;
}
