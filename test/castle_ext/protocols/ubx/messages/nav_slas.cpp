#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_slas.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

nav_slas_correction make_correction(uint8_t seed)
{
    nav_slas_correction correction{};
    correction.gnss_id = static_cast<uint8_t>(seed + 1U);
    correction.sv_id = static_cast<uint8_t>(seed + 2U);
    correction.prc = static_cast<int16_t>(-7000 - seed);
    return correction;
}

void expect_correction_eq(CASTLE_CONST nav_slas_correction& actual, CASTLE_CONST nav_slas_correction& expected)
{
    EXPECT_EQ(actual.gnss_id, expected.gnss_id);
    EXPECT_EQ(actual.sv_id, expected.sv_id);
    EXPECT_EQ(actual.prc, expected.prc);
}

nav_slas<4U> make_message()
{
    nav_slas<4U> message{};
    message.i_tow = 0x2468ACE0U;
    message.version = 0x3CU;
    message.gms_lon = -123456789;
    message.gms_lat = 98765432;
    message.gms_code = 0xC7U;
    message.qzss_sv_id = 199U;
    message.service_flags = 0x05U;
    message.cnt = 222U; // ignored on encode: count comes from corrections.size()
    return message;
}

TEST(UbxMessageNavSlas, RoundTripsEveryField)
{
    nav_slas<4U> message = make_message();
    ASSERT_TRUE(castle::succeeded(message.corrections.push_back(make_correction(0U))));
    ASSERT_TRUE(castle::succeeded(message.corrections.push_back(make_correction(4U))));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD + 20U + (2U * 8U));

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(nav_slas<4U>::matches(raw));

    nav_slas<4U> decoded;
    ASSERT_TRUE(nav_slas<4U>::decode(raw, decoded));
    EXPECT_EQ(decoded.i_tow, 0x2468ACE0U);
    EXPECT_EQ(decoded.version, 0x3CU);
    EXPECT_EQ(decoded.gms_lon, -123456789);
    EXPECT_EQ(decoded.gms_lat, 98765432);
    EXPECT_EQ(decoded.gms_code, 0xC7U);
    EXPECT_EQ(decoded.qzss_sv_id, 199U);
    EXPECT_EQ(decoded.service_flags, 0x05U);
    EXPECT_EQ(decoded.cnt, 2U);
    ASSERT_EQ(decoded.corrections.size(), 2U);
    expect_correction_eq(decoded.corrections[0U], make_correction(0U));
    expect_correction_eq(decoded.corrections[1U], make_correction(4U));
}

TEST(UbxMessageNavSlas, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[36U] = {};
    castle::write_le32(&payload[0U], 0x01234567U);
    payload[4U] = 0U;
    payload[5U] = 0xEEU; // reserved0[3] is ignored
    payload[6U] = 0xEEU;
    payload[7U] = 0xEEU;
    castle::write_le32(&payload[8U], static_cast<uint32_t>(139692));
    castle::write_le32(&payload[12U], static_cast<uint32_t>(-35689));
    payload[16U] = 9U;
    payload[17U] = 194U;
    payload[18U] = 0x06U;
    payload[19U] = 2U;
    payload[20U] = 0U;   // gnssId
    payload[21U] = 11U;  // svId
    payload[22U] = 0xEEU; // reserved1
    payload[23U] = 0xEEU; // reserved2[3]
    payload[24U] = 0xEEU;
    payload[25U] = 0xEEU;
    castle::write_le16(&payload[26U], static_cast<uint16_t>(-321));
    payload[28U] = 5U;
    payload[29U] = 3U;
    castle::write_le16(&payload[34U], 654U);

    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SLAS, payload, 36U);
    nav_slas<2U> out;
    ASSERT_TRUE(nav_slas<2U>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 0x01234567U);
    EXPECT_EQ(out.version, 0U);
    EXPECT_EQ(out.gms_lon, 139692);
    EXPECT_EQ(out.gms_lat, -35689);
    EXPECT_EQ(out.gms_code, 9U);
    EXPECT_EQ(out.qzss_sv_id, 194U);
    EXPECT_EQ(out.service_flags, 0x06U);
    EXPECT_FALSE(out.gms_available());
    EXPECT_TRUE(out.qzss_sv_available());
    EXPECT_TRUE(out.test_mode());
    EXPECT_EQ(out.cnt, 2U);
    ASSERT_EQ(out.corrections.size(), 2U);
    EXPECT_EQ(out.corrections[0U].gnss_id, 0U);
    EXPECT_EQ(out.corrections[0U].sv_id, 11U);
    EXPECT_EQ(out.corrections[0U].prc, -321);
    EXPECT_EQ(out.corrections[1U].gnss_id, 5U);
    EXPECT_EQ(out.corrections[1U].sv_id, 3U);
    EXPECT_EQ(out.corrections[1U].prc, 654);
}

