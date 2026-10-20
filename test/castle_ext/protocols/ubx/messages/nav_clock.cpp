#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_clock.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_clock;

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
    value.clk_b = -1234567890;
    value.clk_d = 876543;
    value.t_acc = 0xA1B2C3D4U;
    value.f_acc = 0x0F1E2D3CU;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.clk_b, 0);
    EXPECT_EQ(value.clk_d, 0);
    EXPECT_EQ(value.t_acc, 0U);
    EXPECT_EQ(value.f_acc, 0U);
}

TEST(UbxMessageNavClock, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_CLOCK);
    EXPECT_EQ(message::payload_length, 20U);
}

TEST(UbxMessageNavClock, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.clk_b, original.clk_b);
    EXPECT_EQ(decoded.clk_d, original.clk_d);
    EXPECT_EQ(decoded.t_acc, original.t_acc);
    EXPECT_EQ(decoded.f_acc, original.f_acc);
}

TEST(UbxMessageNavClock, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 504000123U);
    castle::write_le32s(&payload[4U], -1000002);
    castle::write_le32s(&payload[8U], 3003);
    castle::write_le32(&payload[12U], 4004U);
    castle::write_le32(&payload[16U], 5005U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 504000123U);
    EXPECT_EQ(decoded.clk_b, -1000002);
    EXPECT_EQ(decoded.clk_d, 3003);
    EXPECT_EQ(decoded.t_acc, 4004U);
    EXPECT_EQ(decoded.f_acc, 5005U);
}

TEST(UbxMessageNavClock, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_NAV);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_NAV_CLOCK);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(castle::read_le32(&payload[0U]), original.i_tow);
    EXPECT_EQ(castle::read_le32s(&payload[4U]), original.clk_b);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.clk_d);
    EXPECT_EQ(castle::read_le32(&payload[12U]), original.t_acc);
    EXPECT_EQ(castle::read_le32(&payload[16U]), original.f_acc);
}

TEST(UbxMessageNavClock, RejectsWrongClassOrId)
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
    raw.header.msg_id = UBX_ID_NAV_TIMEUTC; // same 20-byte length
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavClock, RejectsShortAndLongPayloads)
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

TEST(UbxMessageNavClock, EncodeIntoTooSmallBufferReportsFull)
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
