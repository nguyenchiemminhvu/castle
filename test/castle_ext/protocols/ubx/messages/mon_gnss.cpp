#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_gnss.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(CASTLE_CONST uint8_t* payload, uint16_t length, uint8_t msg_class = UBX_CLASS_MON,
                     uint8_t msg_id = UBX_ID_MON_GNSS)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

TEST(UbxMessageMonGnss, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_gnss::msg_class, 0x0AU);
    EXPECT_EQ(mon_gnss::msg_id, 0x28U);
    EXPECT_EQ(mon_gnss::payload_length, 8U);
    EXPECT_EQ(mon_gnss::GNSS_GPS, 0x01U);
    EXPECT_EQ(mon_gnss::GNSS_GLONASS, 0x02U);
    EXPECT_EQ(mon_gnss::GNSS_BEIDOU, 0x04U);
    EXPECT_EQ(mon_gnss::GNSS_GALILEO, 0x08U);
}

TEST(UbxMessageMonGnss, RoundTripThroughFrame)
{
    mon_gnss message{};
    message.version = 0x01U;
    message.supported = 0x0FU;
    message.default_gnss = 0x0BU;
    message.enabled = 0x05U;
    message.simultaneous = 4U;

    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(8U));
    // Reserved bytes are zero on the wire.
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 5U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 6U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 7U], 0U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    ASSERT_TRUE(mon_gnss::matches(raw));

    mon_gnss decoded{};
    ASSERT_TRUE(mon_gnss::decode(raw, decoded));
    EXPECT_EQ(decoded.version, 0x01U);
    EXPECT_EQ(decoded.supported, 0x0FU);
    EXPECT_EQ(decoded.default_gnss, 0x0BU);
    EXPECT_EQ(decoded.enabled, 0x05U);
    EXPECT_EQ(decoded.simultaneous, 4U);
}

TEST(UbxMessageMonGnss, HandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[8U] = {0x01U, 0x0EU, 0x03U, 0x09U, 0x03U, 0xAAU, 0xBBU, 0xCCU};
    mon_gnss decoded{};
    ASSERT_TRUE(mon_gnss::decode(view_of(payload, 8U), decoded));
    EXPECT_EQ(decoded.version, 0x01U);
    EXPECT_EQ(decoded.supported, 0x0EU);
    EXPECT_EQ(decoded.default_gnss, 0x03U);
    EXPECT_EQ(decoded.enabled, 0x09U);
    EXPECT_EQ(decoded.simultaneous, 0x03U);
}

TEST(UbxMessageMonGnss, BitmaskHelpers)
{
    mon_gnss message{};
    message.supported = static_cast<uint8_t>(mon_gnss::GNSS_GPS | mon_gnss::GNSS_GALILEO);
    message.default_gnss = mon_gnss::GNSS_GLONASS;
    message.enabled = mon_gnss::GNSS_BEIDOU;

    EXPECT_TRUE(message.is_supported(mon_gnss::GNSS_GPS));
    EXPECT_TRUE(message.is_supported(mon_gnss::GNSS_GALILEO));
    EXPECT_TRUE(message.is_supported(static_cast<uint8_t>(mon_gnss::GNSS_GPS | mon_gnss::GNSS_GALILEO)));
    EXPECT_FALSE(message.is_supported(mon_gnss::GNSS_GLONASS));
    EXPECT_FALSE(message.is_supported(static_cast<uint8_t>(mon_gnss::GNSS_GPS | mon_gnss::GNSS_BEIDOU)));

    EXPECT_TRUE(message.is_default(mon_gnss::GNSS_GLONASS));
    EXPECT_FALSE(message.is_default(mon_gnss::GNSS_GPS));

    EXPECT_TRUE(message.is_enabled(mon_gnss::GNSS_BEIDOU));
    EXPECT_FALSE(message.is_enabled(mon_gnss::GNSS_GALILEO));
}

TEST(UbxMessageMonGnss, RejectsWrongClassOrId)
{
    uint8_t payload[8U] = {0x01U, 0x0FU, 0x0FU, 0x0FU, 4U, 0U, 0U, 0U};
    mon_gnss decoded{};
    EXPECT_FALSE(mon_gnss::matches(view_of(payload, 8U, UBX_CLASS_NAV, UBX_ID_MON_GNSS)));
    EXPECT_FALSE(mon_gnss::decode(view_of(payload, 8U, UBX_CLASS_NAV, UBX_ID_MON_GNSS), decoded));
    EXPECT_FALSE(mon_gnss::matches(view_of(payload, 8U, UBX_CLASS_MON, UBX_ID_MON_HW)));
    EXPECT_FALSE(mon_gnss::decode(view_of(payload, 8U, UBX_CLASS_MON, UBX_ID_MON_HW), decoded));
    EXPECT_EQ(decoded.supported, 0U);
}

TEST(UbxMessageMonGnss, RejectsShortAndLongPayloads)
{
    uint8_t payload[9U] = {0x01U, 0x0FU, 0x0FU, 0x0FU, 4U, 0U, 0U, 0U, 0U};
    mon_gnss decoded{};
    EXPECT_FALSE(mon_gnss::decode(view_of(payload, 7U), decoded));
    EXPECT_EQ(decoded.supported, 0U);
    EXPECT_FALSE(mon_gnss::decode(view_of(payload, 9U), decoded));
    EXPECT_EQ(decoded.enabled, 0U);
}

TEST(UbxMessageMonGnss, EncodeIntoTooSmallBufferReportsFull)
{
    mon_gnss message{};
    uint8_t frame[frame_size(7U)] = {};
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageMonGnss, PollRequestHasEmptyPayload)
{
    uint8_t frame[UBX_FRAME_OVERHEAD] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_poll<mon_gnss>(frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_MON);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_MON_GNSS);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_LENGTH], 0U);
}

} // namespace
