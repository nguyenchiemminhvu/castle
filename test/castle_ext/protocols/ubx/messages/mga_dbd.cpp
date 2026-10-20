#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mga_dbd.hpp"
#include "castle_ext/protocols/ubx/messages/mga_ack.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view raw_of(CASTLE_CONST uint8_t* payload, castle::size_type length)
{
    return message_view{
        message_header{UBX_CLASS_MGA, UBX_ID_MGA_DBD, static_cast<uint16_t>(length)},
        array_view<CASTLE_CONST uint8_t>(payload, length),
        checksum{}};
}

TEST(UbxMessageMgaDbd, Constants)
{
    EXPECT_EQ(mga_dbd<>::msg_class, UBX_CLASS_MGA);
    EXPECT_EQ(mga_dbd<>::msg_id, UBX_ID_MGA_DBD);
    EXPECT_EQ(mga_dbd<>::header_length, 12U);
    EXPECT_EQ(mga_dbd<>{}.data.capacity(), 164U);
}

TEST(UbxMessageMgaDbd, SpecOffsetsSkipReservedAndKeepData)
{
    uint8_t payload[12U + 5U] = {};
    for (castle::size_type i = 0U; i < 12U; ++i)
    {
        payload[i] = 0xA5U; // reserved0 must not leak into `data`
    }
    for (castle::size_type i = 0U; i < 5U; ++i)
    {
        payload[12U + i] = static_cast<uint8_t>(0x10U + i);
    }

    CASTLE_CONST message_view raw = raw_of(payload, sizeof(payload));
    EXPECT_TRUE(mga_dbd<>::matches(raw));
    mga_dbd<> out{};
    ASSERT_TRUE(mga_dbd<>::decode(raw, out));
    ASSERT_EQ(out.data.size(), 5U);
    for (castle::size_type i = 0U; i < out.data.size(); ++i)
    {
        EXPECT_EQ(out.data[i], static_cast<uint8_t>(0x10U + i));
    }
}

TEST(UbxMessageMgaDbd, HeaderOnlyEntryHasEmptyData)
{
    uint8_t payload[12U] = {};
    mga_dbd<> out{};
    ASSERT_TRUE(mga_dbd<>::decode(raw_of(payload, sizeof(payload)), out));
    EXPECT_TRUE(out.data.empty());
}

TEST(UbxMessageMgaDbd, RoundTripMaximumEntry)
{
    mga_dbd<> in{};
    for (castle::size_type i = 0U; i < in.data.capacity(); ++i)
    {
        ASSERT_EQ(in.data.push_back(static_cast<uint8_t>(0xFFU - i)), castle::status::ok);
    }

    uint8_t frame[12U + 164U + UBX_FRAME_OVERHEAD] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    for (castle::size_type i = 0U; i < 12U; ++i)
    {
        EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + i], 0U);
    }
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 12U], 0xFFU);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_EQ(raw.payload_length(), 12U + 164U);
    mga_dbd<> out{};
    ASSERT_TRUE(mga_dbd<>::decode(raw, out));
    ASSERT_EQ(out.data.size(), in.data.size());
    for (castle::size_type i = 0U; i < out.data.size(); ++i)
    {
        EXPECT_EQ(out.data[i], in.data[i]);
    }
}

TEST(UbxMessageMgaDbd, CapacityOverflowKeepsPrefix)
{
    uint8_t payload[12U + 10U] = {};
    for (castle::size_type i = 0U; i < 10U; ++i)
    {
        payload[12U + i] = static_cast<uint8_t>(i + 1U);
    }
    mga_dbd<4U> out{};
    ASSERT_TRUE(mga_dbd<4U>::decode(raw_of(payload, sizeof(payload)), out));
    ASSERT_EQ(out.data.size(), 4U);
    EXPECT_EQ(out.data[0U], 1U);
    EXPECT_EQ(out.data[3U], 4U);
}

TEST(UbxMessageMgaDbd, RejectsShortWrongIdAndPoll)
{
    uint8_t payload[12U + 2U] = {};
    payload[12U] = 0x77U;

    mga_dbd<> out{};
    ASSERT_EQ(out.data.push_back(0x55U), castle::status::ok);
    EXPECT_FALSE(mga_dbd<>::matches(raw_of(payload, 11U)));
    EXPECT_FALSE(mga_dbd<>::decode(raw_of(payload, 11U), out));
    EXPECT_TRUE(out.data.empty());

    message_view raw = raw_of(payload, sizeof(payload));
    raw.header.msg_id = UBX_ID_MGA_ACK;
    EXPECT_FALSE(mga_dbd<>::matches(raw));
    EXPECT_FALSE(mga_dbd<>::decode(raw, out));

    raw = raw_of(payload, sizeof(payload));
    raw.header.msg_class = UBX_CLASS_NAV;
    EXPECT_FALSE(mga_dbd<>::matches(raw));

    // A header that claims more bytes than the view holds latches a reader failure.
    raw = raw_of(payload, sizeof(payload));
    raw.payload = array_view<CASTLE_CONST uint8_t>(payload, 6U);
    ASSERT_EQ(out.data.push_back(0x55U), castle::status::ok);
    EXPECT_FALSE(mga_dbd<>::decode(raw, out));
    EXPECT_TRUE(out.data.empty());
}

TEST(UbxMessageMgaDbd, PollRequestIsEmptyAndNotAnEntry)
{
    uint8_t frame[UBX_FRAME_OVERHEAD] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_poll<mga_dbd<>>(frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, UBX_FRAME_OVERHEAD);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_MGA);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_MGA_DBD);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_EQ(raw.payload_length(), 0U);
    EXPECT_FALSE(mga_dbd<>::matches(raw));
    mga_dbd<> out{};
    EXPECT_FALSE(mga_dbd<>::decode(raw, out));
}

TEST(UbxMessageMgaDbd, DispatchExclusiveAgainstAck)
{
    mga_dbd<> entry{};
    ASSERT_EQ(entry.data.push_back(0x01U), castle::status::ok);
    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(entry, frame, sizeof(frame), written), castle::status::ok);
    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(mga_dbd<>::matches(raw));
    EXPECT_FALSE(mga_ack::matches(raw));

    mga_ack ack{};
    ASSERT_EQ(encode_message(ack, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(mga_ack::matches(raw));
    EXPECT_FALSE(mga_dbd<>::matches(raw));
}

TEST(UbxMessageMgaDbd, EncodeIntoTooSmallBuffer)
{
    mga_dbd<> entry{};
    ASSERT_EQ(entry.data.push_back(0x01U), castle::status::ok);
    uint8_t frame[12U + 1U + UBX_FRAME_OVERHEAD] = {};
    castle::size_type written = 7U;
    EXPECT_EQ(encode_message(entry, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
