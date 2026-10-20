#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_io.hpp"
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

mon_io_port make_port(uint32_t seed)
{
    mon_io_port port{};
    port.rx_bytes = 0x11121314U + seed;
    port.tx_bytes = 0x21222324U + seed;
    port.parity_errs = static_cast<uint16_t>(0x3132U + seed);
    port.framing_errs = static_cast<uint16_t>(0x4142U + seed);
    port.overrun_errs = static_cast<uint16_t>(0x5152U + seed);
    port.break_cond = static_cast<uint16_t>(0x6162U + seed);
    return port;
}

void expect_port_eq(CASTLE_CONST mon_io_port& actual, CASTLE_CONST mon_io_port& expected)
{
    EXPECT_EQ(actual.rx_bytes, expected.rx_bytes);
    EXPECT_EQ(actual.tx_bytes, expected.tx_bytes);
    EXPECT_EQ(actual.parity_errs, expected.parity_errs);
    EXPECT_EQ(actual.framing_errs, expected.framing_errs);
    EXPECT_EQ(actual.overrun_errs, expected.overrun_errs);
    EXPECT_EQ(actual.break_cond, expected.break_cond);
}

TEST(UbxMessageMonIo, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_io<>::msg_class, UBX_CLASS_MON);
    EXPECT_EQ(mon_io<>::msg_id, UBX_ID_MON_IO);
    EXPECT_EQ(mon_io<>::header_length, 0U);
    EXPECT_EQ(mon_io<>::block_length, 20U);
    EXPECT_EQ(mon_io<>().ports.capacity(), 8U);
}

