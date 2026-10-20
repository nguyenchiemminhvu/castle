#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_timebds.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_timebds;

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
    value.tow = 0x00055AA5U;
    value.f_tow = -123456789;
    value.week = 0x0389;
    value.leap_s = 4;
    value.valid = 0x07U;
    value.t_acc = 0xA1B2C3D4U;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.tow, 0U);
    EXPECT_EQ(value.f_tow, 0);
    EXPECT_EQ(value.week, 0);
    EXPECT_EQ(value.leap_s, 0);
    EXPECT_EQ(value.valid, 0U);
    EXPECT_EQ(value.t_acc, 0U);
}

TEST(UbxMessageNavTimebds, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_TIMEBDS);
    EXPECT_EQ(message::payload_length, 20U);
    EXPECT_EQ(message::VALID_SOW, 0x01U);
    EXPECT_EQ(message::VALID_TOW, 0x01U);
    EXPECT_EQ(message::VALID_WEEK, 0x02U);
    EXPECT_EQ(message::VALID_LEAP_S, 0x04U);
}

TEST(UbxMessageNavTimebds, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.tow, original.tow);
    EXPECT_EQ(decoded.f_tow, original.f_tow);
    EXPECT_EQ(decoded.week, original.week);
    EXPECT_EQ(decoded.leap_s, original.leap_s);
    EXPECT_EQ(decoded.valid, original.valid);
    EXPECT_EQ(decoded.t_acc, original.t_acc);
}

TEST(UbxMessageNavTimebds, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 504000123U);
    castle::write_le32(&payload[4U], 503986U);
    castle::write_le32s(&payload[8U], -499999999);
    castle::write_le16s(&payload[12U], 905);
    castle::write_le8s(&payload[14U], -3);
    payload[15U] = 0x05U;
    castle::write_le32(&payload[16U], 6006U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 504000123U);
    EXPECT_EQ(decoded.tow, 503986U);
    EXPECT_EQ(decoded.sow(), 503986U);
    EXPECT_EQ(decoded.f_tow, -499999999);
    EXPECT_EQ(decoded.f_sow(), -499999999);
    EXPECT_EQ(decoded.week, 905);
    EXPECT_EQ(decoded.leap_s, -3);
    EXPECT_EQ(decoded.valid, 0x05U);
    EXPECT_EQ(decoded.t_acc, 6006U);
}

TEST(UbxMessageNavTimebds, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_NAV);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_NAV_TIMEBDS);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(castle::read_le32(&payload[0U]), original.i_tow);
    EXPECT_EQ(castle::read_le32(&payload[4U]), original.tow);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.f_tow);
    EXPECT_EQ(castle::read_le16s(&payload[12U]), original.week);
    EXPECT_EQ(static_cast<int8_t>(payload[14U]), original.leap_s);
    EXPECT_EQ(payload[15U], original.valid);
    EXPECT_EQ(castle::read_le32(&payload[16U]), original.t_acc);
}

TEST(UbxMessageNavTimebds, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 7U);
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV2;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    // TIMEGAL / TIMEQZSS share the layout but must not be accepted as TIMEBDS.
    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_NAV_TIMEGAL;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_id = UBX_ID_NAV_TIMEQZSS;
    EXPECT_FALSE(message::matches(raw));
}

TEST(UbxMessageNavTimebds, RejectsShortAndLongPayloads)
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

TEST(UbxMessageNavTimebds, ValidityBitsSetAndClear)
{
    message value{};
    EXPECT_FALSE(value.sow_valid());
    EXPECT_FALSE(value.tow_valid());
    EXPECT_FALSE(value.week_valid());
    EXPECT_FALSE(value.leap_s_valid());

    value.valid = message::VALID_SOW;
    EXPECT_TRUE(value.sow_valid());
    EXPECT_TRUE(value.tow_valid());
    EXPECT_FALSE(value.week_valid());
    EXPECT_FALSE(value.leap_s_valid());

    value.valid = message::VALID_WEEK;
    EXPECT_FALSE(value.sow_valid());
    EXPECT_TRUE(value.week_valid());
    EXPECT_FALSE(value.leap_s_valid());

    value.valid = message::VALID_LEAP_S;
    EXPECT_FALSE(value.sow_valid());
    EXPECT_FALSE(value.week_valid());
    EXPECT_TRUE(value.leap_s_valid());

    value.valid = 0xF8U; // undefined bits only
    EXPECT_FALSE(value.sow_valid());
    EXPECT_FALSE(value.week_valid());
    EXPECT_FALSE(value.leap_s_valid());
}

TEST(UbxMessageNavTimebds, SpecNamedAccessorsAliasGenericFields)
{
    message value{};
    value.tow = 86400U;
    value.f_tow = -250000000;
    EXPECT_EQ(value.sow(), 86400U);
    EXPECT_EQ(value.f_sow(), -250000000);
}

TEST(UbxMessageNavTimebds, TimeOfWeekHelper)
{
    message value{};
    value.tow = 100U;
    value.f_tow = -250000000;
    EXPECT_NEAR(value.time_of_week_s(), 99.75, 1e-12);

    value.f_tow = 499999999;
    EXPECT_NEAR(value.time_of_week_s(), 100.499999999, 1e-12);

    const message zero{};
    EXPECT_EQ(zero.time_of_week_s(), 0.0);
}

TEST(UbxMessageNavTimebds, EncodeIntoTooSmallBufferReportsFull)
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
