#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_rxr.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(CASTLE_CONST uint8_t* payload, uint16_t length, uint8_t msg_class = UBX_CLASS_MON,
                     uint8_t msg_id = UBX_ID_MON_RXR)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

TEST(UbxMessageMonRxr, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_rxr::msg_class, 0x0AU);
    EXPECT_EQ(mon_rxr::msg_id, 0x21U);
    EXPECT_EQ(mon_rxr::payload_length, 1U);
}

TEST(UbxMessageMonRxr, RoundTripThroughFrame)
{
    mon_rxr message{};
    message.flags = 0x81U;

    uint8_t frame[16U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, frame_size(1U));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD], 0x81U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    ASSERT_TRUE(mon_rxr::matches(raw));

    mon_rxr decoded{};
    ASSERT_TRUE(mon_rxr::decode(raw, decoded));
    EXPECT_EQ(decoded.flags, 0x81U);
    EXPECT_TRUE(decoded.awake());
}

TEST(UbxMessageMonRxr, AwakeBitSetAndClear)
{
    uint8_t payload[1U] = {0x01U};
    mon_rxr decoded{};
    ASSERT_TRUE(mon_rxr::decode(view_of(payload, 1U), decoded));
    EXPECT_EQ(decoded.flags, 0x01U);
    EXPECT_TRUE(decoded.awake());

    payload[0U] = 0xFEU;
    ASSERT_TRUE(mon_rxr::decode(view_of(payload, 1U), decoded));
    EXPECT_EQ(decoded.flags, 0xFEU);
    EXPECT_FALSE(decoded.awake());

    EXPECT_EQ(mon_rxr::FLAGS_AWAKE, 0x01U);
}

TEST(UbxMessageMonRxr, RejectsWrongClassOrId)
{
    uint8_t payload[1U] = {0x01U};
    mon_rxr decoded{};
    decoded.flags = 0x01U;

    EXPECT_FALSE(mon_rxr::matches(view_of(payload, 1U, UBX_CLASS_NAV, UBX_ID_MON_RXR)));
    EXPECT_FALSE(mon_rxr::decode(view_of(payload, 1U, UBX_CLASS_NAV, UBX_ID_MON_RXR), decoded));
    EXPECT_EQ(decoded.flags, 0U);

    EXPECT_FALSE(mon_rxr::matches(view_of(payload, 1U, UBX_CLASS_MON, UBX_ID_MON_HW)));
    EXPECT_FALSE(mon_rxr::decode(view_of(payload, 1U, UBX_CLASS_MON, UBX_ID_MON_HW), decoded));
}

TEST(UbxMessageMonRxr, RejectsWrongLength)
{
    uint8_t payload[2U] = {0x01U, 0x00U};
    mon_rxr decoded{};

    EXPECT_FALSE(mon_rxr::decode(view_of(payload, 2U), decoded));
    EXPECT_EQ(decoded.flags, 0U);

    message_view empty{{UBX_CLASS_MON, UBX_ID_MON_RXR, 0U}, array_view<CASTLE_CONST uint8_t>(), checksum{}};
    EXPECT_TRUE(mon_rxr::matches(empty));
    EXPECT_FALSE(mon_rxr::decode(empty, decoded));
}

TEST(UbxMessageMonRxr, EncodeIntoTooSmallBufferReportsFull)
{
    mon_rxr message{};
    uint8_t frame[UBX_FRAME_OVERHEAD] = {};
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
