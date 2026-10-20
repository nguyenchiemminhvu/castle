#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav2_timegps.hpp"
#include "castle/utility/bytes.hpp"
#include "castle/core/traits.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav2_timegps;

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
    value.f_tow = -456789;
    value.week = 0x0912;
    value.leap_s = 18;
    value.valid = 0x07U;
    value.t_acc = 0xA1B2C3D4U;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.f_tow, 0);
    EXPECT_EQ(value.week, 0);
    EXPECT_EQ(value.leap_s, 0);
    EXPECT_EQ(value.valid, 0U);
    EXPECT_EQ(value.t_acc, 0U);
}

TEST(UbxMessageNav2Timegps, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV2);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV2_TIMEGPS);
    EXPECT_EQ(message::payload_length, 16U);
    EXPECT_EQ(message::VALID_TOW, 0x01U);
    EXPECT_EQ(message::VALID_WEEK, 0x02U);
    EXPECT_EQ(message::VALID_LEAP_S, 0x04U);
}

TEST(UbxMessageNav2Timegps, SharesLayoutWithNavTimegps)
{
    EXPECT_TRUE((castle::meta::is_same<message, basic_nav_timegps<UBX_CLASS_NAV2, UBX_ID_NAV2_TIMEGPS>>::value));
    EXPECT_FALSE((castle::meta::is_same<message, nav_timegps>::value));
    EXPECT_EQ(message::payload_length, nav_timegps::payload_length);

    // Identical payload bytes decode to identical fields under either class.
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 123456U);
    castle::write_le16s(&payload[8U], 2345);
    payload[11U] = 0x07U;
    message nav2{};
    nav_timegps nav{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), nav2));
    const message_view nav_raw{{UBX_CLASS_NAV, UBX_ID_NAV_TIMEGPS, static_cast<uint16_t>(sizeof(payload))},
                               array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)),
                               checksum{}};
    ASSERT_TRUE(nav_timegps::decode(nav_raw, nav));
    EXPECT_EQ(nav2.i_tow, nav.i_tow);
    EXPECT_EQ(nav2.week, nav.week);
    EXPECT_EQ(nav2.valid, nav.valid);
}

TEST(UbxMessageNav2Timegps, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.f_tow, original.f_tow);
    EXPECT_EQ(decoded.week, original.week);
    EXPECT_EQ(decoded.leap_s, original.leap_s);
    EXPECT_EQ(decoded.valid, original.valid);
    EXPECT_EQ(decoded.t_acc, original.t_acc);
}

TEST(UbxMessageNav2Timegps, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 345000U);
    castle::write_le32s(&payload[4U], -250);
    castle::write_le16s(&payload[8U], 2300);
    castle::write_le8s(&payload[10U], 18);
    payload[11U] = 0x03U;
    castle::write_le32(&payload[12U], 50U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 345000U);
    EXPECT_EQ(decoded.f_tow, -250);
    EXPECT_EQ(decoded.week, 2300);
    EXPECT_EQ(decoded.leap_s, 18);
    EXPECT_EQ(decoded.valid, 0x03U);
    EXPECT_EQ(decoded.t_acc, 50U);
}

TEST(UbxMessageNav2Timegps, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_NAV2);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_NAV2_TIMEGPS);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(castle::read_le32(&payload[0U]), original.i_tow);
    EXPECT_EQ(castle::read_le32s(&payload[4U]), original.f_tow);
    EXPECT_EQ(castle::read_le16s(&payload[8U]), original.week);
    EXPECT_EQ(static_cast<int8_t>(payload[10U]), original.leap_s);
    EXPECT_EQ(payload[11U], original.valid);
    EXPECT_EQ(castle::read_le32(&payload[12U]), original.t_acc);
}

TEST(UbxMessageNav2Timegps, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 7U);
    message_view raw = view_of(payload, sizeof(payload));

    // NAV-TIMEGPS has the same ID and layout but a different class.
    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_NAV2_PVT;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNav2Timegps, RejectsShortAndLongPayloads)
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

TEST(UbxMessageNav2Timegps, ValidityBitsSetAndClear)
{
    message value{};
    EXPECT_FALSE(value.tow_valid());
    EXPECT_FALSE(value.week_valid());
    EXPECT_FALSE(value.leap_s_valid());

    value.valid = message::VALID_TOW;
    EXPECT_TRUE(value.tow_valid());
    EXPECT_FALSE(value.week_valid());
    EXPECT_FALSE(value.leap_s_valid());

    value.valid = message::VALID_WEEK;
    EXPECT_FALSE(value.tow_valid());
    EXPECT_TRUE(value.week_valid());
    EXPECT_FALSE(value.leap_s_valid());

    value.valid = message::VALID_LEAP_S;
    EXPECT_FALSE(value.tow_valid());
    EXPECT_FALSE(value.week_valid());
    EXPECT_TRUE(value.leap_s_valid());

    value.valid = 0xF8U; // undefined bits only
    EXPECT_FALSE(value.tow_valid());
    EXPECT_FALSE(value.week_valid());
    EXPECT_FALSE(value.leap_s_valid());
}

TEST(UbxMessageNav2Timegps, TimeOfWeekHelper)
{
    message value{};
    value.i_tow = 345000U;
    value.f_tow = -250;
    EXPECT_NEAR(value.time_of_week_s(), 344.99999975, 1e-12);

    value.f_tow = 500000;
    EXPECT_NEAR(value.time_of_week_s(), 345.0005, 1e-12);

    const message zero{};
    EXPECT_EQ(zero.time_of_week_s(), 0.0);
}

TEST(UbxMessageNav2Timegps, EncodeIntoTooSmallBufferReportsFull)
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
