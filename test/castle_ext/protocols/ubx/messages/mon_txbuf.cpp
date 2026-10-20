#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_txbuf.hpp"
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

TEST(UbxMessageMonTxbuf, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_txbuf::msg_class, UBX_CLASS_MON);
    EXPECT_EQ(mon_txbuf::msg_id, UBX_ID_MON_TXBUF);
    EXPECT_EQ(mon_txbuf::payload_length, 28U);
    EXPECT_EQ(mon_txbuf::num_targets, 6U);
    EXPECT_EQ(mon_txbuf::ERRORS_LIMIT, 0x3FU);
    EXPECT_EQ(mon_txbuf::ERRORS_MEM, 0x40U);
    EXPECT_EQ(mon_txbuf::ERRORS_ALLOC, 0x80U);
}

TEST(UbxMessageMonTxbuf, RoundTripPreservesEveryField)
{
    mon_txbuf message{};
    for (castle::size_type i = 0U; i < mon_txbuf::num_targets; ++i)
    {
        message.pending[i] = static_cast<uint16_t>(0x1100U + i);
        message.usage[i] = static_cast<uint8_t>(0x21U + i);
        message.peak_usage[i] = static_cast<uint8_t>(0x31U + i);
    }
    message.t_used = 0x41U;
    message.t_peak = 0x51U;
    message.errors = 0xA5U;

    uint8_t frame[UBX_FRAME_OVERHEAD + 28U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    mon_txbuf out{};
    ASSERT_TRUE(mon_txbuf::decode(raw, out));
    for (castle::size_type i = 0U; i < mon_txbuf::num_targets; ++i)
    {
        EXPECT_EQ(out.pending[i], message.pending[i]);
        EXPECT_EQ(out.usage[i], message.usage[i]);
        EXPECT_EQ(out.peak_usage[i], message.peak_usage[i]);
    }
    EXPECT_EQ(out.t_used, 0x41U);
    EXPECT_EQ(out.t_peak, 0x51U);
    EXPECT_EQ(out.errors, 0xA5U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 27U], 0U); // reserved0
}

TEST(UbxMessageMonTxbuf, HandBuiltPayloadMatchesSpecOffsets)
{
    uint8_t payload[28U] = {};
    for (castle::size_type i = 0U; i < 6U; ++i)
    {
        castle::write_le16(&payload[i * 2U], static_cast<uint16_t>(0xA000U + i));
        payload[12U + i] = static_cast<uint8_t>(0xB0U + i);
        payload[18U + i] = static_cast<uint8_t>(0xC0U + i);
    }
    payload[24U] = 0xD1U;
    payload[25U] = 0xD2U;
    payload[26U] = 0x7FU;
    payload[27U] = 0xEEU; // reserved0 ignored
    mon_txbuf out{};
    ASSERT_TRUE(mon_txbuf::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_TXBUF, payload, 28U), out));
    EXPECT_EQ(out.pending[0U], 0xA000U);
    EXPECT_EQ(out.pending[5U], 0xA005U);
    EXPECT_EQ(out.usage[0U], 0xB0U);
    EXPECT_EQ(out.usage[5U], 0xB5U);
    EXPECT_EQ(out.peak_usage[0U], 0xC0U);
    EXPECT_EQ(out.peak_usage[5U], 0xC5U);
    EXPECT_EQ(out.t_used, 0xD1U);
    EXPECT_EQ(out.t_peak, 0xD2U);
    EXPECT_EQ(out.errors, 0x7FU);
}

TEST(UbxMessageMonTxbuf, ErrorAccessorsFollowBitLayout)
{
    mon_txbuf message{};
    EXPECT_EQ(message.limit(), 0U);
    EXPECT_FALSE(message.mem_error());
    EXPECT_FALSE(message.alloc_error());
    for (castle::size_type target = 0U; target < mon_txbuf::num_targets; ++target)
    {
        EXPECT_FALSE(message.limit_reached(target));
    }

    message.errors = 0x3FU;
    EXPECT_EQ(message.limit(), 0x3FU);
    EXPECT_FALSE(message.mem_error());
    EXPECT_FALSE(message.alloc_error());
    for (castle::size_type target = 0U; target < mon_txbuf::num_targets; ++target)
    {
        EXPECT_TRUE(message.limit_reached(target));
    }
    EXPECT_FALSE(message.limit_reached(6U));
    EXPECT_FALSE(message.limit_reached(31U));

    message.errors = 0x40U;
    EXPECT_EQ(message.limit(), 0U);
    EXPECT_TRUE(message.mem_error());
    EXPECT_FALSE(message.alloc_error());

    message.errors = 0x80U;
    EXPECT_EQ(message.limit(), 0U);
    EXPECT_FALSE(message.mem_error());
    EXPECT_TRUE(message.alloc_error());

    message.errors = 0xC8U; // alloc + mem + target 3
    EXPECT_EQ(message.limit(), 0x08U);
    EXPECT_TRUE(message.limit_reached(3U));
    EXPECT_FALSE(message.limit_reached(2U));
    EXPECT_FALSE(message.limit_reached(4U));
    EXPECT_TRUE(message.mem_error());
    EXPECT_TRUE(message.alloc_error());
    // Bits 6/7 never alias a target-limit bit.
    EXPECT_FALSE(message.limit_reached(6U));
    EXPECT_FALSE(message.limit_reached(7U));
}

TEST(UbxMessageMonTxbuf, RejectsWrongClassIdAndLength)
{
    uint8_t payload[29U] = {};
    payload[24U] = 9U;
    mon_txbuf out{};
    EXPECT_FALSE(mon_txbuf::matches(make_view(UBX_CLASS_NAV, UBX_ID_MON_TXBUF, payload, 28U)));
    EXPECT_FALSE(mon_txbuf::decode(make_view(UBX_CLASS_NAV, UBX_ID_MON_TXBUF, payload, 28U), out));
    EXPECT_FALSE(mon_txbuf::matches(make_view(UBX_CLASS_MON, UBX_ID_MON_RXBUF, payload, 28U)));
    EXPECT_FALSE(mon_txbuf::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_RXBUF, payload, 28U), out));
    EXPECT_FALSE(mon_txbuf::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_TXBUF, payload, 27U), out));
    EXPECT_FALSE(mon_txbuf::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_TXBUF, payload, 29U), out));
    EXPECT_EQ(out.t_used, 0U);
    EXPECT_TRUE(mon_txbuf::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_TXBUF, payload, 28U), out));
    EXPECT_EQ(out.t_used, 9U);
}

TEST(UbxMessageMonTxbuf, RejectsDeclaredLengthLongerThanView)
{
    uint8_t payload[20U] = {};
    payload[0U] = 1U;
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_TXBUF, 28U}, array_view<CASTLE_CONST uint8_t>(payload, 20U), checksum{}};
    mon_txbuf out{};
    EXPECT_FALSE(mon_txbuf::decode(raw, out));
    EXPECT_EQ(out.pending[0U], 0U);
}

TEST(UbxMessageMonTxbuf, EncodeIntoTooSmallBufferReportsFull)
{
    mon_txbuf message{};
    uint8_t frame[UBX_FRAME_OVERHEAD + 28U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
