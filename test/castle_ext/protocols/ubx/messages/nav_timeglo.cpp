#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_timeglo.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_timeglo;

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
    value.tod = 0x00012345U;
    value.f_tod = -123456789;
    value.nt = 0x05A1U;
    value.n4 = 0x08U;
    value.valid = 0x03U;
    value.t_acc = 0xA1B2C3D4U;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.tod, 0U);
    EXPECT_EQ(value.f_tod, 0);
    EXPECT_EQ(value.nt, 0U);
    EXPECT_EQ(value.n4, 0U);
    EXPECT_EQ(value.valid, 0U);
    EXPECT_EQ(value.t_acc, 0U);
}

TEST(UbxMessageNavTimeglo, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_TIMEGLO);
    EXPECT_EQ(message::payload_length, 20U);
    EXPECT_EQ(message::VALID_TOD, 0x01U);
    EXPECT_EQ(message::VALID_DATE, 0x02U);
}

TEST(UbxMessageNavTimeglo, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.tod, original.tod);
    EXPECT_EQ(decoded.f_tod, original.f_tod);
    EXPECT_EQ(decoded.nt, original.nt);
    EXPECT_EQ(decoded.n4, original.n4);
    EXPECT_EQ(decoded.valid, original.valid);
    EXPECT_EQ(decoded.t_acc, original.t_acc);
}

TEST(UbxMessageNavTimeglo, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 504000123U);
    castle::write_le32(&payload[4U], 86399U);
    castle::write_le32s(&payload[8U], -499999999);
    castle::write_le16(&payload[12U], 1461U);
    payload[14U] = 7U;
    payload[15U] = 0x02U;
    castle::write_le32(&payload[16U], 6006U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 504000123U);
    EXPECT_EQ(decoded.tod, 86399U);
    EXPECT_EQ(decoded.f_tod, -499999999);
    EXPECT_EQ(decoded.nt, 1461U);
    EXPECT_EQ(decoded.n4, 7U);
    EXPECT_EQ(decoded.valid, 0x02U);
    EXPECT_EQ(decoded.t_acc, 6006U);
}

TEST(UbxMessageNavTimeglo, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_NAV);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_NAV_TIMEGLO);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(castle::read_le32(&payload[0U]), original.i_tow);
    EXPECT_EQ(castle::read_le32(&payload[4U]), original.tod);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.f_tod);
    EXPECT_EQ(castle::read_le16(&payload[12U]), original.nt);
    EXPECT_EQ(payload[14U], original.n4);
    EXPECT_EQ(payload[15U], original.valid);
    EXPECT_EQ(castle::read_le32(&payload[16U]), original.t_acc);
}

TEST(UbxMessageNavTimeglo, RejectsWrongClassOrId)
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
    raw.header.msg_id = UBX_ID_NAV_TIMEBDS;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavTimeglo, RejectsShortAndLongPayloads)
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

TEST(UbxMessageNavTimeglo, ValidityBitsSetAndClear)
{
    message value{};
    EXPECT_FALSE(value.tod_valid());
    EXPECT_FALSE(value.date_valid());

    value.valid = message::VALID_TOD;
    EXPECT_TRUE(value.tod_valid());
    EXPECT_FALSE(value.date_valid());

    value.valid = message::VALID_DATE;
    EXPECT_FALSE(value.tod_valid());
    EXPECT_TRUE(value.date_valid());

    value.valid = static_cast<uint8_t>(message::VALID_TOD | message::VALID_DATE);
    EXPECT_TRUE(value.tod_valid());
    EXPECT_TRUE(value.date_valid());

    value.valid = 0xFCU; // undefined bits only
    EXPECT_FALSE(value.tod_valid());
    EXPECT_FALSE(value.date_valid());
}

TEST(UbxMessageNavTimeglo, TimeOfDayHelper)
{
    message value{};
    value.tod = 3600U;
    value.f_tod = -250000000;
    EXPECT_NEAR(value.time_of_day_s(), 3599.75, 1e-12);

    value.f_tod = 125000000;
    EXPECT_NEAR(value.time_of_day_s(), 3600.125, 1e-12);

    const message zero{};
    EXPECT_EQ(zero.time_of_day_s(), 0.0);
}

TEST(UbxMessageNavTimeglo, EncodeIntoTooSmallBufferReportsFull)
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
