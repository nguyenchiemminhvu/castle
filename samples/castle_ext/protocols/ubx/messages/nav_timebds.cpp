#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_timebds.hpp"

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

    // BeiDou time: SOW 345586 s - 0.25 s, BDS week 1077, BDS-UTC 4 s.
    nav_timebds value{};
    value.i_tow = 345600000U;
    value.tow = 345586U;
    value.f_tow = -250000000;
    value.week = 1077;
    value.leap_s = 4;
    value.valid = static_cast<uint8_t>(nav_timebds::VALID_SOW | nav_timebds::VALID_WEEK | nav_timebds::VALID_LEAP_S);
    value.t_acc = 20U;

    // Encode a complete UBX frame (header + payload + checksum) in place.
    uint8_t frame[UBX_FRAME_OVERHEAD + nav_timebds::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(value, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    // Validate the frame and identify it.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_timebds::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload_length() == nav_timebds::payload_length);

    // Decode the payload back into a typed model.
    nav_timebds decoded{};
    CASTLE_SAMPLE_CHECK(nav_timebds::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.tow == 345586U);
    CASTLE_SAMPLE_CHECK(decoded.f_tow == -250000000);
    CASTLE_SAMPLE_CHECK(decoded.week == 1077);
    CASTLE_SAMPLE_CHECK(decoded.leap_s == 4);
    CASTLE_SAMPLE_CHECK(decoded.t_acc == 20U);

    // Spec-named accessors read the shared generic fields.
    CASTLE_SAMPLE_CHECK(decoded.sow() == decoded.tow);
    CASTLE_SAMPLE_CHECK(decoded.f_sow() == decoded.f_tow);

    // Validity flags, under both the spec and the generic names.
    CASTLE_SAMPLE_CHECK(decoded.sow_valid());
    CASTLE_SAMPLE_CHECK(decoded.tow_valid());
    CASTLE_SAMPLE_CHECK(decoded.week_valid());
    CASTLE_SAMPLE_CHECK(decoded.leap_s_valid());

    // Precise time of week = tow + f_tow * 1e-9.
    CASTLE_SAMPLE_CHECK(near(decoded.time_of_week_s(), 345586.0 + (-250000000 * 1e-9), 1e-9));

    decoded.valid = 0U;
    CASTLE_SAMPLE_CHECK(!decoded.tow_valid() && !decoded.week_valid() && !decoded.leap_s_valid());

    // A different message is not NAV-TIMEBDS; a too-small buffer is reported as full.
    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_TIMEGAL;
    CASTLE_SAMPLE_CHECK(!nav_timebds::matches(other));
    CASTLE_SAMPLE_CHECK(!nav_timebds::decode(other, decoded));
    CASTLE_SAMPLE_CHECK(encode_message(value, frame, sizeof(frame) - 1U, written) == castle::status::full);

    return 0;
}
