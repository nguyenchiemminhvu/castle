#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/tim_vrfy.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = tim_vrfy;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

message distinctive()
{
    message value{};
    value.itow = 0x11223344;
    value.frac = -654321;
    value.delta_ms = -12345678;
    value.delta_ns = 0x0A0B0C0D;
    value.wno = 0xBEEFU;
    value.flags = 0xFBU;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.itow, 0);
    EXPECT_EQ(value.frac, 0);
    EXPECT_EQ(value.delta_ms, 0);
    EXPECT_EQ(value.delta_ns, 0);
    EXPECT_EQ(value.wno, 0U);
    EXPECT_EQ(value.flags, 0U);
}

TEST(UbxMessageTimVrfy, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_TIM);
    EXPECT_EQ(message::msg_id, UBX_ID_TIM_VRFY);
    EXPECT_EQ(message::msg_class, 0x0DU);
    EXPECT_EQ(message::msg_id, 0x06U);
    EXPECT_EQ(message::payload_length, 20U);
    EXPECT_EQ(message::FLAGS_SRC, 0x07U);
    EXPECT_EQ(static_cast<uint8_t>(tim_vrfy_src::no_aiding), 0U);
    EXPECT_EQ(static_cast<uint8_t>(tim_vrfy_src::rtc), 2U);
    EXPECT_EQ(static_cast<uint8_t>(tim_vrfy_src::assistance), 3U);
}

TEST(UbxMessageTimVrfy, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.itow, original.itow);
    EXPECT_EQ(decoded.frac, original.frac);
    EXPECT_EQ(decoded.delta_ms, original.delta_ms);
    EXPECT_EQ(decoded.delta_ns, original.delta_ns);
    EXPECT_EQ(decoded.wno, original.wno);
    EXPECT_EQ(decoded.flags, original.flags);
}

TEST(UbxMessageTimVrfy, DecodesSignedFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32s(&payload[0U], -1);
    castle::write_le32s(&payload[4U], -999999);
    castle::write_le32s(&payload[8U], -2147483647 - 1);
    castle::write_le32s(&payload[12U], -42);
    castle::write_le16(&payload[16U], 2345U);
    payload[18U] = 0x03U;
    payload[19U] = 0xFFU; // reserved0 ignored on decode

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.itow, -1);
    EXPECT_EQ(decoded.frac, -999999);
    EXPECT_EQ(decoded.delta_ms, -2147483647 - 1);
    EXPECT_EQ(decoded.delta_ns, -42);
    EXPECT_EQ(decoded.wno, 2345U);
    EXPECT_EQ(decoded.flags, 0x03U);
    EXPECT_EQ(decoded.src(), tim_vrfy_src::assistance);
}

TEST(UbxMessageTimVrfy, EncodesFieldsAtSpecOffsets)
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
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_TIM);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_TIM_VRFY);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(castle::read_le32s(&payload[0U]), original.itow);
    EXPECT_EQ(castle::read_le32s(&payload[4U]), original.frac);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.delta_ms);
    EXPECT_EQ(castle::read_le32s(&payload[12U]), original.delta_ns);
    EXPECT_EQ(castle::read_le16(&payload[16U]), original.wno);
    EXPECT_EQ(payload[18U], original.flags);
    EXPECT_EQ(payload[19U], 0U);
}

TEST(UbxMessageTimVrfy, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    payload[0U] = 1U;
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_SEC;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_TIM_TM2;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageTimVrfy, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    payload[0U] = 1U;

    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length + 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, 0U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageTimVrfy, SrcAllValuesUsesOnlyBits2To0)
{
    message value{};
    value.flags = 0x00U;
    EXPECT_EQ(value.src(), tim_vrfy_src::no_aiding);
    value.flags = 0x02U;
    EXPECT_EQ(value.src(), tim_vrfy_src::rtc);
    value.flags = 0x03U;
    EXPECT_EQ(value.src(), tim_vrfy_src::assistance);

    // Bits 7..3 are ignored.
    value.flags = 0xF8U;
    EXPECT_EQ(value.src(), tim_vrfy_src::no_aiding);
    value.flags = 0xFAU;
    EXPECT_EQ(value.src(), tim_vrfy_src::rtc);
    value.flags = 0x0BU;
    EXPECT_EQ(value.src(), tim_vrfy_src::assistance);
    // Values without an enumerator are passed through, still masked to 3 bits.
    value.flags = 0xFFU;
    EXPECT_EQ(static_cast<uint8_t>(value.src()), 7U);
    value.flags = 0x81U;
    EXPECT_EQ(static_cast<uint8_t>(value.src()), 1U);
}

TEST(UbxMessageTimVrfy, EncodeIntoTooSmallBufferReportsFull)
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
