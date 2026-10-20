#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/cfg_spt.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(CASTLE_CONST uint8_t* payload, castle::size_type length)
{
    return message_view{{UBX_CLASS_CFG, UBX_ID_CFG_SPT, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

TEST(UbxMessageCfgSpt, ConstantsMatchSpec)
{
    EXPECT_EQ(cfg_spt::msg_class, UBX_CLASS_CFG);
    EXPECT_EQ(cfg_spt::msg_id, 0x64U);
    EXPECT_EQ(cfg_spt::payload_length, 12U);
    EXPECT_EQ(cfg_spt::version, 0x00U);
}

TEST(UbxMessageCfgSpt, RoundTripAndEncodedOffsets)
{
    cfg_spt original{};
    original.sensor_id = 0xA1B2U;

    uint8_t frame[UBX_FRAME_OVERHEAD + cfg_spt::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));

    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_CFG);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_CFG_SPT);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 12U);
    EXPECT_EQ(payload[0U], 0x00U);
    EXPECT_EQ(payload[1U], 0x00U);
    EXPECT_EQ(castle::read_le16(&payload[2U]), 0xA1B2U);
    for (castle::size_type index = 4U; index < 12U; ++index)
    {
        EXPECT_EQ(payload[index], 0U);
    }

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(cfg_spt::matches(raw));
    cfg_spt decoded{};
    ASSERT_TRUE(cfg_spt::decode(raw, decoded));
    EXPECT_EQ(decoded.sensor_id, 0xA1B2U);
}

TEST(UbxMessageCfgSpt, DecodesFieldsAtSpecOffsetsAndIgnoresReserved)
{
    uint8_t payload[12U] = {0x00U, 0xFFU, 0U, 0U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U};
    castle::write_le16(&payload[2U], 0x0010U);
    cfg_spt decoded{};
    ASSERT_TRUE(cfg_spt::decode(view_of(payload, 12U), decoded));
    EXPECT_EQ(decoded.sensor_id, 0x0010U);
}

TEST(UbxMessageCfgSpt, RejectsUnknownVersion)
{
    uint8_t payload[12U] = {0x01U, 0U, 0x05U, 0U};
    cfg_spt decoded{};
    decoded.sensor_id = 7U;
    EXPECT_FALSE(cfg_spt::decode(view_of(payload, 12U), decoded));
    EXPECT_EQ(decoded.sensor_id, 0U);
}

TEST(UbxMessageCfgSpt, RejectsWrongClassIdAndLength)
{
    uint8_t payload[13U] = {};
    cfg_spt decoded{};

    message_view raw = view_of(payload, 12U);
    raw.header.msg_id = UBX_ID_CFG_RST;
    EXPECT_FALSE(cfg_spt::matches(raw));
    EXPECT_FALSE(cfg_spt::decode(raw, decoded));
    raw.header.msg_id = UBX_ID_CFG_SPT;
    raw.header.msg_class = UBX_CLASS_MON;
    EXPECT_FALSE(cfg_spt::matches(raw));
    EXPECT_FALSE(cfg_spt::decode(raw, decoded));

    EXPECT_FALSE(cfg_spt::decode(view_of(payload, 11U), decoded));
    EXPECT_FALSE(cfg_spt::decode(view_of(payload, 13U), decoded));
    EXPECT_FALSE(cfg_spt::decode(view_of(nullptr, 0U), decoded));
    EXPECT_EQ(decoded.sensor_id, 0U);
}

TEST(UbxMessageCfgSpt, EncodeIntoTooSmallBufferReportsFull)
{
    cfg_spt value{};
    uint8_t frame[UBX_FRAME_OVERHEAD + cfg_spt::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
