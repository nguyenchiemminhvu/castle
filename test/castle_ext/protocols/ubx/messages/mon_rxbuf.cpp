#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_rxbuf.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

TEST(UbxMessageMonRxbuf, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_rxbuf::msg_class, UBX_CLASS_MON);
    EXPECT_EQ(mon_rxbuf::msg_id, UBX_ID_MON_RXBUF);
    EXPECT_EQ(mon_rxbuf::payload_length, 24U);
    EXPECT_EQ(mon_rxbuf::num_targets, 6U);
}

TEST(UbxMessageMonRxbuf, RoundTripPreservesEveryField)
{
    mon_rxbuf message{};
    for (castle::size_type i = 0U; i < mon_rxbuf::num_targets; ++i)
    {
        message.pending[i] = static_cast<uint16_t>(0x1201U + (i * 0x0101U));
        message.usage[i] = static_cast<uint8_t>(0x41U + i);
        message.peak_usage[i] = static_cast<uint8_t>(0x61U + i);
    }
    uint8_t frame[UBX_FRAME_OVERHEAD + 24U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    mon_rxbuf out{};
    ASSERT_TRUE(mon_rxbuf::decode(raw, out));
    for (castle::size_type i = 0U; i < mon_rxbuf::num_targets; ++i)
    {
        EXPECT_EQ(out.pending[i], message.pending[i]);
        EXPECT_EQ(out.usage[i], message.usage[i]);
        EXPECT_EQ(out.peak_usage[i], message.peak_usage[i]);
    }
}

TEST(UbxMessageMonRxbuf, HandBuiltPayloadMatchesSpecOffsets)
{
    uint8_t payload[24U] = {};
    for (castle::size_type i = 0U; i < 6U; ++i)
    {
        castle::write_le16(&payload[i * 2U], static_cast<uint16_t>(0xA1B0U + i));
        payload[12U + i] = static_cast<uint8_t>(0xC0U + i);
        payload[18U + i] = static_cast<uint8_t>(0xD0U + i);
    }
    mon_rxbuf out{};
    ASSERT_TRUE(mon_rxbuf::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_RXBUF, payload, 24U), out));
    for (castle::size_type i = 0U; i < 6U; ++i)
    {
        EXPECT_EQ(out.pending[i], 0xA1B0U + i);
        EXPECT_EQ(out.usage[i], 0xC0U + i);
        EXPECT_EQ(out.peak_usage[i], 0xD0U + i);
    }
}

TEST(UbxMessageMonRxbuf, RejectsWrongClassIdAndLength)
{
    uint8_t payload[25U] = {};
    payload[12U] = 7U;
    mon_rxbuf out{};
    EXPECT_FALSE(mon_rxbuf::matches(make_view(UBX_CLASS_NAV, UBX_ID_MON_RXBUF, payload, 24U)));
    EXPECT_FALSE(mon_rxbuf::decode(make_view(UBX_CLASS_NAV, UBX_ID_MON_RXBUF, payload, 24U), out));
    EXPECT_FALSE(mon_rxbuf::matches(make_view(UBX_CLASS_MON, UBX_ID_MON_TXBUF, payload, 24U)));
    EXPECT_FALSE(mon_rxbuf::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_TXBUF, payload, 24U), out));
    EXPECT_FALSE(mon_rxbuf::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_RXBUF, payload, 23U), out));
    EXPECT_FALSE(mon_rxbuf::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_RXBUF, payload, 25U), out));
    EXPECT_EQ(out.usage[0U], 0U);
    EXPECT_TRUE(mon_rxbuf::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_RXBUF, payload, 24U), out));
    EXPECT_EQ(out.usage[0U], 7U);
}

TEST(UbxMessageMonRxbuf, RejectsDeclaredLengthLongerThanView)
{
    uint8_t payload[10U] = {};
    payload[0U] = 1U;
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_RXBUF, 24U}, array_view<CASTLE_CONST uint8_t>(payload, 10U), checksum{}};
    mon_rxbuf out{};
    EXPECT_FALSE(mon_rxbuf::decode(raw, out));
    EXPECT_EQ(out.pending[0U], 0U);
}

TEST(UbxMessageMonRxbuf, EncodeIntoTooSmallBufferReportsFull)
{
    mon_rxbuf message{};
    uint8_t frame[UBX_FRAME_OVERHEAD + 24U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
