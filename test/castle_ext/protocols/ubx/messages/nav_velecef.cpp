#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_velecef.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_velecef;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

message distinctive()
{
    message value{};
    value.i_tow = 0x11223344U;
    value.ecef_vx = -123456;
    value.ecef_vy = 234567;
    value.ecef_vz = -345678;
    value.s_acc = 0xA1B2C3D4U;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.ecef_vx, 0);
    EXPECT_EQ(value.ecef_vy, 0);
    EXPECT_EQ(value.ecef_vz, 0);
    EXPECT_EQ(value.s_acc, 0U);
}

TEST(UbxMessageNavVelecef, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_VELECEF);
    EXPECT_EQ(message::payload_length, 20U);
}

TEST(UbxMessageNavVelecef, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.i_tow, original.i_tow);
    EXPECT_EQ(decoded.ecef_vx, original.ecef_vx);
    EXPECT_EQ(decoded.ecef_vy, original.ecef_vy);
    EXPECT_EQ(decoded.ecef_vz, original.ecef_vz);
    EXPECT_EQ(decoded.s_acc, original.s_acc);
}

TEST(UbxMessageNavVelecef, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 100001U);
    castle::write_le32s(&payload[4U], -200002);
    castle::write_le32s(&payload[8U], 300003);
    castle::write_le32s(&payload[12U], -400004);
    castle::write_le32(&payload[16U], 500005U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 100001U);
    EXPECT_EQ(decoded.ecef_vx, -200002);
    EXPECT_EQ(decoded.ecef_vy, 300003);
    EXPECT_EQ(decoded.ecef_vz, -400004);
    EXPECT_EQ(decoded.s_acc, 500005U);
}

TEST(UbxMessageNavVelecef, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(castle::read_le32(&payload[0U]), original.i_tow);
    EXPECT_EQ(castle::read_le32s(&payload[4U]), original.ecef_vx);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.ecef_vy);
    EXPECT_EQ(castle::read_le32s(&payload[12U]), original.ecef_vz);
    EXPECT_EQ(castle::read_le32(&payload[16U]), original.s_acc);
}

TEST(UbxMessageNavVelecef, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV2;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_NAV_VELNED;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavVelecef, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length + 1U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavVelecef, ScaledHelpers)
{
    message value{};
    value.ecef_vx = -1250;
    value.ecef_vy = 380;
    value.ecef_vz = 1105;
    value.s_acc = 45U;
    EXPECT_NEAR(value.ecef_vx_m_s(), -12.50, 1e-12);
    EXPECT_NEAR(value.ecef_vy_m_s(), 3.80, 1e-12);
    EXPECT_NEAR(value.ecef_vz_m_s(), 11.05, 1e-12);
    EXPECT_NEAR(value.s_acc_m_s(), 0.45, 1e-12);
}

TEST(UbxMessageNavVelecef, EncodeIntoTooSmallBufferReportsFull)
{
    const message value = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
