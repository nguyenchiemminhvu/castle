#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_msgpp.hpp"
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

TEST(UbxMessageMonMsgpp, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_msgpp::msg_class, UBX_CLASS_MON);
    EXPECT_EQ(mon_msgpp::msg_id, UBX_ID_MON_MSGPP);
    EXPECT_EQ(mon_msgpp::payload_length, 120U);
    EXPECT_EQ(mon_msgpp::num_ports, 6U);
    EXPECT_EQ(mon_msgpp::num_protocols, 8U);
}

TEST(UbxMessageMonMsgpp, RoundTripPreservesEveryField)
{
    mon_msgpp message{};
    for (castle::size_type port = 0U; port < mon_msgpp::num_ports; ++port)
    {
        for (castle::size_type protocol = 0U; protocol < mon_msgpp::num_protocols; ++protocol)
        {
            message.msg[port][protocol] = static_cast<uint16_t>(0x1000U + (port << 4U) + protocol + 1U);
        }
        message.skipped[port] = 0xA0B0C0D0U + static_cast<uint32_t>(port);
    }
    uint8_t frame[UBX_FRAME_OVERHEAD + 120U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    mon_msgpp out{};
    ASSERT_TRUE(mon_msgpp::decode(raw, out));
    for (castle::size_type port = 0U; port < mon_msgpp::num_ports; ++port)
    {
        for (castle::size_type protocol = 0U; protocol < mon_msgpp::num_protocols; ++protocol)
        {
            EXPECT_EQ(out.msg[port][protocol], message.msg[port][protocol]);
        }
        EXPECT_EQ(out.skipped[port], message.skipped[port]);
    }
}

TEST(UbxMessageMonMsgpp, HandBuiltPayloadMatchesSpecOffsets)
{
    uint8_t payload[120U] = {};
    // msgN at offset (N-1)*16, element k at +2k; skipped at 96 + 4p.
    for (castle::size_type port = 0U; port < 6U; ++port)
    {
        for (castle::size_type k = 0U; k < 8U; ++k)
        {
            castle::write_le16(&payload[(port * 16U) + (k * 2U)], static_cast<uint16_t>((port * 1000U) + k));
        }
        castle::write_le32(&payload[96U + (port * 4U)], 0x01020304U * static_cast<uint32_t>(port + 1U));
    }
    mon_msgpp out{};
    ASSERT_TRUE(mon_msgpp::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_MSGPP, payload, 120U), out));
    EXPECT_EQ(out.msg[0U][0U], 0U);
    EXPECT_EQ(out.msg[0U][7U], 7U);
    EXPECT_EQ(out.msg[1U][0U], 1000U);
    EXPECT_EQ(out.msg[3U][5U], 3005U);
    EXPECT_EQ(out.msg[5U][7U], 5007U);
    EXPECT_EQ(out.skipped[0U], 0x01020304U);
    EXPECT_EQ(out.skipped[5U], 0x01020304U * 6U);
}

TEST(UbxMessageMonMsgpp, RejectsWrongClassIdAndLength)
{
    uint8_t payload[121U] = {};
    payload[0U] = 3U;
    mon_msgpp out{};
    EXPECT_FALSE(mon_msgpp::matches(make_view(UBX_CLASS_NAV, UBX_ID_MON_MSGPP, payload, 120U)));
    EXPECT_FALSE(mon_msgpp::decode(make_view(UBX_CLASS_NAV, UBX_ID_MON_MSGPP, payload, 120U), out));
    EXPECT_FALSE(mon_msgpp::matches(make_view(UBX_CLASS_MON, UBX_ID_MON_IO, payload, 120U)));
    EXPECT_FALSE(mon_msgpp::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_IO, payload, 120U), out));
    EXPECT_FALSE(mon_msgpp::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_MSGPP, payload, 119U), out));
    EXPECT_FALSE(mon_msgpp::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_MSGPP, payload, 121U), out));
    EXPECT_EQ(out.msg[0U][0U], 0U);
    EXPECT_TRUE(mon_msgpp::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_MSGPP, payload, 120U), out));
    EXPECT_EQ(out.msg[0U][0U], 3U);
}

TEST(UbxMessageMonMsgpp, RejectsDeclaredLengthLongerThanView)
{
    uint8_t payload[100U] = {};
    payload[0U] = 1U;
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_MSGPP, 120U}, array_view<CASTLE_CONST uint8_t>(payload, 100U), checksum{}};
    mon_msgpp out{};
    EXPECT_FALSE(mon_msgpp::decode(raw, out));
    EXPECT_EQ(out.msg[0U][0U], 0U);
}

TEST(UbxMessageMonMsgpp, EncodeIntoTooSmallBufferReportsFull)
{
    mon_msgpp message{};
    uint8_t frame[UBX_FRAME_OVERHEAD + 120U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
