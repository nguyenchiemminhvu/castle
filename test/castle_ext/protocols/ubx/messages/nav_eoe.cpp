#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_eoe.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_eoe;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

TEST(UbxMessageNavEoe, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_EOE);
    EXPECT_EQ(message::payload_length, 4U);
}

TEST(UbxMessageNavEoe, RoundTripPreservesEveryField)
{
    message original{};
    original.i_tow = 0xDEADBEEFU;
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
    EXPECT_EQ(castle::read_le32(&frame[UBX_FRAME_INDEX_PAYLOAD]), 0xDEADBEEFU);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(message::matches(raw));

    message decoded{};
    ASSERT_TRUE(message::decode(raw, decoded));
    EXPECT_EQ(decoded.i_tow, 0xDEADBEEFU);
}

TEST(UbxMessageNavEoe, DecodesFieldsAtSpecOffsets)
{
    const uint8_t payload[message::payload_length] = {0x04U, 0x03U, 0x02U, 0x01U};
    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 0x01020304U);
}

TEST(UbxMessageNavEoe, RejectsWrongClassOrId)
{
    const uint8_t payload[message::payload_length] = {1U, 2U, 3U, 4U};
    message_view raw = view_of(payload, sizeof(payload));
    message decoded{};
    decoded.i_tow = 5U;

    raw.header.msg_class = UBX_CLASS_NAV2;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    EXPECT_EQ(decoded.i_tow, 0U);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_NAV_PVT;
    EXPECT_FALSE(message::matches(raw));
    decoded.i_tow = 5U;
    EXPECT_FALSE(message::decode(raw, decoded));
    EXPECT_EQ(decoded.i_tow, 0U);
}

TEST(UbxMessageNavEoe, RejectsShortAndLongPayloads)
{
    const uint8_t payload[message::payload_length + 1U] = {1U, 2U, 3U, 4U, 5U};
    message decoded{};

    decoded.i_tow = 5U;
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    EXPECT_EQ(decoded.i_tow, 0U);

    decoded.i_tow = 5U;
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length + 1U), decoded));
    EXPECT_EQ(decoded.i_tow, 0U);
}

TEST(UbxMessageNavEoe, EncodeIntoTooSmallBufferReportsFull)
{
    const message value{};
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
