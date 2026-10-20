#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_att.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_att;
CASTLE_CONSTEXPR uint8_t expected_class = UBX_CLASS_NAV;
CASTLE_CONSTEXPR uint8_t expected_id = UBX_ID_NAV_ATT;
CASTLE_CONSTEXPR uint8_t other_class = UBX_CLASS_NAV2;
CASTLE_CONSTEXPR castle::size_type length = 32U;

message_view view_of(CASTLE_CONST uint8_t* payload, castle::size_type size,
                     uint8_t msg_class = expected_class, uint8_t msg_id = expected_id)
{
    return message_view{{msg_class, msg_id, static_cast<uint16_t>(size)},
                        array_view<CASTLE_CONST uint8_t>(payload, size), checksum{}};
}

message make_message()
{
    message value;
    value.i_tow = 345600123U;
    value.version = 1U;
    value.roll = -1234567;
    value.pitch = 2345678;
    value.heading = 34567890;
    value.acc_roll = 12345U;
    value.acc_pitch = 23456U;
    value.acc_heading = 345678U;
    return value;
}

// Builds the payload of make_message() byte by byte at the spec offsets.
void build_payload(uint8_t* p, uint8_t reserved_fill)
{
    for (castle::size_type i = 0U; i < length; ++i)
    {
        p[i] = 0U;
    }
    castle::write_le32(&p[0U], 345600123U);
    p[4U] = 1U;
    castle::write_le32s(&p[8U], -1234567);
    castle::write_le32s(&p[12U], 2345678);
    castle::write_le32s(&p[16U], 34567890);
    castle::write_le32(&p[20U], 12345U);
    castle::write_le32(&p[24U], 23456U);
    castle::write_le32(&p[28U], 345678U);
    for (castle::size_type i = 5U; i < 8U; ++i)
    {
        p[i] = reserved_fill;
    }
}

void expect_sample_fields(CASTLE_CONST message& value)
{
    EXPECT_EQ(value.i_tow, 345600123U);
    EXPECT_EQ(value.version, 1U);
    EXPECT_EQ(value.roll, -1234567);
    EXPECT_EQ(value.pitch, 2345678);
    EXPECT_EQ(value.heading, 34567890);
    EXPECT_EQ(value.acc_roll, 12345U);
    EXPECT_EQ(value.acc_pitch, 23456U);
    EXPECT_EQ(value.acc_heading, 345678U);
}

TEST(UbxMessageNavAtt, IdentityConstants)
{
    EXPECT_EQ(message::msg_class, expected_class);
    EXPECT_EQ(message::msg_id, expected_id);
    EXPECT_EQ(message::payload_length, length);
}

TEST(UbxMessageNavAtt, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[length];
    build_payload(payload, 0xA5U);
    message out;
    EXPECT_TRUE(message::matches(view_of(payload, length)));
    ASSERT_TRUE(message::decode(view_of(payload, length), out));
    expect_sample_fields(out);
}

TEST(UbxMessageNavAtt, EncodeMatchesSpecBytesAndRoundTrips)
{
    uint8_t expected[length];
    build_payload(expected, 0U);

    uint8_t frame[frame_size(length)];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(make_message(), frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(length));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], expected_class);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], expected_id);
    for (castle::size_type i = 0U; i < length; ++i)
    {
        EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + i], expected[i]) << "offset " << i;
    }

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    message out;
    ASSERT_TRUE(message::decode(raw, out));
    expect_sample_fields(out);
}

TEST(UbxMessageNavAtt, RejectsWrongClassOrId)
{
    uint8_t payload[length];
    build_payload(payload, 0U);
    message out = make_message();

    EXPECT_FALSE(message::matches(view_of(payload, length, other_class, expected_id)));
    EXPECT_FALSE(message::decode(view_of(payload, length, other_class, expected_id), out));
    EXPECT_EQ(out.i_tow, 0U);

    out = make_message();
    EXPECT_FALSE(message::matches(view_of(payload, length, expected_class, UBX_ID_NAV_PVT)));
    EXPECT_FALSE(message::decode(view_of(payload, length, expected_class, UBX_ID_NAV_PVT), out));
    EXPECT_EQ(out.i_tow, 0U);
}

TEST(UbxMessageNavAtt, RejectsShortAndLongPayloads)
{
    uint8_t payload[length + 1U];
    build_payload(payload, 0U);
    payload[length] = 0U;
    message out = make_message();

    EXPECT_FALSE(message::decode(view_of(payload, length - 1U), out));
    EXPECT_EQ(out.i_tow, 0U);

    out = make_message();
    EXPECT_FALSE(message::decode(view_of(payload, length + 1U), out));
    EXPECT_EQ(out.i_tow, 0U);

    EXPECT_FALSE(message::decode(view_of(payload, 0U), out));
}

TEST(UbxMessageNavAtt, EncodeIntoTooSmallBufferIsFull)
{
    uint8_t frame[frame_size(length)];
    castle::size_type written = 123U;
    EXPECT_EQ(encode_message(make_message(), frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(make_message(), frame, 4U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageNavAtt, ScaledHelpers)
{
    CASTLE_CONST message value = make_message();
    EXPECT_DOUBLE_EQ(value.roll_deg(), -12.34567);
    EXPECT_DOUBLE_EQ(value.pitch_deg(), 23.45678);
    EXPECT_DOUBLE_EQ(value.heading_deg(), 345.6789);
    EXPECT_DOUBLE_EQ(value.acc_roll_deg(), 0.12345);
    EXPECT_DOUBLE_EQ(value.acc_pitch_deg(), 0.23456);
    EXPECT_DOUBLE_EQ(value.acc_heading_deg(), 3.45678);

    CASTLE_CONST message zero;
    EXPECT_DOUBLE_EQ(zero.roll_deg(), 0.0);
    EXPECT_DOUBLE_EQ(zero.acc_heading_deg(), 0.0);
}

} // namespace
