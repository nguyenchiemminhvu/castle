#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/sec_crc.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = sec_crc;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

message distinctive()
{
    message value{};
    value.version = 0x5AU;
    value.crc_type = 0xC3U;
    value.reserved = 0xBEEFU;
    value.crc32 = 0x89ABCDEFU;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.version, 0U);
    EXPECT_EQ(value.crc_type, 0U);
    EXPECT_EQ(value.reserved, 0U);
    EXPECT_EQ(value.crc32, 0U);
}

void fill_prefix(uint8_t* payload)
{
    payload[0U] = 0x02U;
    payload[1U] = 0x07U;
    castle::write_le16(&payload[2U], 0x1234U);
    castle::write_le32(&payload[4U], 0xDEADBEEFU);
}

void expect_prefix(const message& value)
{
    EXPECT_EQ(value.version, 0x02U);
    EXPECT_EQ(value.crc_type, 0x07U);
    EXPECT_EQ(value.reserved, 0x1234U);
    EXPECT_EQ(value.crc32, 0xDEADBEEFU);
}

TEST(UbxMessageSecCrc, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_SEC);
    EXPECT_EQ(message::msg_id, UBX_ID_SEC_CRC);
    EXPECT_EQ(message::msg_class, 0x27U);
    EXPECT_EQ(message::msg_id, 0x01U);
    EXPECT_EQ(message::payload_length, 8U);
}

TEST(UbxMessageSecCrc, RoundTripPreservesEveryField)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(message::matches(raw));
    EXPECT_EQ(raw.payload_length(), message::payload_length);

    message decoded{};
    ASSERT_TRUE(message::decode(raw, decoded));
    EXPECT_EQ(decoded.version, original.version);
    EXPECT_EQ(decoded.crc_type, original.crc_type);
    EXPECT_EQ(decoded.reserved, original.reserved);
    EXPECT_EQ(decoded.crc32, original.crc32);
}

TEST(UbxMessageSecCrc, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    fill_prefix(payload);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    expect_prefix(decoded);
}

TEST(UbxMessageSecCrc, EncodesExactlyEightBytesAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length + 4U];
    for (castle::size_type i = 0U; i < sizeof(frame); ++i)
    {
        frame[i] = 0xEEU;
    }
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD + 8U);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_SEC);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_SEC_CRC);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 8U);
    EXPECT_EQ(payload[0U], 0x5AU);
    EXPECT_EQ(payload[1U], 0xC3U);
    EXPECT_EQ(castle::read_le16(&payload[2U]), 0xBEEFU);
    EXPECT_EQ(castle::read_le32(&payload[4U]), 0x89ABCDEFU);
    // Nothing is written past the checksum.
    for (castle::size_type i = written; i < sizeof(frame); ++i)
    {
        EXPECT_EQ(frame[i], 0xEEU);
    }
}

TEST(UbxMessageSecCrc, AcceptsLongerPayloadsAndIgnoresTail)
{
    uint8_t payload[20U] = {};
    fill_prefix(payload);
    for (castle::size_type i = message::payload_length; i < sizeof(payload); ++i)
    {
        payload[i] = static_cast<uint8_t>(0xA0U + i);
    }

    const castle::size_type lengths[] = {8U, 9U, 20U};
    for (castle::size_type length : lengths)
    {
        message decoded = distinctive();
        EXPECT_TRUE(message::matches(view_of(payload, length)));
        ASSERT_TRUE(message::decode(view_of(payload, length), decoded)) << "length " << length;
        expect_prefix(decoded);
    }
}

TEST(UbxMessageSecCrc, RejectsWrongClassOrIdEvenWithLongPayload)
{
    uint8_t payload[20U] = {};
    fill_prefix(payload);
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_TIM;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_SEC_UNIQID;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw = view_of(payload, message::payload_length);
    raw.header.msg_id = UBX_ID_SEC_SIG;
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageSecCrc, RejectsShortPayloads)
{
    uint8_t payload[message::payload_length] = {};
    fill_prefix(payload);

    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, 0U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageSecCrc, EncodeIntoTooSmallBufferReportsFull)
{
    const message value = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    written = 99U;
    EXPECT_EQ(encode_message(value, frame, UBX_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
