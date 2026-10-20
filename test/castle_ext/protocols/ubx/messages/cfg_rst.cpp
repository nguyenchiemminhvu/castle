#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/cfg_rst.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(CASTLE_CONST uint8_t* payload, castle::size_type length)
{
    return message_view{{UBX_CLASS_CFG, UBX_ID_CFG_RST, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

TEST(UbxMessageCfgRst, ConstantsMatchSpec)
{
    EXPECT_EQ(cfg_rst::msg_class, UBX_CLASS_CFG);
    EXPECT_EQ(cfg_rst::msg_id, 0x04U);
    EXPECT_EQ(cfg_rst::payload_length, 4U);

    EXPECT_EQ(cfg_rst::BBR_EPH, 0x0001U);
    EXPECT_EQ(cfg_rst::BBR_ALM, 0x0002U);
    EXPECT_EQ(cfg_rst::BBR_HEALTH, 0x0004U);
    EXPECT_EQ(cfg_rst::BBR_KLOB, 0x0008U);
    EXPECT_EQ(cfg_rst::BBR_POS, 0x0010U);
    EXPECT_EQ(cfg_rst::BBR_CLKD, 0x0020U);
    EXPECT_EQ(cfg_rst::BBR_OSC, 0x0040U);
    EXPECT_EQ(cfg_rst::BBR_UTC, 0x0080U);
    EXPECT_EQ(cfg_rst::BBR_RTC, 0x0100U);
    EXPECT_EQ(cfg_rst::BBR_AOP, 0x8000U);
    EXPECT_EQ(cfg_rst::BBR_HOT_START, 0x0000U);
    EXPECT_EQ(cfg_rst::BBR_WARM_START, 0x0001U);
    EXPECT_EQ(cfg_rst::BBR_COLD_START, 0xFFFFU);

    EXPECT_EQ(static_cast<uint8_t>(cfg_rst_mode::hardware_reset), 0x00U);
    EXPECT_EQ(static_cast<uint8_t>(cfg_rst_mode::software_reset), 0x01U);
    EXPECT_EQ(static_cast<uint8_t>(cfg_rst_mode::software_reset_gnss_only), 0x02U);
    EXPECT_EQ(static_cast<uint8_t>(cfg_rst_mode::hardware_reset_after_shutdown), 0x04U);
    EXPECT_EQ(static_cast<uint8_t>(cfg_rst_mode::gnss_stop), 0x08U);
    EXPECT_EQ(static_cast<uint8_t>(cfg_rst_mode::gnss_start), 0x09U);
}

TEST(UbxMessageCfgRst, DefaultIsHotSoftwareReset)
{
    const cfg_rst value{};
    EXPECT_EQ(value.nav_bbr_mask, cfg_rst::BBR_HOT_START);
    EXPECT_EQ(value.reset_mode, cfg_rst_mode::software_reset);
}

TEST(UbxMessageCfgRst, FactoriesSetMaskAndMode)
{
    EXPECT_EQ(cfg_rst::hot_start().nav_bbr_mask, 0x0000U);
    EXPECT_EQ(cfg_rst::hot_start().reset_mode, cfg_rst_mode::software_reset);
    EXPECT_EQ(cfg_rst::warm_start().nav_bbr_mask, 0x0001U);
    EXPECT_EQ(cfg_rst::warm_start().reset_mode, cfg_rst_mode::software_reset);
    EXPECT_EQ(cfg_rst::cold_start().nav_bbr_mask, 0xFFFFU);
    EXPECT_EQ(cfg_rst::cold_start().reset_mode, cfg_rst_mode::software_reset);

    EXPECT_EQ(cfg_rst::hot_start(cfg_rst_mode::gnss_stop).reset_mode, cfg_rst_mode::gnss_stop);
    EXPECT_EQ(cfg_rst::warm_start(cfg_rst_mode::hardware_reset).reset_mode, cfg_rst_mode::hardware_reset);
    EXPECT_EQ(cfg_rst::cold_start(cfg_rst_mode::hardware_reset_after_shutdown).reset_mode,
              cfg_rst_mode::hardware_reset_after_shutdown);
}

TEST(UbxMessageCfgRst, RoundTripAndEncodedOffsets)
{
    cfg_rst original{};
    original.nav_bbr_mask = 0x81A5U;
    original.reset_mode = cfg_rst_mode::software_reset_gnss_only;

    uint8_t frame[UBX_FRAME_OVERHEAD + cfg_rst::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));

    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_CFG);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_CFG_RST);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 4U);
    EXPECT_EQ(castle::read_le16(&payload[0U]), 0x81A5U);
    EXPECT_EQ(payload[2U], 0x02U);
    EXPECT_EQ(payload[3U], 0x00U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(cfg_rst::matches(raw));
    cfg_rst decoded{};
    ASSERT_TRUE(cfg_rst::decode(raw, decoded));
    EXPECT_EQ(decoded.nav_bbr_mask, 0x81A5U);
    EXPECT_EQ(decoded.reset_mode, cfg_rst_mode::software_reset_gnss_only);
}

TEST(UbxMessageCfgRst, DecodesFieldsAtSpecOffsetsAndIgnoresReserved)
{
    uint8_t payload[4U] = {0U, 0U, 0x08U, 0xEEU};
    castle::write_le16(&payload[0U], 0x0110U);
    cfg_rst decoded{};
    ASSERT_TRUE(cfg_rst::decode(view_of(payload, 4U), decoded));
    EXPECT_EQ(decoded.nav_bbr_mask, cfg_rst::BBR_POS | cfg_rst::BBR_RTC);
    EXPECT_EQ(decoded.reset_mode, cfg_rst_mode::gnss_stop);
}

TEST(UbxMessageCfgRst, RejectsWrongClassIdAndLength)
{
    uint8_t payload[5U] = {0x01U, 0x00U, 0x01U, 0x00U, 0x00U};
    cfg_rst decoded{};

    message_view raw = view_of(payload, 4U);
    raw.header.msg_id = UBX_ID_CFG_SPT;
    EXPECT_FALSE(cfg_rst::matches(raw));
    decoded.nav_bbr_mask = 0x1234U;
    EXPECT_FALSE(cfg_rst::decode(raw, decoded));
    EXPECT_EQ(decoded.nav_bbr_mask, cfg_rst::BBR_HOT_START);

    raw.header.msg_id = UBX_ID_CFG_RST;
    raw.header.msg_class = UBX_CLASS_NAV;
    EXPECT_FALSE(cfg_rst::matches(raw));
    EXPECT_FALSE(cfg_rst::decode(raw, decoded));

    EXPECT_FALSE(cfg_rst::decode(view_of(payload, 3U), decoded));
    EXPECT_FALSE(cfg_rst::decode(view_of(payload, 5U), decoded));
    EXPECT_FALSE(cfg_rst::decode(view_of(nullptr, 0U), decoded));
}

TEST(UbxMessageCfgRst, EncodeIntoTooSmallBufferReportsFull)
{
    const cfg_rst value = cfg_rst::cold_start();
    uint8_t frame[UBX_FRAME_OVERHEAD + cfg_rst::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
