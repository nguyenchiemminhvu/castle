#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_dop.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a NAV-DOP payload.
    uint8_t payload[nav_dop::payload_length] = {};

    castle::write_le32(&payload[0U], 123456U); // iTOW

    castle::write_le16(&payload[4U], 100U);  // gDOP
    castle::write_le16(&payload[6U], 110U);  // pDOP
    castle::write_le16(&payload[8U], 120U);  // tDOP
    castle::write_le16(&payload[10U], 130U); // vDOP
    castle::write_le16(&payload[12U], 140U); // hDOP
    castle::write_le16(&payload[14U], 150U); // nDOP
    castle::write_le16(&payload[16U], 160U); // eDOP

    // Encode a UBX-NAV-DOP frame.
    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        nav_dop::msg_class,
        nav_dop::msg_id,
        castle::container::array_view<const uint8_t>(
            payload,
            nav_dop::payload_length),
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
    CASTLE_SAMPLE_CHECK(nav_dop::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_NAV, UBX_ID_NAV_DOP));

    // Decode the NAV-DOP payload.
    nav_dop dop;
    bool decoded = nav_dop::decode(raw, dop);

    CASTLE_SAMPLE_CHECK(decoded);

    // Verify decoded values.
    CASTLE_SAMPLE_CHECK(dop.i_tow == 123456U);

    CASTLE_SAMPLE_CHECK(dop.g_dop == 100U);
    CASTLE_SAMPLE_CHECK(dop.p_dop == 110U);
    CASTLE_SAMPLE_CHECK(dop.t_dop == 120U);
    CASTLE_SAMPLE_CHECK(dop.v_dop == 130U);
    CASTLE_SAMPLE_CHECK(dop.h_dop == 140U);
    CASTLE_SAMPLE_CHECK(dop.n_dop == 150U);
    CASTLE_SAMPLE_CHECK(dop.e_dop == 160U);

    // Different message types do not match NAV-DOP.
    message_view different{};
    different.header.msg_class = UBX_CLASS_CFG;
    different.header.msg_id = UBX_ID_CFG_VALSET;

    CASTLE_SAMPLE_CHECK(!nav_dop::matches(different));

    return 0;
}
