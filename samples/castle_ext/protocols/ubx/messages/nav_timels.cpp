#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_timels.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // 18 s current leap seconds (from GPS); a +1 s event announced by Galileo in one day.
    nav_timels value{};
    value.i_tow = 345600000U;
    value.version = 0U;
    value.src_of_curr_ls = nav_timels_src_of_curr_ls::gps;
    value.curr_ls = 18;
    value.src_of_ls_change = nav_timels_src_of_ls_change::galileo;
    value.ls_change = 1;
    value.time_to_ls_event = 86400;
    value.date_of_ls_gps_wn = 2437U;
    value.date_of_ls_gps_dn = 1U;
    value.valid = static_cast<uint8_t>(nav_timels::VALID_CURR_LS | nav_timels::VALID_TIME_TO_LS_EVENT);

    // Encode a complete UBX frame (header + payload + checksum) in place.
    uint8_t frame[UBX_FRAME_OVERHEAD + nav_timels::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(value, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    // Validate the frame and identify it.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_timels::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload_length() == nav_timels::payload_length);

    // Decode the payload back into a typed model.
    nav_timels decoded{};
    CASTLE_SAMPLE_CHECK(nav_timels::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.version == 0U);
    CASTLE_SAMPLE_CHECK(decoded.src_of_curr_ls == nav_timels_src_of_curr_ls::gps);
    CASTLE_SAMPLE_CHECK(decoded.curr_ls == 18);
    CASTLE_SAMPLE_CHECK(decoded.src_of_ls_change == nav_timels_src_of_ls_change::galileo);
    CASTLE_SAMPLE_CHECK(decoded.ls_change == 1);
    CASTLE_SAMPLE_CHECK(decoded.time_to_ls_event == 86400);
    CASTLE_SAMPLE_CHECK(decoded.date_of_ls_gps_wn == 2437U);
    CASTLE_SAMPLE_CHECK(decoded.date_of_ls_gps_dn == 1U);

    // Validity flags.
    CASTLE_SAMPLE_CHECK(decoded.valid_curr_ls());
    CASTLE_SAMPLE_CHECK(decoded.valid_time_to_ls_event());
    decoded.valid = nav_timels::VALID_CURR_LS;
    CASTLE_SAMPLE_CHECK(decoded.valid_curr_ls() && !decoded.valid_time_to_ls_event());

    // Source enums carry the spec values.
    CASTLE_SAMPLE_CHECK(static_cast<uint8_t>(nav_timels_src_of_curr_ls::unknown) == 255U);
    CASTLE_SAMPLE_CHECK(static_cast<uint8_t>(nav_timels_src_of_ls_change::glonass) == 6U);

    // A different message is not NAV-TIMELS; a too-small buffer is reported as full.
    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_TIMEUTC;
    CASTLE_SAMPLE_CHECK(!nav_timels::matches(other));
    CASTLE_SAMPLE_CHECK(!nav_timels::decode(other, decoded));
    CASTLE_SAMPLE_CHECK(encode_message(value, frame, sizeof(frame) - 1U, written) == castle::status::full);

    return 0;
}
