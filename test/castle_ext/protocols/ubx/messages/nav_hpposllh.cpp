#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_hpposllh.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_hpposllh;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

message distinctive()
{
    message value{};
    value.version = 0x5AU;
    value.flags = 0xA5U;
    value.i_tow = 0x11223344U;
    value.lon = -1223456789;
    value.lat = 374219999;
    value.height = -45678;
    value.h_msl = 98765;
    value.lon_hp = -99;
    value.lat_hp = 98;
    value.height_hp = -9;
    value.h_msl_hp = 8;
    value.h_acc = 0xA1B2C3D4U;
    value.v_acc = 0x0F1E2D3CU;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.version, 0U);
    EXPECT_EQ(value.flags, 0U);
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.lon, 0);
    EXPECT_EQ(value.lat, 0);
    EXPECT_EQ(value.height, 0);
    EXPECT_EQ(value.h_msl, 0);
    EXPECT_EQ(value.lon_hp, 0);
    EXPECT_EQ(value.lat_hp, 0);
    EXPECT_EQ(value.height_hp, 0);
    EXPECT_EQ(value.h_msl_hp, 0);
    EXPECT_EQ(value.h_acc, 0U);
    EXPECT_EQ(value.v_acc, 0U);
}

TEST(UbxMessageNavHpposllh, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_HPPOSLLH);
    EXPECT_EQ(message::payload_length, 36U);
    EXPECT_EQ(message::FLAGS_INVALID_LLH, 0x01U);
}

TEST(UbxMessageNavHpposllh, RoundTripPreservesEveryField)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(message::matches(raw));

    message decoded{};
    ASSERT_TRUE(message::decode(raw, decoded));
    EXPECT_EQ(decoded.version, original.version);
    EXPECT_EQ(decoded.flags, original.flags);
    EXPECT_EQ(decoded.i_tow, original.i_tow);
    EXPECT_EQ(decoded.lon, original.lon);
    EXPECT_EQ(decoded.lat, original.lat);
    EXPECT_EQ(decoded.height, original.height);
    EXPECT_EQ(decoded.h_msl, original.h_msl);
    EXPECT_EQ(decoded.lon_hp, original.lon_hp);
    EXPECT_EQ(decoded.lat_hp, original.lat_hp);
    EXPECT_EQ(decoded.height_hp, original.height_hp);
    EXPECT_EQ(decoded.h_msl_hp, original.h_msl_hp);
    EXPECT_EQ(decoded.h_acc, original.h_acc);
    EXPECT_EQ(decoded.v_acc, original.v_acc);
}

TEST(UbxMessageNavHpposllh, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    payload[0U] = 0x00U;
    payload[1U] = 0xEEU; // reserved0
    payload[2U] = 0xEEU;
    payload[3U] = 0x01U;
    castle::write_le32(&payload[4U], 100001U);
    castle::write_le32s(&payload[8U], -200002);
    castle::write_le32s(&payload[12U], 300003);
    castle::write_le32s(&payload[16U], -400004);
    castle::write_le32s(&payload[20U], 500005);
    castle::write_le8s(&payload[24U], -11);
    castle::write_le8s(&payload[25U], 22);
    castle::write_le8s(&payload[26U], -3);
    castle::write_le8s(&payload[27U], 4);
    castle::write_le32(&payload[28U], 600006U);
    castle::write_le32(&payload[32U], 700007U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.version, 0x00U);
    EXPECT_EQ(decoded.flags, 0x01U);
    EXPECT_TRUE(decoded.invalid_llh());
    EXPECT_EQ(decoded.i_tow, 100001U);
    EXPECT_EQ(decoded.lon, -200002);
    EXPECT_EQ(decoded.lat, 300003);
    EXPECT_EQ(decoded.height, -400004);
    EXPECT_EQ(decoded.h_msl, 500005);
    EXPECT_EQ(decoded.lon_hp, -11);
    EXPECT_EQ(decoded.lat_hp, 22);
    EXPECT_EQ(decoded.height_hp, -3);
    EXPECT_EQ(decoded.h_msl_hp, 4);
    EXPECT_EQ(decoded.h_acc, 600006U);
    EXPECT_EQ(decoded.v_acc, 700007U);
}

