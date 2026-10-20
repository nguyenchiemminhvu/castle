#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_velned.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_velned;

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
    value.vel_n = -101010;
    value.vel_e = 202020;
    value.vel_d = -303030;
    value.speed = 0x0A0B0C0DU;
    value.g_speed = 0x01020304U;
    value.heading = -17999999;
    value.s_acc = 0xA1B2C3D4U;
    value.c_acc = 0x0F1E2D3CU;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.vel_n, 0);
    EXPECT_EQ(value.vel_e, 0);
    EXPECT_EQ(value.vel_d, 0);
    EXPECT_EQ(value.speed, 0U);
    EXPECT_EQ(value.g_speed, 0U);
    EXPECT_EQ(value.heading, 0);
    EXPECT_EQ(value.s_acc, 0U);
    EXPECT_EQ(value.c_acc, 0U);
}

TEST(UbxMessageNavVelned, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_VELNED);
    EXPECT_EQ(message::payload_length, 36U);
}

TEST(UbxMessageNavVelned, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.vel_n, original.vel_n);
    EXPECT_EQ(decoded.vel_e, original.vel_e);
    EXPECT_EQ(decoded.vel_d, original.vel_d);
    EXPECT_EQ(decoded.speed, original.speed);
    EXPECT_EQ(decoded.g_speed, original.g_speed);
    EXPECT_EQ(decoded.heading, original.heading);
    EXPECT_EQ(decoded.s_acc, original.s_acc);
    EXPECT_EQ(decoded.c_acc, original.c_acc);
}

TEST(UbxMessageNavVelned, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 100001U);
    castle::write_le32s(&payload[4U], -200002);
    castle::write_le32s(&payload[8U], 300003);
    castle::write_le32s(&payload[12U], -400004);
    castle::write_le32(&payload[16U], 500005U);
    castle::write_le32(&payload[20U], 600006U);
    castle::write_le32s(&payload[24U], -700007);
    castle::write_le32(&payload[28U], 800008U);
    castle::write_le32(&payload[32U], 900009U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 100001U);
    EXPECT_EQ(decoded.vel_n, -200002);
    EXPECT_EQ(decoded.vel_e, 300003);
    EXPECT_EQ(decoded.vel_d, -400004);
    EXPECT_EQ(decoded.speed, 500005U);
    EXPECT_EQ(decoded.g_speed, 600006U);
    EXPECT_EQ(decoded.heading, -700007);
    EXPECT_EQ(decoded.s_acc, 800008U);
    EXPECT_EQ(decoded.c_acc, 900009U);
}

TEST(UbxMessageNavVelned, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(castle::read_le32(&payload[0U]), original.i_tow);
    EXPECT_EQ(castle::read_le32s(&payload[4U]), original.vel_n);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.vel_e);
    EXPECT_EQ(castle::read_le32s(&payload[12U]), original.vel_d);
    EXPECT_EQ(castle::read_le32(&payload[16U]), original.speed);
    EXPECT_EQ(castle::read_le32(&payload[20U]), original.g_speed);
    EXPECT_EQ(castle::read_le32s(&payload[24U]), original.heading);
    EXPECT_EQ(castle::read_le32(&payload[28U]), original.s_acc);
    EXPECT_EQ(castle::read_le32(&payload[32U]), original.c_acc);
}

TEST(UbxMessageNavVelned, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV2;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_NAV_VELECEF;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavVelned, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length + 1U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavVelned, ScaledHelpers)
{
    message value{};
    value.vel_n = -1414;
    value.vel_e = 707;
    value.vel_d = -12;
    value.speed = 2000U;
    value.g_speed = 1999U;
    value.heading = -9012345;
    value.s_acc = 30U;
    value.c_acc = 125000U;
    EXPECT_NEAR(value.vel_n_m_s(), -14.14, 1e-12);
    EXPECT_NEAR(value.vel_e_m_s(), 7.07, 1e-12);
    EXPECT_NEAR(value.vel_d_m_s(), -0.12, 1e-12);
    EXPECT_NEAR(value.speed_m_s(), 20.0, 1e-12);
    EXPECT_NEAR(value.g_speed_m_s(), 19.99, 1e-12);
    EXPECT_NEAR(value.heading_deg(), -90.12345, 1e-12);
    EXPECT_NEAR(value.s_acc_m_s(), 0.30, 1e-12);
    EXPECT_NEAR(value.c_acc_deg(), 1.25, 1e-12);
}

TEST(UbxMessageNavVelned, EncodeIntoTooSmallBufferReportsFull)
{
    const message value = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
