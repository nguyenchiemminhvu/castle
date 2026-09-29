#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav2_pvt.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    uint8_t payload_data[nav2_pvt::payload_length] = {};

    write_le32(&payload_data[0U], 1000U);            // iTOW
    write_le16(&payload_data[4U], 2026U);            // year
    payload_data[6U] = 10U;                          // month
    payload_data[7U] = 2U;                           // day
    payload_data[8U] = 12U;                          // hour
    payload_data[9U] = 34U;                          // min
    payload_data[10U] = 56U;                         // sec
    payload_data[11U] = nav2_pvt::VALID_DATE
                       | nav2_pvt::VALID_TIME;       // valid

    write_le32(&payload_data[12U], 50U);             // tAcc
    write_le32(&payload_data[16U], 100U);            // nano

    payload_data[20U] = static_cast<uint8_t>(nav2_pvt_fix_type::fix_3d);
    payload_data[21U] = nav2_pvt::FLAGS_GNSS_FIX_OK;
    payload_data[22U] = 0U;                          // flags2
    payload_data[23U] = 12U;                         // numSV

    write_le32(&payload_data[24U], 123456789U);      // lon
    write_le32(&payload_data[28U], 987654321U);      // lat
    write_le32(&payload_data[32U], 100U);            // height
    write_le32(&payload_data[36U], 80U);             // hMSL

    write_le32(&payload_data[40U], 5U);              // hAcc
    write_le32(&payload_data[44U], 7U);              // vAcc

    write_le32(&payload_data[48U], 10U);             // velN
    write_le32(&payload_data[52U], 20U);             // velE
    write_le32(&payload_data[56U], 30U);             // velD
    write_le32(&payload_data[60U], 40U);             // gSpeed
    write_le32(&payload_data[64U], 50U);             // headMot

    write_le32(&payload_data[68U], 2U);              // sAcc
    write_le32(&payload_data[72U], 3U);              // headAcc

    write_le16(&payload_data[76U], 150U);            // pDOP

    payload_data[78U] = 0U;                          // flags3
    // bytes 79..83 reserved

    write_le32(&payload_data[84U], 60U);             // headVeh
    write_le16(&payload_data[88U], 15U);             // magDec
    write_le16(&payload_data[90U], 5U);              // magAcc

    message_view raw;
    raw.header.msg_class = UBX_CLASS_NAV2;
    raw.header.msg_id = UBX_ID_NAV2_PVT;
    raw.header.payload_length = nav2_pvt::payload_length;
    raw.payload =
        castle::container::array_view<const uint8_t>(
            payload_data,
            sizeof(payload_data));

    CASTLE_SAMPLE_CHECK(nav2_pvt::matches(raw));

    nav2_pvt message;
    bool decoded = nav2_pvt::decode(raw, message);

    CASTLE_SAMPLE_CHECK(decoded);

    CASTLE_SAMPLE_CHECK(message.i_tow == 1000U);
    CASTLE_SAMPLE_CHECK(message.year == 2026U);
    CASTLE_SAMPLE_CHECK(message.month == 10U);
    CASTLE_SAMPLE_CHECK(message.day == 2U);
    CASTLE_SAMPLE_CHECK(message.hour == 12U);
    CASTLE_SAMPLE_CHECK(message.min == 34U);
    CASTLE_SAMPLE_CHECK(message.sec == 56U);

    CASTLE_SAMPLE_CHECK(
        message.fix_type == nav2_pvt_fix_type::fix_3d);

    CASTLE_SAMPLE_CHECK(
        (message.flags & nav2_pvt::FLAGS_GNSS_FIX_OK) != 0U);

    CASTLE_SAMPLE_CHECK(message.num_sv == 12U);

    CASTLE_SAMPLE_CHECK(message.lon == 123456789);
    CASTLE_SAMPLE_CHECK(message.lat == 987654321);
    CASTLE_SAMPLE_CHECK(message.height == 100);
    CASTLE_SAMPLE_CHECK(message.h_msl == 80);

    CASTLE_SAMPLE_CHECK(message.h_acc == 5U);
    CASTLE_SAMPLE_CHECK(message.v_acc == 7U);

    CASTLE_SAMPLE_CHECK(message.vel_n == 10);
    CASTLE_SAMPLE_CHECK(message.vel_e == 20);
    CASTLE_SAMPLE_CHECK(message.vel_d == 30);
    CASTLE_SAMPLE_CHECK(message.g_speed == 40);
    CASTLE_SAMPLE_CHECK(message.head_mot == 50);

    CASTLE_SAMPLE_CHECK(message.s_acc == 2U);
    CASTLE_SAMPLE_CHECK(message.head_acc == 3U);
    CASTLE_SAMPLE_CHECK(message.p_dop == 150U);

    CASTLE_SAMPLE_CHECK(message.head_veh == 60);
    CASTLE_SAMPLE_CHECK(message.mag_dec == 15);
    CASTLE_SAMPLE_CHECK(message.mag_acc == 5U);

    // Wrong payload length must be rejected.
    message_view invalid = raw;
    invalid.header.payload_length = nav2_pvt::payload_length - 1U;

    nav2_pvt rejected;
    CASTLE_SAMPLE_CHECK(!nav2_pvt::decode(invalid, rejected));

    return 0;
}
