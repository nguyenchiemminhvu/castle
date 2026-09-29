#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_eell.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a NAV-EELL payload.
    uint8_t payload[nav_eell::payload_length] = {};

    castle::write_le32(&payload[0U], 123456U); // iTOW
    payload[4U] = 1U;                  // version
    // bytes [5..7] reserved

    castle::write_le16(&payload[8U],  500U);  // errMaj
    castle::write_le16(&payload[10U], 250U);  // errMin
    castle::write_le16(&payload[12U], 900U);  // errOrient
    // bytes [14..15] reserved

    // Encode a UBX-NAV-EELL frame.
    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        nav_eell::msg_class,
        nav_eell::msg_id,
        castle::container::array_view<const uint8_t>(
            payload,
            nav_eell::payload_length),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // Decode the UBX frame.
    message_view raw;
    bool frame_ok = decode_frame(
        castle::container::array_view<const uint8_t>(
            frame,
            written),
        raw);

    CASTLE_SAMPLE_CHECK(frame_ok);

    // Verify message identification helpers.
    CASTLE_SAMPLE_CHECK(nav_eell::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_NAV, UBX_ID_NAV_EELL));

    // Decode the NAV-EELL payload.
    nav_eell ellipse;
    bool decoded = nav_eell::decode(raw, ellipse);

    CASTLE_SAMPLE_CHECK(decoded);

    // Verify decoded values.
    CASTLE_SAMPLE_CHECK(ellipse.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(ellipse.version == 1U);
    CASTLE_SAMPLE_CHECK(ellipse.err_maj == 500U);
    CASTLE_SAMPLE_CHECK(ellipse.err_min == 250U);
    CASTLE_SAMPLE_CHECK(ellipse.err_orient == 900U);

    // Different message types do not match NAV-EELL.
    message_view different{};
    different.header.msg_class = UBX_CLASS_CFG;
    different.header.msg_id = UBX_ID_CFG_VALSET;

    CASTLE_SAMPLE_CHECK(!nav_eell::matches(different));

    return 0;
}