TEST(UbxMessageNavHpposllh, EncodesFieldsAtSpecOffsetsWithZeroReserved)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(payload[0U], original.version);
    EXPECT_EQ(payload[1U], 0U);
    EXPECT_EQ(payload[2U], 0U);
    EXPECT_EQ(payload[3U], original.flags);
    EXPECT_EQ(castle::read_le32(&payload[4U]), original.i_tow);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.lon);
    EXPECT_EQ(castle::read_le32s(&payload[12U]), original.lat);
    EXPECT_EQ(castle::read_le32s(&payload[16U]), original.height);
    EXPECT_EQ(castle::read_le32s(&payload[20U]), original.h_msl);
    EXPECT_EQ(castle::read_le8s(&payload[24U]), original.lon_hp);
    EXPECT_EQ(castle::read_le8s(&payload[25U]), original.lat_hp);
    EXPECT_EQ(castle::read_le8s(&payload[26U]), original.height_hp);
    EXPECT_EQ(castle::read_le8s(&payload[27U]), original.h_msl_hp);
    EXPECT_EQ(castle::read_le32(&payload[28U]), original.h_acc);
    EXPECT_EQ(castle::read_le32(&payload[32U]), original.v_acc);
}

TEST(UbxMessageNavHpposllh, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV2;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_NAV_POSLLH;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavHpposllh, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length + 1U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavHpposllh, InvalidLlhFlagSetAndClear)
{
    message value{};
    EXPECT_FALSE(value.invalid_llh());
    value.flags = message::FLAGS_INVALID_LLH;
    EXPECT_TRUE(value.invalid_llh());
    value.flags = static_cast<uint8_t>(0xFFU & ~message::FLAGS_INVALID_LLH);
    EXPECT_FALSE(value.invalid_llh());
}

TEST(UbxMessageNavHpposllh, CombinesStandardAndHighPrecisionParts)
{
    message value{};
    value.lon = -1223456789;   // -122.3456789 deg
    value.lon_hp = -99;        // -0.000000099 deg
    value.lat = 374219999;     // 37.4219999 deg
    value.lat_hp = 50;         // +0.00000005 deg
    value.height = -12345;     // -12.345 m
    value.height_hp = -9;      // -0.9 mm
    value.h_msl = 67890;       // 67.890 m
    value.h_msl_hp = 9;        // +0.9 mm
    value.h_acc = 140U;        // 14.0 mm
    value.v_acc = 215U;        // 21.5 mm
    EXPECT_NEAR(value.longitude_deg(), -122.345678999, 1e-12);
    EXPECT_NEAR(value.latitude_deg(), 37.42199995, 1e-12);
    EXPECT_NEAR(value.height_m(), -12.3459, 1e-12);
    EXPECT_NEAR(value.h_msl_m(), 67.8909, 1e-12);
    EXPECT_NEAR(value.h_acc_m(), 0.014, 1e-15);
    EXPECT_NEAR(value.v_acc_m(), 0.0215, 1e-15);

    // HP parts alone: 1e-9 deg and 0.1 mm per LSB.
    message hp_only{};
    hp_only.lon_hp = 1;
    hp_only.lat_hp = -1;
    hp_only.height_hp = 1;
    hp_only.h_msl_hp = -1;
    EXPECT_NEAR(hp_only.longitude_deg(), 1e-9, 1e-18);
    EXPECT_NEAR(hp_only.latitude_deg(), -1e-9, 1e-18);
    EXPECT_NEAR(hp_only.height_m(), 1e-4, 1e-15);
    EXPECT_NEAR(hp_only.h_msl_m(), -1e-4, 1e-15);
}

TEST(UbxMessageNavHpposllh, EncodeIntoTooSmallBufferReportsFull)
{
    const message value = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
