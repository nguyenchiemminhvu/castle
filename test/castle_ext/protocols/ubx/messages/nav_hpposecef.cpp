#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_hpposecef.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_hpposecef;

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
    value.i_tow = 0x11223344U;
    value.ecef_x = -428242234;
    value.ecef_y = 64003318;
    value.ecef_z = -467233890;
    value.ecef_x_hp = -99;
    value.ecef_y_hp = 42;
    value.ecef_z_hp = 99;
    value.flags = 0xA5U;
    value.p_acc = 0xA1B2C3D4U;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.version, 0U);
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.ecef_x, 0);
    EXPECT_EQ(value.ecef_y, 0);
    EXPECT_EQ(value.ecef_z, 0);
    EXPECT_EQ(value.ecef_x_hp, 0);
    EXPECT_EQ(value.ecef_y_hp, 0);
    EXPECT_EQ(value.ecef_z_hp, 0);
    EXPECT_EQ(value.flags, 0U);
    EXPECT_EQ(value.p_acc, 0U);
}

TEST(UbxMessageNavHpposecef, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_HPPOSECEF);
    EXPECT_EQ(message::payload_length, 28U);
    EXPECT_EQ(message::FLAGS_INVALID_ECEF, 0x01U);
}

TEST(UbxMessageNavHpposecef, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.i_tow, original.i_tow);
    EXPECT_EQ(decoded.ecef_x, original.ecef_x);
    EXPECT_EQ(decoded.ecef_y, original.ecef_y);
    EXPECT_EQ(decoded.ecef_z, original.ecef_z);
    EXPECT_EQ(decoded.ecef_x_hp, original.ecef_x_hp);
    EXPECT_EQ(decoded.ecef_y_hp, original.ecef_y_hp);
    EXPECT_EQ(decoded.ecef_z_hp, original.ecef_z_hp);
    EXPECT_EQ(decoded.flags, original.flags);
    EXPECT_EQ(decoded.p_acc, original.p_acc);
}

TEST(UbxMessageNavHpposecef, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    payload[0U] = 0x00U;
    payload[1U] = 0xEEU; // reserved0
    payload[2U] = 0xEEU;
    payload[3U] = 0xEEU;
    castle::write_le32(&payload[4U], 100001U);
    castle::write_le32s(&payload[8U], -200002);
    castle::write_le32s(&payload[12U], 300003);
    castle::write_le32s(&payload[16U], -400004);
    castle::write_le8s(&payload[20U], -11);
    castle::write_le8s(&payload[21U], 22);
    castle::write_le8s(&payload[22U], -33);
    payload[23U] = 0x01U;
    castle::write_le32(&payload[24U], 500005U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.version, 0x00U);
    EXPECT_EQ(decoded.i_tow, 100001U);
    EXPECT_EQ(decoded.ecef_x, -200002);
    EXPECT_EQ(decoded.ecef_y, 300003);
    EXPECT_EQ(decoded.ecef_z, -400004);
    EXPECT_EQ(decoded.ecef_x_hp, -11);
    EXPECT_EQ(decoded.ecef_y_hp, 22);
    EXPECT_EQ(decoded.ecef_z_hp, -33);
    EXPECT_EQ(decoded.flags, 0x01U);
    EXPECT_TRUE(decoded.invalid_ecef());
    EXPECT_EQ(decoded.p_acc, 500005U);
}

TEST(UbxMessageNavHpposecef, EncodesFieldsAtSpecOffsetsWithZeroReserved)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(payload[0U], original.version);
    EXPECT_EQ(payload[1U], 0U);
    EXPECT_EQ(payload[2U], 0U);
    EXPECT_EQ(payload[3U], 0U);
    EXPECT_EQ(castle::read_le32(&payload[4U]), original.i_tow);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.ecef_x);
    EXPECT_EQ(castle::read_le32s(&payload[12U]), original.ecef_y);
    EXPECT_EQ(castle::read_le32s(&payload[16U]), original.ecef_z);
    EXPECT_EQ(castle::read_le8s(&payload[20U]), original.ecef_x_hp);
    EXPECT_EQ(castle::read_le8s(&payload[21U]), original.ecef_y_hp);
    EXPECT_EQ(castle::read_le8s(&payload[22U]), original.ecef_z_hp);
    EXPECT_EQ(payload[23U], original.flags);
    EXPECT_EQ(castle::read_le32(&payload[24U]), original.p_acc);
}

TEST(UbxMessageNavHpposecef, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV2;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_NAV_POSECEF;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavHpposecef, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length + 1U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavHpposecef, InvalidEcefFlagSetAndClear)
{
    message value{};
    EXPECT_FALSE(value.invalid_ecef());
    value.flags = message::FLAGS_INVALID_ECEF;
    EXPECT_TRUE(value.invalid_ecef());
    value.flags = static_cast<uint8_t>(0xFFU & ~message::FLAGS_INVALID_ECEF);
    EXPECT_FALSE(value.invalid_ecef());
}

TEST(UbxMessageNavHpposecef, CombinesCentimetreAndHighPrecisionParts)
{
    message value{};
    value.ecef_x = 428242234;   // 4282422.34 m
    value.ecef_x_hp = 56;       // + 5.6 mm
    value.ecef_y = -64003318;   // -640033.18 m
    value.ecef_y_hp = -99;      // - 9.9 mm
    value.ecef_z = 1;           // 0.01 m
    value.ecef_z_hp = -99;      // - 9.9 mm -> 0.1 mm net
    value.p_acc = 142U;         // 14.2 mm
    EXPECT_NEAR(value.ecef_x_m(), 4282422.3456, 1e-7);
    EXPECT_NEAR(value.ecef_y_m(), -640033.1899, 1e-7);
    EXPECT_NEAR(value.ecef_z_m(), 0.0001, 1e-12);
    EXPECT_NEAR(value.p_acc_m(), 0.0142, 1e-12);

    // The HP part alone is in 0.1 mm.
    message hp_only{};
    hp_only.ecef_x_hp = 10;
    EXPECT_NEAR(hp_only.ecef_x_m(), 0.001, 1e-15);
}

TEST(UbxMessageNavHpposecef, EncodeIntoTooSmallBufferReportsFull)
{
    const message value = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
