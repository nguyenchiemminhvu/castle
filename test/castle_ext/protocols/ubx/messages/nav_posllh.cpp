#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_posllh.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_posllh;

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
    value.lon = -1234567890;
    value.lat = 876543210;
    value.height = -45678;
    value.h_msl = 98765;
    value.h_acc = 0xA1B2C3D4U;
    value.v_acc = 0x0F1E2D3CU;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.lon, 0);
    EXPECT_EQ(value.lat, 0);
    EXPECT_EQ(value.height, 0);
    EXPECT_EQ(value.h_msl, 0);
    EXPECT_EQ(value.h_acc, 0U);
    EXPECT_EQ(value.v_acc, 0U);
}

TEST(UbxMessageNavPosllh, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_POSLLH);
    EXPECT_EQ(message::payload_length, 28U);
}

TEST(UbxMessageNavPosllh, RoundTripPreservesEveryField)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(message::matches(raw));
    EXPECT_EQ(raw.payload_length(), message::payload_length);

    message decoded{};
    ASSERT_TRUE(message::decode(raw, decoded));
    EXPECT_EQ(decoded.i_tow, original.i_tow);
    EXPECT_EQ(decoded.lon, original.lon);
    EXPECT_EQ(decoded.lat, original.lat);
    EXPECT_EQ(decoded.height, original.height);
    EXPECT_EQ(decoded.h_msl, original.h_msl);
    EXPECT_EQ(decoded.h_acc, original.h_acc);
    EXPECT_EQ(decoded.v_acc, original.v_acc);
}

TEST(UbxMessageNavPosllh, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 504000123U);
    castle::write_le32s(&payload[4U], -1220000001);
    castle::write_le32s(&payload[8U], 374000002);
    castle::write_le32s(&payload[12U], -32003);
    castle::write_le32s(&payload[16U], 1004);
    castle::write_le32(&payload[20U], 4005U);
    castle::write_le32(&payload[24U], 6006U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 504000123U);
    EXPECT_EQ(decoded.lon, -1220000001);
    EXPECT_EQ(decoded.lat, 374000002);
    EXPECT_EQ(decoded.height, -32003);
    EXPECT_EQ(decoded.h_msl, 1004);
    EXPECT_EQ(decoded.h_acc, 4005U);
    EXPECT_EQ(decoded.v_acc, 6006U);
}

TEST(UbxMessageNavPosllh, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_NAV);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_NAV_POSLLH);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(castle::read_le32(&payload[0U]), original.i_tow);
    EXPECT_EQ(castle::read_le32s(&payload[4U]), original.lon);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.lat);
    EXPECT_EQ(castle::read_le32s(&payload[12U]), original.height);
    EXPECT_EQ(castle::read_le32s(&payload[16U]), original.h_msl);
    EXPECT_EQ(castle::read_le32(&payload[20U]), original.h_acc);
    EXPECT_EQ(castle::read_le32(&payload[24U]), original.v_acc);
}

TEST(UbxMessageNavPosllh, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 7U);
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV2;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_NAV_HPPOSLLH;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavPosllh, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    castle::write_le32(&payload[0U], 7U);

    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length + 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, 0U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavPosllh, ScaledHelpers)
{
    message value{};
    value.lon = -1223456789;
    value.lat = 374219999;
    value.height = -12345;
    value.h_msl = 67890;
    EXPECT_NEAR(value.longitude_deg(), -122.3456789, 1e-12);
    EXPECT_NEAR(value.latitude_deg(), 37.4219999, 1e-12);
    EXPECT_NEAR(value.height_m(), -12.345, 1e-12);
    EXPECT_NEAR(value.h_msl_m(), 67.890, 1e-12);

    const message zero{};
    EXPECT_EQ(zero.longitude_deg(), 0.0);
    EXPECT_EQ(zero.latitude_deg(), 0.0);
    EXPECT_EQ(zero.height_m(), 0.0);
    EXPECT_EQ(zero.h_msl_m(), 0.0);
}

TEST(UbxMessageNavPosllh, EncodeIntoTooSmallBufferReportsFull)
{
    const message value = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    written = 99U;
    EXPECT_EQ(encode_message(value, frame, UBX_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
