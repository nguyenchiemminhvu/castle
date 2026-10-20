#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/tim_tm2.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = tim_tm2;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

message distinctive()
{
    message value{};
    value.ch = 0x01U;
    value.flags = 0xEDU;
    value.count = 0x1234U;
    value.wn_r = 0x0901U;
    value.wn_f = 0x0A02U;
    value.tow_ms_r = 0x11223344U;
    value.tow_sub_ms_r = 0x00055667U;
    value.tow_ms_f = 0x21324354U;
    value.tow_sub_ms_f = 0x000A8899U;
    value.acc_est = 0xC1D2E3F4U;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.ch, 0U);
    EXPECT_EQ(value.flags, 0U);
    EXPECT_EQ(value.count, 0U);
    EXPECT_EQ(value.wn_r, 0U);
    EXPECT_EQ(value.wn_f, 0U);
    EXPECT_EQ(value.tow_ms_r, 0U);
    EXPECT_EQ(value.tow_sub_ms_r, 0U);
    EXPECT_EQ(value.tow_ms_f, 0U);
    EXPECT_EQ(value.tow_sub_ms_f, 0U);
    EXPECT_EQ(value.acc_est, 0U);
}

TEST(UbxMessageTimTm2, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_TIM);
    EXPECT_EQ(message::msg_id, UBX_ID_TIM_TM2);
    EXPECT_EQ(message::msg_class, 0x0DU);
    EXPECT_EQ(message::msg_id, 0x03U);
    EXPECT_EQ(message::payload_length, 28U);
    EXPECT_EQ(message::FLAGS_MODE, 0x01U);
    EXPECT_EQ(message::FLAGS_RUN, 0x02U);
    EXPECT_EQ(message::FLAGS_NEW_FALLING_EDGE, 0x04U);
    EXPECT_EQ(message::FLAGS_TIME_BASE, 0x18U);
    EXPECT_EQ(message::FLAGS_UTC, 0x20U);
    EXPECT_EQ(message::FLAGS_TIME, 0x40U);
    EXPECT_EQ(message::FLAGS_NEW_RISING_EDGE, 0x80U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tm2_time_base::receiver), 0U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tm2_time_base::gnss), 1U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tm2_time_base::utc), 2U);
}

TEST(UbxMessageTimTm2, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.ch, original.ch);
    EXPECT_EQ(decoded.flags, original.flags);
    EXPECT_EQ(decoded.count, original.count);
    EXPECT_EQ(decoded.wn_r, original.wn_r);
    EXPECT_EQ(decoded.wn_f, original.wn_f);
    EXPECT_EQ(decoded.tow_ms_r, original.tow_ms_r);
    EXPECT_EQ(decoded.tow_sub_ms_r, original.tow_sub_ms_r);
    EXPECT_EQ(decoded.tow_ms_f, original.tow_ms_f);
    EXPECT_EQ(decoded.tow_sub_ms_f, original.tow_sub_ms_f);
    EXPECT_EQ(decoded.acc_est, original.acc_est);
}

TEST(UbxMessageTimTm2, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    payload[0U] = 0x00U;
    payload[1U] = 0x6BU;
    castle::write_le16(&payload[2U], 513U);
    castle::write_le16(&payload[4U], 2301U);
    castle::write_le16(&payload[6U], 2302U);
    castle::write_le32(&payload[8U], 345600123U);
    castle::write_le32(&payload[12U], 999999U);
    castle::write_le32(&payload[16U], 345600456U);
    castle::write_le32(&payload[20U], 123456U);
    castle::write_le32(&payload[24U], 27U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.ch, 0U);
    EXPECT_EQ(decoded.flags, 0x6BU);
    EXPECT_EQ(decoded.count, 513U);
    EXPECT_EQ(decoded.wn_r, 2301U);
    EXPECT_EQ(decoded.wn_f, 2302U);
    EXPECT_EQ(decoded.tow_ms_r, 345600123U);
    EXPECT_EQ(decoded.tow_sub_ms_r, 999999U);
    EXPECT_EQ(decoded.tow_ms_f, 345600456U);
    EXPECT_EQ(decoded.tow_sub_ms_f, 123456U);
    EXPECT_EQ(decoded.acc_est, 27U);
    // 0x6B = 0110 1011: mode, run, timeBase=1 (GNSS), utc, time.
    EXPECT_TRUE(decoded.running());
    EXPECT_TRUE(decoded.stopped());
    EXPECT_FALSE(decoded.new_falling_edge());
    EXPECT_EQ(decoded.time_base(), tim_tm2_time_base::gnss);
    EXPECT_TRUE(decoded.utc_available());
    EXPECT_TRUE(decoded.time_valid());
    EXPECT_FALSE(decoded.new_rising_edge());
}

