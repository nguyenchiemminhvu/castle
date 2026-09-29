#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_pvt.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    // Build a synthetic UBX-NAV-PVT payload.
    uint8_t payload[nav_pvt::payload_length] = {};

    write_le32(&payload[0U], 123456U);                  // iTOW
    write_le16(&payload[4U], 2026U);                    // year
    payload[6U] = 10U;                                  // month
    payload[7U] = 2U;                                   // day
    payload[8U] = 12U;                                  // hour
    payload[9U] = 34U;                                  // minute
    payload[10U] = 56U;                                 // second
    payload[11U] = nav_pvt::VALID_DATE | nav_pvt::VALID_TIME;

    payload[20U] = static_cast<uint8_t>(nav_pvt_fix_type::fix_3d);
    payload[21U] = nav_pvt::FLAGS_GNSS_FIX_OK;
    payload[23U] = 18U;                                 // numSV

    write_le32(&payload[24U], static_cast<uint32_t>(139876543)); // lon
    write_le32(&payload[28U], static_cast<uint32_t>(358765432)); // lat

    write_le32(&payload[32U], 12345U);                  // height (mm)
    write_le32(&payload[76U], 145U);                    // pDOP = 1.45

    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        nav_pvt::msg_class,
        nav_pvt::msg_id,
        castle::container::array_view<const uint8_t>(
            payload,
            nav_pvt::payload_length),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // Decode the UBX frame.
    message_view raw;
    bool ok = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(ok);
    CASTLE_SAMPLE_CHECK(nav_pvt::matches(raw));

    // Decode the NAV-PVT message.
    nav_pvt pvt;
    CASTLE_SAMPLE_CHECK(nav_pvt::decode(raw, pvt));

    CASTLE_SAMPLE_CHECK(pvt.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(pvt.year == 2026U);
    CASTLE_SAMPLE_CHECK(pvt.month == 10U);
    CASTLE_SAMPLE_CHECK(pvt.day == 2U);

    CASTLE_SAMPLE_CHECK(pvt.fix_type == nav_pvt_fix_type::fix_3d);
    CASTLE_SAMPLE_CHECK(pvt.fix_ok());
    CASTLE_SAMPLE_CHECK(pvt.num_sv == 18U);

    CASTLE_SAMPLE_CHECK(pvt.lon == 139876543);
    CASTLE_SAMPLE_CHECK(pvt.lat == 358765432);

    // Convenience conversions.
    CASTLE_SAMPLE_CHECK(pvt.longitude_deg() > 13.9);
    CASTLE_SAMPLE_CHECK(pvt.latitude_deg() > 35.8);
    CASTLE_SAMPLE_CHECK(pvt.height_m() == 12.345);
    CASTLE_SAMPLE_CHECK(pvt.pdop_value() == 1.45);

    // Wrong message type does not match NAV-PVT.
    raw.header.msg_id = UBX_ID_NAV_STATUS;
    CASTLE_SAMPLE_CHECK(!nav_pvt::matches(raw));

    return 0;
}
