#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/ack.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(uint8_t msg_id, CASTLE_CONST uint8_t* payload, castle::size_type length)
{
    return message_view{{UBX_CLASS_ACK, msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

TEST(UbxMessageAck, ConstantsMatchSpec)
{
    EXPECT_EQ(ack_ack::msg_class, UBX_CLASS_ACK);
    EXPECT_EQ(ack_ack::msg_id, 0x01U);
    EXPECT_EQ(ack_ack::payload_length, 2U);
    EXPECT_EQ(ack_nak::msg_class, UBX_CLASS_ACK);
    EXPECT_EQ(ack_nak::msg_id, 0x00U);
    EXPECT_EQ(ack_nak::payload_length, 2U);
    EXPECT_EQ(ack::msg_class, UBX_CLASS_ACK);
    EXPECT_EQ(ack::payload_length, 2U);
}

TEST(UbxMessageAck, RoundTripPreservesEveryField)
{
    ack_nak original{};
    original.cls_id = 0xA6U;
    original.acked_msg_id = 0x5BU;

    uint8_t frame[UBX_FRAME_OVERHEAD + 2U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_ACK);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_ACK_NAK);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 2U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD], 0xA6U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 1U], 0x5BU);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(ack_nak::matches(raw));
    EXPECT_FALSE(ack_ack::matches(raw));

    ack_nak decoded{};
    ASSERT_TRUE(ack_nak::decode(raw, decoded));
    EXPECT_EQ(decoded.cls_id, 0xA6U);
    EXPECT_EQ(decoded.acked_msg_id, 0x5BU);
}

TEST(UbxMessageAck, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[2U] = {UBX_CLASS_CFG, UBX_ID_CFG_VALSET};
    ack_ack decoded{};
    ASSERT_TRUE(ack_ack::decode(view_of(UBX_ID_ACK_ACK, payload, 2U), decoded));
    EXPECT_EQ(decoded.cls_id, UBX_CLASS_CFG);
    EXPECT_EQ(decoded.acked_msg_id, UBX_ID_CFG_VALSET);
}

TEST(UbxMessageAck, TypedModelsRejectWrongIdClassAndLength)
{
    uint8_t payload[3U] = {0x06U, 0x8AU, 0x00U};
    ack_ack decoded{};

    EXPECT_FALSE(ack_ack::matches(view_of(UBX_ID_ACK_NAK, payload, 2U)));
    decoded.cls_id = 1U;
    EXPECT_FALSE(ack_ack::decode(view_of(UBX_ID_ACK_NAK, payload, 2U), decoded));
    EXPECT_EQ(decoded.cls_id, 0U);
    EXPECT_EQ(decoded.acked_msg_id, 0U);

    message_view other_class = view_of(UBX_ID_ACK_ACK, payload, 2U);
    other_class.header.msg_class = UBX_CLASS_CFG;
    EXPECT_FALSE(ack_ack::matches(other_class));
    EXPECT_FALSE(ack_ack::decode(other_class, decoded));

    EXPECT_FALSE(ack_ack::decode(view_of(UBX_ID_ACK_ACK, payload, 1U), decoded));
    EXPECT_FALSE(ack_ack::decode(view_of(UBX_ID_ACK_ACK, payload, 3U), decoded));
    EXPECT_FALSE(ack_ack::decode(view_of(UBX_ID_ACK_ACK, nullptr, 0U), decoded));
    EXPECT_EQ(decoded.cls_id, 0U);
}

TEST(UbxMessageAck, AggregateDistinguishesAckFromNak)
{
    uint8_t payload[2U] = {0x13U, 0x40U};
    ack decoded{};

    ASSERT_TRUE(ack::matches(view_of(UBX_ID_ACK_ACK, payload, 2U)));
    ASSERT_TRUE(ack::decode(view_of(UBX_ID_ACK_ACK, payload, 2U), decoded));
    EXPECT_TRUE(decoded.accepted);
    EXPECT_EQ(decoded.acked_class, 0x13U);
    EXPECT_EQ(decoded.acked_id, 0x40U);

    ASSERT_TRUE(ack::matches(view_of(UBX_ID_ACK_NAK, payload, 2U)));
    ASSERT_TRUE(ack::decode(view_of(UBX_ID_ACK_NAK, payload, 2U), decoded));
    EXPECT_FALSE(decoded.accepted);
    EXPECT_EQ(decoded.acked_class, 0x13U);
    EXPECT_EQ(decoded.acked_id, 0x40U);
}

TEST(UbxMessageAck, AggregateRejectsOtherMessagesAndLengths)
{
    uint8_t payload[3U] = {0x13U, 0x40U, 0x00U};
    ack decoded{};

    EXPECT_FALSE(ack::matches(view_of(0x02U, payload, 2U)));
    decoded.accepted = true;
    decoded.acked_class = 1U;
    EXPECT_FALSE(ack::decode(view_of(0x02U, payload, 2U), decoded));
    EXPECT_FALSE(decoded.accepted);
    EXPECT_EQ(decoded.acked_class, 0U);

    message_view other_class = view_of(UBX_ID_ACK_ACK, payload, 2U);
    other_class.header.msg_class = UBX_CLASS_INF;
    EXPECT_FALSE(ack::matches(other_class));
    EXPECT_FALSE(ack::decode(other_class, decoded));

    decoded.accepted = true;
    EXPECT_FALSE(ack::decode(view_of(UBX_ID_ACK_ACK, payload, 1U), decoded));
    EXPECT_FALSE(decoded.accepted);
    EXPECT_FALSE(ack::decode(view_of(UBX_ID_ACK_ACK, payload, 3U), decoded));
    EXPECT_FALSE(ack::decode(view_of(UBX_ID_ACK_NAK, nullptr, 0U), decoded));
}

TEST(UbxMessageAck, AggregateRejectsNullPayloadWithDeclaredLength)
{
    // A length-2 header over a null view must not be read.
    ack decoded{};
    EXPECT_FALSE(ack::decode(view_of(UBX_ID_ACK_ACK, nullptr, 2U), decoded));
    EXPECT_FALSE(decoded.accepted);
}

TEST(UbxMessageAck, EncodeIntoTooSmallBufferReportsFull)
{
    ack_ack value{};
    value.cls_id = 1U;
    uint8_t frame[UBX_FRAME_OVERHEAD + 2U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
