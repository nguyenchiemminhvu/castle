#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/rxm_rtcm.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = rxm_rtcm;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

message distinctive()
{
    message value{};
    value.version = 0x02U;
    value.flags = 0xA5U;
    value.sub_type = 0x1234U;
    value.ref_station = 0x0FEDU;
    value.msg_type = 0x0FE8U;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.version, 0U);
    EXPECT_EQ(value.flags, 0U);
    EXPECT_EQ(value.sub_type, 0U);
    EXPECT_EQ(value.ref_station, 0U);
    EXPECT_EQ(value.msg_type, 0U);
}

TEST(UbxMessageRxmRtcm, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_RXM);
    EXPECT_EQ(message::msg_class, 0x02U);
    EXPECT_EQ(message::msg_id, UBX_ID_RXM_RTCM);
    EXPECT_EQ(message::msg_id, 0x32U);
    EXPECT_EQ(message::payload_length, 8U);
    EXPECT_EQ(message::FLAGS_CRC_FAILED, 0x01U);
    EXPECT_EQ(message::FLAGS_MSG_USED, 0x06U);
    EXPECT_EQ(message::REF_STATION_NOT_REPORTED, 0xFFFFU);
    EXPECT_EQ(static_cast<uint8_t>(rxm_rtcm_msg_used::unknown), 0U);
    EXPECT_EQ(static_cast<uint8_t>(rxm_rtcm_msg_used::not_used), 1U);
    EXPECT_EQ(static_cast<uint8_t>(rxm_rtcm_msg_used::used), 2U);
}

TEST(UbxMessageRxmRtcm, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.flags, original.flags);
    EXPECT_EQ(decoded.sub_type, original.sub_type);
    EXPECT_EQ(decoded.ref_station, original.ref_station);
    EXPECT_EQ(decoded.msg_type, original.msg_type);
    EXPECT_EQ(decoded.crc_failed(), original.crc_failed());
    EXPECT_EQ(decoded.msg_used(), original.msg_used());
}

TEST(UbxMessageRxmRtcm, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    payload[0U] = 0x02U;
    payload[1U] = 0x05U; // crcFailed=1, msgUsed=2
    castle::write_le16(&payload[2U], 0x0A0BU);
    castle::write_le16(&payload[4U], 4095U);
    castle::write_le16(&payload[6U], 1077U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.version, 0x02U);
    EXPECT_EQ(decoded.flags, 0x05U);
    EXPECT_EQ(decoded.sub_type, 0x0A0BU);
    EXPECT_EQ(decoded.ref_station, 4095U);
    EXPECT_EQ(decoded.msg_type, 1077U);
    EXPECT_TRUE(decoded.crc_failed());
    EXPECT_EQ(decoded.msg_used(), rxm_rtcm_msg_used::used);
}

TEST(UbxMessageRxmRtcm, DecodesRefStationNotReported)
{
    uint8_t payload[message::payload_length] = {};
    payload[0U] = 0x02U;
    castle::write_le16(&payload[4U], 0xFFFFU);
    castle::write_le16(&payload[6U], 1230U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.ref_station, message::REF_STATION_NOT_REPORTED);
    EXPECT_EQ(decoded.msg_type, 1230U);
}

TEST(UbxMessageRxmRtcm, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_RXM);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_RXM_RTCM);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(payload[0U], 0x02U);
    EXPECT_EQ(payload[1U], 0xA5U);
    EXPECT_EQ(castle::read_le16(&payload[2U]), 0x1234U);
    EXPECT_EQ(castle::read_le16(&payload[4U]), 0x0FEDU);
    EXPECT_EQ(castle::read_le16(&payload[6U]), 0x0FE8U);
    EXPECT_EQ(payload[2U], 0x34U);
    EXPECT_EQ(payload[3U], 0x12U);
    EXPECT_EQ(payload[4U], 0xEDU);
    EXPECT_EQ(payload[5U], 0x0FU);
    EXPECT_EQ(payload[6U], 0xE8U);
    EXPECT_EQ(payload[7U], 0x0FU);
}

TEST(UbxMessageRxmRtcm, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    payload[0U] = 0x02U;
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_RXM_PMREQ;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageRxmRtcm, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    payload[0U] = 0x02U;

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

TEST(UbxMessageRxmRtcm, CrcFailedAccessor)
{
    message value{};
    EXPECT_FALSE(value.crc_failed());

    value.flags = message::FLAGS_CRC_FAILED;
    EXPECT_TRUE(value.crc_failed());

    value.flags = static_cast<uint8_t>(~message::FLAGS_CRC_FAILED);
    EXPECT_FALSE(value.crc_failed());

    value.flags = 0xFFU;
    EXPECT_TRUE(value.crc_failed());
}

TEST(UbxMessageRxmRtcm, MsgUsedAccessorEveryEnumValue)
{
    message value{};
    EXPECT_EQ(value.msg_used(), rxm_rtcm_msg_used::unknown);

    value.flags = 0x02U;
    EXPECT_EQ(value.msg_used(), rxm_rtcm_msg_used::not_used);

    value.flags = 0x04U;
    EXPECT_EQ(value.msg_used(), rxm_rtcm_msg_used::used);

    // Raw value 3 is not enumerated by the spec but must still decode as 3.
    value.flags = 0x06U;
    EXPECT_EQ(static_cast<uint8_t>(value.msg_used()), 3U);
}

TEST(UbxMessageRxmRtcm, MsgUsedIgnoresOtherBits)
{
    message value{};
    // crcFailed (bit 0) and reserved bits 7..3 set around each msgUsed value.
    const uint8_t other_bits = static_cast<uint8_t>(~message::FLAGS_MSG_USED);
    EXPECT_EQ(other_bits, 0xF9U);

    value.flags = other_bits;
    EXPECT_EQ(value.msg_used(), rxm_rtcm_msg_used::unknown);
    EXPECT_TRUE(value.crc_failed());

    value.flags = static_cast<uint8_t>(other_bits | 0x02U);
    EXPECT_EQ(value.msg_used(), rxm_rtcm_msg_used::not_used);
    EXPECT_TRUE(value.crc_failed());

    value.flags = static_cast<uint8_t>(other_bits | 0x04U);
    EXPECT_EQ(value.msg_used(), rxm_rtcm_msg_used::used);
    EXPECT_TRUE(value.crc_failed());

    // msgUsed bits never leak into crcFailed.
    value.flags = message::FLAGS_MSG_USED;
    EXPECT_FALSE(value.crc_failed());
    value.flags = 0x04U;
    EXPECT_FALSE(value.crc_failed());
    value.flags = 0x02U;
    EXPECT_FALSE(value.crc_failed());

    // Bit 3 alone is outside msgUsed.
    value.flags = 0x08U;
    EXPECT_EQ(value.msg_used(), rxm_rtcm_msg_used::unknown);
    EXPECT_FALSE(value.crc_failed());
}

TEST(UbxMessageRxmRtcm, EncodeIntoTooSmallBufferReportsFull)
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
