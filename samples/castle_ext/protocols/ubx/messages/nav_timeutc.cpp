#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_timeutc.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // UTC 2026-10-20 23:59:60.5 (a positive leap second), steered to USNO.
    nav_timeutc value{};
    value.i_tow = 345618000U;
    value.t_acc = 25U;
    value.nano = 500000000;
    value.year = 2026U;
    value.month = 10U;
    value.day = 20U;
    value.hour = 23U;
    value.min = 59U;
    value.sec = 60U;
    value.valid = static_cast<uint8_t>(nav_timeutc::VALID_TOW | nav_timeutc::VALID_WKN | nav_timeutc::VALID_UTC |
                                       (static_cast<uint8_t>(nav_timeutc_utc_standard::usno)
                                        << nav_timeutc::VALID_UTC_STANDARD_SHIFT));

    // Encode a complete UBX frame (header + payload + checksum) in place.
    uint8_t frame[UBX_FRAME_OVERHEAD + nav_timeutc::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(value, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    // Validate the frame and identify it.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_timeutc::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload_length() == nav_timeutc::payload_length);

    // Decode the payload back into a typed model.
    nav_timeutc decoded{};
    CASTLE_SAMPLE_CHECK(nav_timeutc::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345618000U);
    CASTLE_SAMPLE_CHECK(decoded.t_acc == 25U);
    CASTLE_SAMPLE_CHECK(decoded.nano == 500000000);
    CASTLE_SAMPLE_CHECK(decoded.year == 2026U);
    CASTLE_SAMPLE_CHECK(decoded.month == 10U);
    CASTLE_SAMPLE_CHECK(decoded.day == 20U);
    CASTLE_SAMPLE_CHECK(decoded.hour == 23U);
    CASTLE_SAMPLE_CHECK(decoded.min == 59U);
    CASTLE_SAMPLE_CHECK(decoded.sec == 60U);

    // Validity flags and the UTC standard in bits 7..4.
    CASTLE_SAMPLE_CHECK(decoded.valid_tow());
    CASTLE_SAMPLE_CHECK(decoded.valid_wkn());
    CASTLE_SAMPLE_CHECK(decoded.valid_utc());
    CASTLE_SAMPLE_CHECK(decoded.utc_standard() == nav_timeutc_utc_standard::usno);
    decoded.valid = nav_timeutc::VALID_TOW;
    CASTLE_SAMPLE_CHECK(!decoded.valid_utc());
    CASTLE_SAMPLE_CHECK(decoded.utc_standard() == nav_timeutc_utc_standard::not_available);

    // A different message is not NAV-TIMEUTC; a too-small buffer is reported as full.
    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_CLOCK;
    CASTLE_SAMPLE_CHECK(!nav_timeutc::matches(other));
    CASTLE_SAMPLE_CHECK(!nav_timeutc::decode(other, decoded));
    CASTLE_SAMPLE_CHECK(encode_message(value, frame, sizeof(frame) - 1U, written) == castle::status::full);

    return 0;
}