TEST(UbxMessageTimTm2, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_TIM);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_TIM_TM2);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(payload[0U], original.ch);
    EXPECT_EQ(payload[1U], original.flags);
    EXPECT_EQ(castle::read_le16(&payload[2U]), original.count);
    EXPECT_EQ(castle::read_le16(&payload[4U]), original.wn_r);
    EXPECT_EQ(castle::read_le16(&payload[6U]), original.wn_f);
    EXPECT_EQ(castle::read_le32(&payload[8U]), original.tow_ms_r);
    EXPECT_EQ(castle::read_le32(&payload[12U]), original.tow_sub_ms_r);
    EXPECT_EQ(castle::read_le32(&payload[16U]), original.tow_ms_f);
    EXPECT_EQ(castle::read_le32(&payload[20U]), original.tow_sub_ms_f);
    EXPECT_EQ(castle::read_le32(&payload[24U]), original.acc_est);
}

TEST(UbxMessageTimTm2, RejectsWrongClassOrId)
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
    raw.header.msg_id = UBX_ID_TIM_TP;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageTimTm2, RejectsShortAndLongPayloads)
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

TEST(UbxMessageTimTm2, SingleBitAccessorsSetAndClear)
{
    message value{};
    EXPECT_FALSE(value.running());
    EXPECT_FALSE(value.stopped());
    EXPECT_FALSE(value.new_falling_edge());
    EXPECT_FALSE(value.utc_available());
    EXPECT_FALSE(value.time_valid());
    EXPECT_FALSE(value.new_rising_edge());

    value.flags = message::FLAGS_MODE;
    EXPECT_TRUE(value.running());
    EXPECT_FALSE(value.stopped());
    value.flags = message::FLAGS_RUN;
    EXPECT_TRUE(value.stopped());
    EXPECT_FALSE(value.running());
    value.flags = message::FLAGS_NEW_FALLING_EDGE;
    EXPECT_TRUE(value.new_falling_edge());
    EXPECT_FALSE(value.stopped());
    value.flags = message::FLAGS_UTC;
    EXPECT_TRUE(value.utc_available());
    EXPECT_FALSE(value.new_falling_edge());
    value.flags = message::FLAGS_TIME;
    EXPECT_TRUE(value.time_valid());
    EXPECT_FALSE(value.utc_available());
    value.flags = message::FLAGS_NEW_RISING_EDGE;
    EXPECT_TRUE(value.new_rising_edge());
    EXPECT_FALSE(value.time_valid());

    value.flags = 0xFFU;
    EXPECT_TRUE(value.running());
    EXPECT_TRUE(value.stopped());
    EXPECT_TRUE(value.new_falling_edge());
    EXPECT_TRUE(value.utc_available());
    EXPECT_TRUE(value.time_valid());
    EXPECT_TRUE(value.new_rising_edge());

    // Each bit cleared individually while all others are set.
    value.flags = static_cast<uint8_t>(~message::FLAGS_MODE);
    EXPECT_FALSE(value.running());
    value.flags = static_cast<uint8_t>(~message::FLAGS_RUN);
    EXPECT_FALSE(value.stopped());
    value.flags = static_cast<uint8_t>(~message::FLAGS_NEW_FALLING_EDGE);
    EXPECT_FALSE(value.new_falling_edge());
    value.flags = static_cast<uint8_t>(~message::FLAGS_UTC);
    EXPECT_FALSE(value.utc_available());
    value.flags = static_cast<uint8_t>(~message::FLAGS_TIME);
    EXPECT_FALSE(value.time_valid());
    value.flags = static_cast<uint8_t>(~message::FLAGS_NEW_RISING_EDGE);
    EXPECT_FALSE(value.new_rising_edge());
}

TEST(UbxMessageTimTm2, TimeBaseAllValues)
{
    message value{};
    value.flags = 0x00U;
    EXPECT_EQ(value.time_base(), tim_tm2_time_base::receiver);
    value.flags = 0x08U;
    EXPECT_EQ(value.time_base(), tim_tm2_time_base::gnss);
    value.flags = 0x10U;
    EXPECT_EQ(value.time_base(), tim_tm2_time_base::utc);

    // Neighbouring bits do not leak into the time base.
    value.flags = static_cast<uint8_t>(~message::FLAGS_TIME_BASE);
    EXPECT_EQ(value.time_base(), tim_tm2_time_base::receiver);
    value.flags = static_cast<uint8_t>(0xE7U | 0x10U);
    EXPECT_EQ(value.time_base(), tim_tm2_time_base::utc);
    // Reserved value 3 is passed through.
    value.flags = message::FLAGS_TIME_BASE;
    EXPECT_EQ(static_cast<uint8_t>(value.time_base()), 3U);
}

TEST(UbxMessageTimTm2, EncodeIntoTooSmallBufferReportsFull)
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