TEST(UbxMessageMonIo, RoundTripPreservesEveryField)
{
    mon_io<4U> message{};
    ASSERT_EQ(message.ports.push_back(make_port(0U)), castle::status::ok);
    ASSERT_EQ(message.ports.push_back(make_port(7U)), castle::status::ok);
    ASSERT_EQ(message.ports.push_back(make_port(9U)), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 60U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_EQ(raw.payload_length(), 60U);
    mon_io<4U> out{};
    ASSERT_TRUE(mon_io<4U>::decode(raw, out));
    ASSERT_EQ(out.ports.size(), 3U);
    expect_port_eq(out.ports[0U], make_port(0U));
    expect_port_eq(out.ports[1U], make_port(7U));
    expect_port_eq(out.ports[2U], make_port(9U));
}

TEST(UbxMessageMonIo, HandBuiltPayloadMatchesSpecOffsets)
{
    uint8_t payload[40U] = {};
    for (castle::size_type block = 0U; block < 2U; ++block)
    {
        uint8_t* p = &payload[block * 20U];
        castle::write_le32(&p[0U], 0xA0A1A2A3U + static_cast<uint32_t>(block));
        castle::write_le32(&p[4U], 0xB0B1B2B3U + static_cast<uint32_t>(block));
        castle::write_le16(&p[8U], static_cast<uint16_t>(0xC0C1U + block));
        castle::write_le16(&p[10U], static_cast<uint16_t>(0xD0D1U + block));
        castle::write_le16(&p[12U], static_cast<uint16_t>(0xE0E1U + block));
        castle::write_le16(&p[14U], static_cast<uint16_t>(0xF0F1U + block));
        p[16U] = 0xEEU; // reserved0 is ignored
        p[19U] = 0xEEU;
    }
    message_view raw = make_view(UBX_CLASS_MON, UBX_ID_MON_IO, payload, 40U);
    mon_io<> out{};
    ASSERT_TRUE(mon_io<>::decode(raw, out));
    ASSERT_EQ(out.ports.size(), 2U);
    EXPECT_EQ(out.ports[0U].rx_bytes, 0xA0A1A2A3U);
    EXPECT_EQ(out.ports[0U].tx_bytes, 0xB0B1B2B3U);
    EXPECT_EQ(out.ports[0U].parity_errs, 0xC0C1U);
    EXPECT_EQ(out.ports[0U].framing_errs, 0xD0D1U);
    EXPECT_EQ(out.ports[0U].overrun_errs, 0xE0E1U);
    EXPECT_EQ(out.ports[0U].break_cond, 0xF0F1U);
    EXPECT_EQ(out.ports[1U].rx_bytes, 0xA0A1A2A4U);
    EXPECT_EQ(out.ports[1U].tx_bytes, 0xB0B1B2B4U);
    EXPECT_EQ(out.ports[1U].parity_errs, 0xC0C2U);
    EXPECT_EQ(out.ports[1U].framing_errs, 0xD0D2U);
    EXPECT_EQ(out.ports[1U].overrun_errs, 0xE0E2U);
    EXPECT_EQ(out.ports[1U].break_cond, 0xF0F2U);

    // Encoding writes zeros into reserved0.
    uint8_t frame[UBX_FRAME_OVERHEAD + 40U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(out, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 16U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 19U], 0U);
    EXPECT_EQ(castle::read_le32(&frame[UBX_FRAME_INDEX_PAYLOAD + 20U]), 0xA0A1A2A4U);
}

TEST(UbxMessageMonIo, EmptyPayloadDecodesToZeroPorts)
{
    mon_io<> empty{};
    uint8_t frame[UBX_FRAME_OVERHEAD];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(empty, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_EQ(raw.payload.data(), nullptr);
    mon_io<> out{};
    ASSERT_EQ(out.ports.push_back(make_port(1U)), castle::status::ok);
    ASSERT_TRUE(mon_io<>::decode(raw, out));
    EXPECT_TRUE(out.ports.empty());
}

TEST(UbxMessageMonIo, RejectsWrongClassOrId)
{
    uint8_t payload[20U] = {};
    mon_io<> out{};
    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_MON_IO, payload, 20U);
    EXPECT_FALSE(mon_io<>::matches(raw));
    EXPECT_FALSE(mon_io<>::decode(raw, out));
    raw = make_view(UBX_CLASS_MON, UBX_ID_MON_VER, payload, 20U);
    EXPECT_FALSE(mon_io<>::matches(raw));
    EXPECT_FALSE(mon_io<>::decode(raw, out));
    raw = make_view(UBX_CLASS_MON, UBX_ID_MON_IO, payload, 20U);
    EXPECT_TRUE(mon_io<>::matches(raw));
}

TEST(UbxMessageMonIo, RejectsPartialBlockLengthsAndResets)
{
    uint8_t payload[41U] = {};
    payload[0U] = 1U;
    mon_io<> out{};
    for (uint16_t length : {1U, 19U, 21U, 39U, 41U})
    {
        ASSERT_EQ(out.ports.push_back(make_port(2U)), castle::status::ok);
        message_view raw = make_view(UBX_CLASS_MON, UBX_ID_MON_IO, payload, length);
        EXPECT_FALSE(mon_io<>::decode(raw, out)) << length;
        EXPECT_TRUE(out.ports.empty()) << length;
    }
}

TEST(UbxMessageMonIo, RejectsDeclaredLengthLongerThanView)
{
    uint8_t payload[20U] = {};
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_IO, 40U}, array_view<CASTLE_CONST uint8_t>(payload, 20U), checksum{}};
    mon_io<> out{};
    EXPECT_FALSE(mon_io<>::decode(raw, out));
    EXPECT_TRUE(out.ports.empty());
}

TEST(UbxMessageMonIo, CapacityOverflowKeepsFirstPorts)
{
    uint8_t payload[60U] = {};
    payload[0U] = 1U;
    payload[20U] = 2U;
    payload[40U] = 3U;
    message_view raw = make_view(UBX_CLASS_MON, UBX_ID_MON_IO, payload, 60U);
    mon_io<2U> out{};
    ASSERT_TRUE(mon_io<2U>::decode(raw, out));
    ASSERT_EQ(out.ports.size(), 2U);
    EXPECT_EQ(out.ports[0U].rx_bytes, 1U);
    EXPECT_EQ(out.ports[1U].rx_bytes, 2U);
}

TEST(UbxMessageMonIo, EncodeIntoTooSmallBufferReportsFull)
{
    mon_io<> message{};
    ASSERT_EQ(message.ports.push_back(make_port(0U)), castle::status::ok);
    uint8_t frame[UBX_FRAME_OVERHEAD + 20U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(message, frame, UBX_FRAME_OVERHEAD - 1U, written), castle::status::full);
}

} // namespace
