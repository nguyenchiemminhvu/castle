#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/ack.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageAckMirror, DecodesAckAndNakVariantsAndCompatibilityType)
{
    uint8_t payload[2U] = {UBX_CLASS_CFG, UBX_ID_CFG_VALGET};
    message_view raw{{UBX_CLASS_ACK, UBX_ID_ACK_ACK, 2U}, array_view<CASTLE_CONST uint8_t>(payload, 2U), checksum{}};

    ack_ack acked;
    ASSERT_TRUE(ack_ack::decode(raw, acked));
    EXPECT_EQ(acked.cls_id, UBX_CLASS_CFG);
    EXPECT_EQ(acked.msg_id_, UBX_ID_CFG_VALGET);
    EXPECT_TRUE(ack_ack::matches(raw));

    ack decoded;
    ASSERT_TRUE(ack::decode(raw, decoded));
    EXPECT_TRUE(decoded.accepted);

    raw.header.msg_id = UBX_ID_ACK_NAK;
    EXPECT_TRUE(ack::matches(raw));
    ASSERT_TRUE(ack::decode(raw, decoded));
    EXPECT_FALSE(decoded.accepted);
    EXPECT_TRUE(ack_nak::matches(raw));
}

TEST(UbxMessageAckMirror, RejectsWrongPayloadLengthAndUnrelatedMessage)
{
    uint8_t payload[1U] = {0U};
    message_view raw{{UBX_CLASS_ACK, UBX_ID_ACK_ACK, 1U}, array_view<CASTLE_CONST uint8_t>(payload, 1U), checksum{}};
    ack_ack message;
    EXPECT_FALSE(ack_ack::decode(raw, message));

    raw.header.payload_length = 2U;
    raw.header.msg_id = 0x7FU;
    EXPECT_FALSE(ack::matches(raw));
}

} // namespace
