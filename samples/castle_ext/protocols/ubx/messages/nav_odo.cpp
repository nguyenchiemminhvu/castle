#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_odo.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    // Build a NAV-ODO payload.
    uint8_t payload[nav_odo::payload_length] = {};

    payload[0U] = 1U; // version
    // bytes [1..3] reserved

    write_le32(&payload[4U], 123456U); // iTOW
    write_le32(&payload[8U], 1500U);   // distance
    write_le32(&payload[12U], 25000U); // totalDistance
    write_le32(&payload[16U], 15U);    // distanceStd

    // Encode a UBX-NAV-ODO frame.
    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        nav_odo::msg_class,
        nav_odo::msg_id,
        castle::container::array_view<const uint8_t>(
            payload,
            nav_odo::payload_length),
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
    CASTLE_SAMPLE_CHECK(nav_odo::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_NAV, UBX_ID_NAV_ODO));

    // Decode the NAV-ODO payload.
    nav_odo odo;
    bool decoded = nav_odo::decode(raw, odo);

    CASTLE_SAMPLE_CHECK(decoded);

    // Verify decoded values.
    CASTLE_SAMPLE_CHECK(odo.version == 1U);
    CASTLE_SAMPLE_CHECK(odo.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(odo.distance == 1500U);
    CASTLE_SAMPLE_CHECK(odo.total_distance == 25000U);
    CASTLE_SAMPLE_CHECK(odo.distance_std == 15U);

    // Different message types do not match NAV-ODO.
    message_view different{};
    different.header.msg_class = UBX_CLASS_CFG;
    different.header.msg_id = UBX_ID_CFG_VALSET;

    CASTLE_SAMPLE_CHECK(!nav_odo::matches(different));

    return 0;
}
