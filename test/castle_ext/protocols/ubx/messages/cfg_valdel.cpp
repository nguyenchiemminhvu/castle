#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/cfg_valdel.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(CASTLE_CONST uint8_t* payload, castle::size_type length)
{
    return message_view{{UBX_CLASS_CFG, UBX_ID_CFG_VALDEL, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

TEST(UbxMessageCfgValdel, ConstantsMatchSpec)
{
    EXPECT_EQ(cfg_valdel<>::msg_class, UBX_CLASS_CFG);
    EXPECT_EQ(cfg_valdel<>::msg_id, 0x8CU);
    EXPECT_EQ(cfg_valdel<>::version, 0x00U);
    EXPECT_EQ(cfg_valdel<>::header_length, 4U);
    EXPECT_EQ(cfg_valdel_transaction<>::msg_id, 0x8CU);
    EXPECT_EQ(cfg_valdel_transaction<>::version, 0x01U);
    EXPECT_EQ(cfg_valdel<>::LAYERS_BBR, 0x02U);
    EXPECT_EQ(cfg_valdel<>::LAYERS_FLASH, 0x04U);
    EXPECT_EQ(cfg_valdel<>().keys.capacity(), 64U);
    EXPECT_EQ(cfg_valdel_transaction<>().keys.capacity(), 64U);
}

TEST(UbxMessageCfgValdel, V0RoundTripAndSpecOffsets)
{
    cfg_valdel<> original{};
    original.layers = cfg_valdel<>::LAYERS_BBR;
    ASSERT_EQ(original.keys.push_back(0x30210001U), castle::status::ok);
    ASSERT_EQ(original.keys.push_back(0x40520001U), castle::status::ok);
    ASSERT_EQ(original.keys.push_back(0xA1B2C3D4U), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 4U + 12U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));

    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_CFG_VALDEL);
    EXPECT_EQ(payload[0U], 0x00U);
    EXPECT_EQ(payload[1U], 0x02U);
    EXPECT_EQ(payload[2U], 0U);
    EXPECT_EQ(payload[3U], 0U);
    EXPECT_EQ(castle::read_le32(&payload[4U]), 0x30210001U);
    EXPECT_EQ(castle::read_le32(&payload[8U]), 0x40520001U);
    EXPECT_EQ(castle::read_le32(&payload[12U]), 0xA1B2C3D4U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(cfg_valdel<>::matches(raw));
    EXPECT_FALSE(cfg_valdel_transaction<>::matches(raw));

    cfg_valdel<> decoded{};
    ASSERT_TRUE(cfg_valdel<>::decode(raw, decoded));
    EXPECT_EQ(decoded.layers, 0x02U);
    ASSERT_EQ(decoded.keys.size(), 3U);
    EXPECT_EQ(decoded.keys[0U], 0x30210001U);
    EXPECT_EQ(decoded.keys[1U], 0x40520001U);
    EXPECT_EQ(decoded.keys[2U], 0xA1B2C3D4U);
}

TEST(UbxMessageCfgValdel, V0DecodesHandBuiltPayloadAndIgnoresReserved)
{
    uint8_t payload[8U] = {0x00U, 0x04U, 0x5AU, 0xA5U};
    castle::write_le32(&payload[4U], 0x10110013U);
    cfg_valdel<> decoded{};
    ASSERT_TRUE(cfg_valdel<>::decode(view_of(payload, 8U), decoded));
    EXPECT_EQ(decoded.layers, 0x04U);
    ASSERT_EQ(decoded.keys.size(), 1U);
    EXPECT_EQ(decoded.keys[0U], 0x10110013U);
}

TEST(UbxMessageCfgValdel, V1RoundTripAndSpecOffsets)
{
    cfg_valdel_transaction<> original{};
    original.layers = cfg_valdel_transaction<>::LAYERS_FLASH;
    original.set_action(cfg_transaction_action::start);
    ASSERT_EQ(original.keys.push_back(0x20910007U), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 4U + 4U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));

    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(payload[0U], 0x01U);
    EXPECT_EQ(payload[1U], 0x04U);
    EXPECT_EQ(payload[2U], 0x01U);
    EXPECT_EQ(payload[3U], 0U);
    EXPECT_EQ(castle::read_le32(&payload[4U]), 0x20910007U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(cfg_valdel_transaction<>::matches(raw));
    EXPECT_FALSE(cfg_valdel<>::matches(raw));

    cfg_valdel_transaction<> decoded{};
    ASSERT_TRUE(cfg_valdel_transaction<>::decode(raw, decoded));
    EXPECT_EQ(decoded.layers, 0x04U);
    EXPECT_EQ(decoded.transaction, 0x01U);
    EXPECT_EQ(decoded.action(), cfg_transaction_action::start);
    ASSERT_EQ(decoded.keys.size(), 1U);
    EXPECT_EQ(decoded.keys[0U], 0x20910007U);
}

TEST(UbxMessageCfgValdel, V1WithoutKeysManagesTransactionOnly)
{
    uint8_t payload[4U] = {0x01U, 0x06U, 0x03U, 0x00U};
    cfg_valdel_transaction<> decoded{};
    ASSERT_TRUE(cfg_valdel_transaction<>::decode(view_of(payload, 4U), decoded));
    EXPECT_EQ(decoded.action(), cfg_transaction_action::apply);
    EXPECT_TRUE(decoded.keys.empty());
}

TEST(UbxMessageCfgValdel, LayerAccessorsSetAndClear)
{
    cfg_valdel<> value{};
    EXPECT_TRUE(value.bbr());
    EXPECT_TRUE(value.flash());

    value.layers = cfg_valdel<>::LAYERS_BBR;
    EXPECT_TRUE(value.bbr());
    EXPECT_FALSE(value.flash());

    value.layers = cfg_valdel<>::LAYERS_FLASH;
    EXPECT_FALSE(value.bbr());
    EXPECT_TRUE(value.flash());

    value.layers = 0x01U; // RAM is not a VALDEL layer
    EXPECT_FALSE(value.bbr());
    EXPECT_FALSE(value.flash());
}

TEST(UbxMessageCfgValdel, KeysBeyondCapacityAreSkipped)
{
    uint8_t payload[4U + 12U] = {0x00U, 0x04U, 0U, 0U};
    castle::write_le32(&payload[4U], 1U);
    castle::write_le32(&payload[8U], 2U);
    castle::write_le32(&payload[12U], 3U);
    cfg_valdel<1U> decoded{};
    ASSERT_TRUE(cfg_valdel<1U>::decode(view_of(payload, sizeof(payload)), decoded));
    ASSERT_EQ(decoded.keys.size(), 1U);
    EXPECT_EQ(decoded.keys[0U], 1U);
}

TEST(UbxMessageCfgValdel, RejectsMalformedPayloads)
{
    cfg_valdel<> decoded{};

    uint8_t partial[4U + 6U] = {0x00U, 0x04U, 0U, 0U};
    decoded.layers = 0U;
    EXPECT_FALSE(cfg_valdel<>::decode(view_of(partial, sizeof(partial)), decoded));
    EXPECT_EQ(decoded.layers, cfg_valdel<>::LAYERS_BBR | cfg_valdel<>::LAYERS_FLASH);
    EXPECT_TRUE(decoded.keys.empty());

    uint8_t header[4U] = {0x00U, 0x04U, 0U, 0U};
    EXPECT_FALSE(cfg_valdel<>::decode(view_of(header, 3U), decoded));
    EXPECT_FALSE(cfg_valdel<>::decode(view_of(nullptr, 0U), decoded));

    header[0U] = 0x02U;
    EXPECT_FALSE(cfg_valdel<>::matches(view_of(header, 4U)));
    EXPECT_FALSE(cfg_valdel_transaction<>::matches(view_of(header, 4U)));

    header[0U] = 0x00U;
    message_view raw = view_of(header, 4U);
    raw.header.msg_id = UBX_ID_CFG_VALSET;
    EXPECT_FALSE(cfg_valdel<>::matches(raw));
    EXPECT_FALSE(cfg_valdel<>::decode(raw, decoded));
    raw.header.msg_id = UBX_ID_CFG_VALDEL;
    raw.header.msg_class = UBX_CLASS_MON;
    EXPECT_FALSE(cfg_valdel<>::matches(raw));
}

TEST(UbxMessageCfgValdel, EncodeIntoTooSmallBufferReportsFull)
{
    cfg_valdel_transaction<> value{};
    ASSERT_EQ(value.keys.push_back(1U), castle::status::ok);
    uint8_t frame[UBX_FRAME_OVERHEAD + 8U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
