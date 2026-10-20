#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/sec_uniqid.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = sec_uniqid;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

message distinctive()
{
    message value{};
    value.version = 0x01U;
    value.unique_id[0U] = 0x11U;
    value.unique_id[1U] = 0x22U;
    value.unique_id[2U] = 0x33U;
    value.unique_id[3U] = 0x44U;
    value.unique_id[4U] = 0x55U;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.version, 0U);
    for (castle::size_type i = 0U; i < value.unique_id.size(); ++i)
    {
        EXPECT_EQ(value.unique_id[i], 0U) << "index " << i;
    }
}

TEST(UbxMessageSecUniqid, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_SEC);
    EXPECT_EQ(message::msg_id, UBX_ID_SEC_UNIQID);
    EXPECT_EQ(message::msg_class, 0x27U);
    EXPECT_EQ(message::msg_id, 0x03U);
    EXPECT_EQ(message::payload_length, 9U);
    const message value{};
    EXPECT_EQ(value.unique_id.size(), 5U);
}

TEST(UbxMessageSecUniqid, RoundTripPreservesEveryField)
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
    for (castle::size_type i = 0U; i < original.unique_id.size(); ++i)
    {
        EXPECT_EQ(decoded.unique_id[i], original.unique_id[i]) << "index " << i;
    }
}

TEST(UbxMessageSecUniqid, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    payload[0U] = 0x01U;
    payload[1U] = 0xFFU; // reserved0 ignored on decode
    payload[2U] = 0xFFU;
    payload[3U] = 0xFFU;
    payload[4U] = 0xA1U;
    payload[5U] = 0xB2U;
    payload[6U] = 0xC3U;
    payload[7U] = 0xD4U;
    payload[8U] = 0xE5U;

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.version, 0x01U);
    EXPECT_EQ(decoded.unique_id[0U], 0xA1U);
    EXPECT_EQ(decoded.unique_id[1U], 0xB2U);
    EXPECT_EQ(decoded.unique_id[2U], 0xC3U);
    EXPECT_EQ(decoded.unique_id[3U], 0xD4U);
    EXPECT_EQ(decoded.unique_id[4U], 0xE5U);
}

TEST(UbxMessageSecUniqid, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    for (castle::size_type i = 0U; i < sizeof(frame); ++i)
    {
        frame[i] = 0xEEU;
    }
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_SEC);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_SEC_UNIQID);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(payload[0U], 0x01U);
    EXPECT_EQ(payload[1U], 0U);
    EXPECT_EQ(payload[2U], 0U);
    EXPECT_EQ(payload[3U], 0U);
    EXPECT_EQ(payload[4U], 0x11U);
    EXPECT_EQ(payload[5U], 0x22U);
    EXPECT_EQ(payload[6U], 0x33U);
    EXPECT_EQ(payload[7U], 0x44U);
    EXPECT_EQ(payload[8U], 0x55U);
}

TEST(UbxMessageSecUniqid, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    payload[0U] = 0x01U;
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_TIM;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_SEC_CRC;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageSecUniqid, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    payload[0U] = 0x01U;

    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, 10U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, 0U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageSecUniqid, EncodeIntoTooSmallBufferReportsFull)
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