TEST(UbxMessageNavSlas, EncodesSpecOffsetsAndZeroReserved)
{
    nav_slas<4U> message = make_message();
    ASSERT_TRUE(castle::succeeded(message.corrections.push_back(make_correction(0U))));

    uint8_t frame[40U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(castle::read_le32(&payload[0U]), 0x2468ACE0U);
    EXPECT_EQ(payload[4U], 0x3CU);
    EXPECT_EQ(payload[5U], 0U);
    EXPECT_EQ(payload[6U], 0U);
    EXPECT_EQ(payload[7U], 0U);
    EXPECT_EQ(static_cast<int32_t>(castle::read_le32(&payload[8U])), -123456789);
    EXPECT_EQ(static_cast<int32_t>(castle::read_le32(&payload[12U])), 98765432);
    EXPECT_EQ(payload[16U], 0xC7U);
    EXPECT_EQ(payload[17U], 199U);
    EXPECT_EQ(payload[18U], 0x05U);
    EXPECT_EQ(payload[19U], 1U);
    EXPECT_EQ(payload[20U], 1U);
    EXPECT_EQ(payload[21U], 2U);
    EXPECT_EQ(payload[22U], 0U);
    EXPECT_EQ(payload[23U], 0U);
    EXPECT_EQ(payload[24U], 0U);
    EXPECT_EQ(payload[25U], 0U);
    EXPECT_EQ(static_cast<int16_t>(castle::read_le16(&payload[26U])), -7000);
}

TEST(UbxMessageNavSlas, AcceptsEmptyGroup)
{
    uint8_t payload[20U] = {};
    castle::write_le32(&payload[0U], 6U);
    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SLAS, payload, 20U);
    nav_slas<> out;
    ASSERT_TRUE(nav_slas<>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 6U);
    EXPECT_EQ(out.cnt, 0U);
    EXPECT_TRUE(out.corrections.empty());
}

TEST(UbxMessageNavSlas, RejectsWrongClassOrId)
{
    uint8_t payload[20U] = {};
    castle::write_le32(&payload[0U], 6U);
    nav_slas<> out;
    out.i_tow = 1U;

    message_view wrong_class = make_view(UBX_CLASS_RXM, UBX_ID_NAV_SLAS, payload, 20U);
    EXPECT_FALSE(nav_slas<>::matches(wrong_class));
    EXPECT_FALSE(nav_slas<>::decode(wrong_class, out));
    EXPECT_EQ(out.i_tow, 0U);

    message_view wrong_id = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SBAS, payload, 20U);
    EXPECT_FALSE(nav_slas<>::matches(wrong_id));
    EXPECT_FALSE(nav_slas<>::decode(wrong_id, out));
}

TEST(UbxMessageNavSlas, RejectsShortHeader)
{
    uint8_t payload[19U] = {};
    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SLAS, payload, 19U);
    nav_slas<> out;
    EXPECT_FALSE(nav_slas<>::decode(raw, out));
}

TEST(UbxMessageNavSlas, RejectsGroupLengthMismatchAndResetsOutput)
{
    uint8_t payload[37U] = {};
    castle::write_le32(&payload[0U], 77U);
    payload[17U] = 193U;
    payload[19U] = 2U;
    nav_slas<> out;

    message_view too_short = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SLAS, payload, 35U);
    EXPECT_FALSE(nav_slas<>::decode(too_short, out));
    EXPECT_EQ(out.i_tow, 0U);
    EXPECT_EQ(out.qzss_sv_id, 0U);
    EXPECT_EQ(out.cnt, 0U);
    EXPECT_TRUE(out.corrections.empty());

    message_view too_long = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SLAS, payload, 37U);
    EXPECT_FALSE(nav_slas<>::decode(too_long, out));

    payload[19U] = 1U;
    message_view count_mismatch = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SLAS, payload, 36U);
    EXPECT_FALSE(nav_slas<>::decode(count_mismatch, out));
    EXPECT_EQ(out.cnt, 0U);
}

TEST(UbxMessageNavSlas, ClampsStorageButKeepsWireCount)
{
    nav_slas<4U> message = make_message();
    ASSERT_TRUE(castle::succeeded(message.corrections.push_back(make_correction(0U))));
    ASSERT_TRUE(castle::succeeded(message.corrections.push_back(make_correction(1U))));
    ASSERT_TRUE(castle::succeeded(message.corrections.push_back(make_correction(2U))));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));

    nav_slas<2U> out;
    ASSERT_TRUE(nav_slas<2U>::decode(raw, out));
    EXPECT_EQ(out.cnt, 3U);
    ASSERT_EQ(out.corrections.size(), 2U);
    expect_correction_eq(out.corrections[0U], make_correction(0U));
    expect_correction_eq(out.corrections[1U], make_correction(1U));
}

TEST(UbxMessageNavSlas, EncodeIntoTooSmallBufferReportsFull)
{
    nav_slas<4U> message = make_message();
    ASSERT_TRUE(castle::succeeded(message.corrections.push_back(make_correction(0U))));
    uint8_t frame[UBX_FRAME_OVERHEAD + 20U + 7U];
    castle::size_type written = 4U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageNavSlas, ServiceFlagAccessors)
{
    nav_slas<> message{};
    EXPECT_FALSE(message.gms_available());
    EXPECT_FALSE(message.qzss_sv_available());
    EXPECT_FALSE(message.test_mode());

    message.service_flags = nav_slas<>::SERVICE_FLAGS_GMS_AVAILABLE;
    EXPECT_TRUE(message.gms_available());
    EXPECT_FALSE(message.qzss_sv_available());
    message.service_flags = nav_slas<>::SERVICE_FLAGS_QZSS_SV_AVAILABLE;
    EXPECT_TRUE(message.qzss_sv_available());
    EXPECT_FALSE(message.test_mode());
    message.service_flags = nav_slas<>::SERVICE_FLAGS_TEST_MODE;
    EXPECT_TRUE(message.test_mode());
    EXPECT_FALSE(message.gms_available());
    message.service_flags = 0xF8U; // unassigned bits
    EXPECT_FALSE(message.test_mode());
}

TEST(UbxMessageNavSlas, ScaledHelpers)
{
    nav_slas<> message{};
    message.gms_lon = -139692;
    message.gms_lat = 35689;
    EXPECT_DOUBLE_EQ(message.gms_longitude_deg(), -139.692);
    EXPECT_DOUBLE_EQ(message.gms_latitude_deg(), 35.689);

    nav_slas_correction correction{};
    correction.prc = -250;
    EXPECT_DOUBLE_EQ(correction.prc_m(), -2.5);
}

} // namespace
