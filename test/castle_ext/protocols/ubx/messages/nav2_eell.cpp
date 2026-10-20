#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav2_eell.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav2_eell;
CASTLE_CONSTEXPR uint8_t expected_class = UBX_CLASS_NAV2;
CASTLE_CONSTEXPR uint8_t expected_id = UBX_ID_NAV2_EELL;
CASTLE_CONSTEXPR uint8_t other_class = UBX_CLASS_NAV;
CASTLE_CONSTEXPR castle::size_type length = 16U;

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
    value.version = 2U;
    value.err_orient = 12345U;
    value.err_maj = 1234567U;
    value.err_min = 987654U;
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
    p[4U] = 2U;
    castle::write_le16(&p[6U], 12345U);
    castle::write_le32(&p[8U], 1234567U);
    castle::write_le32(&p[12U], 987654U);
    p[5U] = reserved_fill;
}

void expect_sample_fields(CASTLE_CONST message& value)
{
    EXPECT_EQ(value.i_tow, 345600123U);
    EXPECT_EQ(value.version, 2U);
    EXPECT_EQ(value.err_orient, 12345U);
    EXPECT_EQ(value.err_maj, 1234567U);
    EXPECT_EQ(value.err_min, 987654U);
}

TEST(UbxMessageNav2Eell, IdentityConstants)
{
    EXPECT_EQ(message::msg_class, expected_class);
    EXPECT_EQ(message::msg_id, expected_id);
    EXPECT_EQ(message::payload_length, length);
}

TEST(UbxMessageNav2Eell, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[length];
    build_payload(payload, 0xA5U);
    message out;
    EXPECT_TRUE(message::matches(view_of(payload, length)));
    ASSERT_TRUE(message::decode(view_of(payload, length), out));
    expect_sample_fields(out);
}

TEST(UbxMessageNav2Eell, EncodeMatchesSpecBytesAndRoundTrips)
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

TEST(UbxMessageNav2Eell, RejectsWrongClassOrId)
{
    uint8_t payload[length];
    build_payload(payload, 0U);
    message out = make_message();

    EXPECT_FALSE(message::matches(view_of(payload, length, other_class, expected_id)));
    EXPECT_FALSE(message::decode(view_of(payload, length, other_class, expected_id), out));
    EXPECT_EQ(out.i_tow, 0U);

    out = make_message();
    EXPECT_FALSE(message::matches(view_of(payload, length, expected_class, UBX_ID_NAV2_PVT)));
    EXPECT_FALSE(message::decode(view_of(payload, length, expected_class, UBX_ID_NAV2_PVT), out));
    EXPECT_EQ(out.i_tow, 0U);
}

TEST(UbxMessageNav2Eell, RejectsShortAndLongPayloads)
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

TEST(UbxMessageNav2Eell, EncodeIntoTooSmallBufferIsFull)
{
    uint8_t frame[frame_size(length)];
    castle::size_type written = 123U;
    EXPECT_EQ(encode_message(make_message(), frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(make_message(), frame, 4U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageNav2Eell, AxesUseFullU4Width)
{
    // errEllipseMajor/Minor are U4: values above 16 bits must survive both directions.
    message value;
    value.err_maj = 0xFEDCBA98U;
    value.err_min = 0x01234567U;
    value.err_orient = 0xFFFFU;

    uint8_t frame[frame_size(length)];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(value, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_PAYLOAD + 6U]), 0xFFFFU);
    EXPECT_EQ(castle::read_le32(&frame[UBX_FRAME_INDEX_PAYLOAD + 8U]), 0xFEDCBA98U);
    EXPECT_EQ(castle::read_le32(&frame[UBX_FRAME_INDEX_PAYLOAD + 12U]), 0x01234567U);

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    message out;
    ASSERT_TRUE(message::decode(raw, out));
    EXPECT_EQ(out.err_maj, 0xFEDCBA98U);
    EXPECT_EQ(out.err_min, 0x01234567U);
    EXPECT_EQ(out.err_orient, 0xFFFFU);
}

TEST(UbxMessageNav2Eell, ScaledHelpers)
{
    CASTLE_CONST message value = make_message();
    EXPECT_DOUBLE_EQ(value.err_orient_deg(), 123.45);
    EXPECT_DOUBLE_EQ(value.err_maj_m(), 1234.567);
    EXPECT_DOUBLE_EQ(value.err_min_m(), 987.654);

    CASTLE_CONST message zero;
    EXPECT_DOUBLE_EQ(zero.err_orient_deg(), 0.0);
    EXPECT_DOUBLE_EQ(zero.err_maj_m(), 0.0);
    EXPECT_DOUBLE_EQ(zero.err_min_m(), 0.0);
}

} // namespace
