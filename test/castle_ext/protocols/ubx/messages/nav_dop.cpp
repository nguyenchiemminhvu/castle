#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_dop.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_dop;
CASTLE_CONSTEXPR uint8_t expected_class = UBX_CLASS_NAV;
CASTLE_CONSTEXPR uint8_t expected_id = UBX_ID_NAV_DOP;
CASTLE_CONSTEXPR uint8_t other_class = UBX_CLASS_NAV2;
CASTLE_CONSTEXPR castle::size_type length = 18U;

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
    value.g_dop = 251U;
    value.p_dop = 156U;
    value.t_dop = 98U;
    value.v_dop = 187U;
    value.h_dop = 123U;
    value.n_dop = 77U;
    value.e_dop = 64U;
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
    castle::write_le16(&p[4U], 251U);
    castle::write_le16(&p[6U], 156U);
    castle::write_le16(&p[8U], 98U);
    castle::write_le16(&p[10U], 187U);
    castle::write_le16(&p[12U], 123U);
    castle::write_le16(&p[14U], 77U);
    castle::write_le16(&p[16U], 64U);
    static_cast<void>(reserved_fill);
}

void expect_sample_fields(CASTLE_CONST message& value)
{
    EXPECT_EQ(value.i_tow, 345600123U);
    EXPECT_EQ(value.g_dop, 251U);
    EXPECT_EQ(value.p_dop, 156U);
    EXPECT_EQ(value.t_dop, 98U);
    EXPECT_EQ(value.v_dop, 187U);
    EXPECT_EQ(value.h_dop, 123U);
    EXPECT_EQ(value.n_dop, 77U);
    EXPECT_EQ(value.e_dop, 64U);
}

TEST(UbxMessageNavDop, IdentityConstants)
{
    EXPECT_EQ(message::msg_class, expected_class);
    EXPECT_EQ(message::msg_id, expected_id);
    EXPECT_EQ(message::payload_length, length);
}

TEST(UbxMessageNavDop, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[length];
    build_payload(payload, 0xA5U);
    message out;
    EXPECT_TRUE(message::matches(view_of(payload, length)));
    ASSERT_TRUE(message::decode(view_of(payload, length), out));
    expect_sample_fields(out);
}

TEST(UbxMessageNavDop, EncodeMatchesSpecBytesAndRoundTrips)
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

TEST(UbxMessageNavDop, RejectsWrongClassOrId)
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

TEST(UbxMessageNavDop, RejectsShortAndLongPayloads)
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

TEST(UbxMessageNavDop, EncodeIntoTooSmallBufferIsFull)
{
    uint8_t frame[frame_size(length)];
    castle::size_type written = 123U;
    EXPECT_EQ(encode_message(make_message(), frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(make_message(), frame, 4U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageNavDop, ScaledHelpers)
{
    CASTLE_CONST message value = make_message();
    EXPECT_DOUBLE_EQ(value.gdop_value(), 2.51);
    EXPECT_DOUBLE_EQ(value.pdop_value(), 1.56);
    EXPECT_DOUBLE_EQ(value.tdop_value(), 0.98);
    EXPECT_DOUBLE_EQ(value.vdop_value(), 1.87);
    EXPECT_DOUBLE_EQ(value.hdop_value(), 1.23);
    EXPECT_DOUBLE_EQ(value.ndop_value(), 0.77);
    EXPECT_DOUBLE_EQ(value.edop_value(), 0.64);

    message limits;
    limits.g_dop = 65535U;
    EXPECT_DOUBLE_EQ(limits.gdop_value(), 655.35);
    EXPECT_DOUBLE_EQ(limits.pdop_value(), 0.0);
}

} // namespace
