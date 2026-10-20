#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/cfg_valset.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;
using castle::protocols::ubx::config::config_entry;
using castle::protocols::ubx::config::config_layer;
using castle::protocols::ubx::config::config_value;

static CASTLE_CONSTEXPR uint32_t KEY_L = 0x10000011U;
static CASTLE_CONSTEXPR uint32_t KEY_U2 = 0x30210001U;
static CASTLE_CONSTEXPR uint32_t KEY_U4 = 0x40520001U;
static CASTLE_CONSTEXPR uint32_t KEY_U8 = 0x50000009U;

message_view view_of(CASTLE_CONST uint8_t* payload, castle::size_type length)
{
    return message_view{{UBX_CLASS_CFG, UBX_ID_CFG_VALSET, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

config_entry make_entry(uint32_t key, config_value value)
{
    config_entry entry{};
    entry.key_id = key;
    entry.value = value;
    return entry;
}

TEST(UbxMessageCfgValset, ConstantsMatchSpec)
{
    EXPECT_EQ(cfg_valset<>::msg_class, UBX_CLASS_CFG);
    EXPECT_EQ(cfg_valset<>::msg_id, 0x8AU);
    EXPECT_EQ(cfg_valset<>::version, 0x00U);
    EXPECT_EQ(cfg_valset<>::header_length, 4U);
    EXPECT_EQ(cfg_valset_transaction<>::msg_id, 0x8AU);
    EXPECT_EQ(cfg_valset_transaction<>::version, 0x01U);
    EXPECT_EQ(cfg_valset<>::LAYERS_RAM, 0x01U);
    EXPECT_EQ(cfg_valset<>::LAYERS_BBR, 0x02U);
    EXPECT_EQ(cfg_valset<>::LAYERS_FLASH, 0x04U);
    EXPECT_EQ(cfg_valset<>().entries.capacity(), 64U);
    EXPECT_EQ(cfg_valset_transaction<>().entries.capacity(), 64U);
}

TEST(UbxMessageCfgValset, V0RoundTripAndSpecOffsets)
{
    cfg_valset<> original{};
    original.layers = config_layer::bbr | config_layer::flash;
    ASSERT_EQ(original.entries.push_back(make_entry(KEY_L, config_value(true))), castle::status::ok);
    ASSERT_EQ(original.entries.push_back(make_entry(KEY_U2, config_value(static_cast<uint16_t>(0xA1B2U)))), castle::status::ok);
    ASSERT_EQ(original.entries.push_back(make_entry(KEY_U4, config_value(static_cast<int32_t>(-5)))), castle::status::ok);
    ASSERT_EQ(original.entries.push_back(make_entry(KEY_U8, config_value(static_cast<uint64_t>(0x0102030405060708ULL)))), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 4U + 5U + 6U + 8U + 12U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));

    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_CFG_VALSET);
    EXPECT_EQ(payload[0U], 0x00U);
    EXPECT_EQ(payload[1U], 0x06U);
    EXPECT_EQ(payload[2U], 0U);
    EXPECT_EQ(payload[3U], 0U);
    EXPECT_EQ(castle::read_le32(&payload[4U]), KEY_L);
    EXPECT_EQ(payload[8U], 1U);
    EXPECT_EQ(castle::read_le32(&payload[9U]), KEY_U2);
    EXPECT_EQ(castle::read_le16(&payload[13U]), 0xA1B2U);
    EXPECT_EQ(castle::read_le32(&payload[15U]), KEY_U4);
    EXPECT_EQ(castle::read_le32s(&payload[19U]), -5);
    EXPECT_EQ(castle::read_le32(&payload[23U]), KEY_U8);
    EXPECT_EQ(castle::read_le64(&payload[27U]), 0x0102030405060708ULL);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(cfg_valset<>::matches(raw));
    EXPECT_FALSE(cfg_valset_transaction<>::matches(raw));

    cfg_valset<> decoded{};
    ASSERT_TRUE(cfg_valset<>::decode(raw, decoded));
    EXPECT_EQ(decoded.layers, 0x06U);
    ASSERT_EQ(decoded.entries.size(), 4U);
    EXPECT_TRUE(decoded.entries[0U].value.as_bool());
    EXPECT_EQ(decoded.entries[1U].value.as_u16(), 0xA1B2U);
    EXPECT_EQ(decoded.entries[2U].value.as_i32(), -5);
    EXPECT_EQ(decoded.entries[3U].value.as_u64(), 0x0102030405060708ULL);
    for (castle::size_type index = 0U; index < 4U; ++index)
    {
        EXPECT_EQ(decoded.entries[index].key_id, original.entries[index].key_id);
    }
}

TEST(UbxMessageCfgValset, V0DecodesHandBuiltPayloadAndIgnoresReserved)
{
    uint8_t payload[4U + 6U] = {0x00U, 0x01U, 0xEEU, 0xFFU};
    castle::write_le32(&payload[4U], KEY_U2);
    castle::write_le16(&payload[8U], 1000U);
    cfg_valset<> decoded{};
    ASSERT_TRUE(cfg_valset<>::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.layers, 0x01U);
    ASSERT_EQ(decoded.entries.size(), 1U);
    EXPECT_EQ(decoded.entries[0U].key_id, KEY_U2);
    EXPECT_EQ(decoded.entries[0U].value.as_u16(), 1000U);
}

TEST(UbxMessageCfgValset, V1RoundTripAndSpecOffsets)
{
    cfg_valset_transaction<> original{};
    original.layers = cfg_valset_transaction<>::LAYERS_RAM | cfg_valset_transaction<>::LAYERS_FLASH;
    original.set_action(cfg_transaction_action::ongoing);
    ASSERT_EQ(original.entries.push_back(make_entry(KEY_U4, config_value(static_cast<uint32_t>(0xCAFEBABEU)))), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 4U + 8U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));

    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(payload[0U], 0x01U);
    EXPECT_EQ(payload[1U], 0x05U);
    EXPECT_EQ(payload[2U], 0x02U);
    EXPECT_EQ(payload[3U], 0U);
    EXPECT_EQ(castle::read_le32(&payload[4U]), KEY_U4);
    EXPECT_EQ(castle::read_le32(&payload[8U]), 0xCAFEBABEU);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(cfg_valset_transaction<>::matches(raw));
    EXPECT_FALSE(cfg_valset<>::matches(raw));

    cfg_valset_transaction<> decoded{};
    ASSERT_TRUE(cfg_valset_transaction<>::decode(raw, decoded));
    EXPECT_EQ(decoded.layers, 0x05U);
    EXPECT_EQ(decoded.transaction, 0x02U);
    EXPECT_EQ(decoded.action(), cfg_transaction_action::ongoing);
    ASSERT_EQ(decoded.entries.size(), 1U);
    EXPECT_EQ(decoded.entries[0U].value.as_u32(), 0xCAFEBABEU);
}

TEST(UbxMessageCfgValset, V1WithoutEntriesManagesTransactionOnly)
{
    uint8_t payload[4U] = {0x01U, 0x01U, 0x03U, 0x00U};
    cfg_valset_transaction<> decoded{};
    ASSERT_TRUE(cfg_valset_transaction<>::decode(view_of(payload, 4U), decoded));
    EXPECT_EQ(decoded.action(), cfg_transaction_action::apply);
    EXPECT_TRUE(decoded.entries.empty());
}

TEST(UbxMessageCfgValset, LayerAccessorsSetAndClear)
{
    cfg_valset<> value{};
    EXPECT_EQ(value.layers, cfg_valset<>::LAYERS_RAM);
    EXPECT_TRUE(value.ram());
    EXPECT_FALSE(value.bbr());
    EXPECT_FALSE(value.flash());

    value.layers = cfg_valset<>::LAYERS_BBR;
    EXPECT_FALSE(value.ram());
    EXPECT_TRUE(value.bbr());
    EXPECT_FALSE(value.flash());

    value.layers = cfg_valset<>::LAYERS_FLASH;
    EXPECT_FALSE(value.ram());
    EXPECT_FALSE(value.bbr());
    EXPECT_TRUE(value.flash());

    value.layers = 0x07U;
    EXPECT_TRUE(value.ram() && value.bbr() && value.flash());
    value.layers = 0U;
    EXPECT_FALSE(value.ram() || value.bbr() || value.flash());
}

TEST(UbxMessageCfgValset, EntriesBeyondCapacityAreSkipped)
{
    uint8_t payload[4U + (3U * 5U)] = {0x00U, 0x01U, 0U, 0U};
    for (castle::size_type index = 0U; index < 3U; ++index)
    {
        castle::write_le32(&payload[4U + (index * 5U)], KEY_L);
        payload[8U + (index * 5U)] = static_cast<uint8_t>(index);
    }
    cfg_valset<2U> decoded{};
    ASSERT_TRUE(cfg_valset<2U>::decode(view_of(payload, sizeof(payload)), decoded));
    ASSERT_EQ(decoded.entries.size(), 2U);
    EXPECT_EQ(decoded.entries[1U].value.as_u8(), 1U);
}

TEST(UbxMessageCfgValset, RejectsMalformedPayloads)
{
    cfg_valset<> decoded{};

    uint8_t unknown[9U] = {0x00U, 0x01U, 0U, 0U};
    castle::write_le32(&unknown[4U], 0x00000001U);
    decoded.layers = 0x07U;
    EXPECT_FALSE(cfg_valset<>::decode(view_of(unknown, sizeof(unknown)), decoded));
    EXPECT_EQ(decoded.layers, cfg_valset<>::LAYERS_RAM);
    EXPECT_TRUE(decoded.entries.empty());

    uint8_t truncated[9U] = {0x00U, 0x01U, 0U, 0U};
    castle::write_le32(&truncated[4U], KEY_U2);
    EXPECT_FALSE(cfg_valset<>::decode(view_of(truncated, sizeof(truncated)), decoded));

    uint8_t header[4U] = {0x00U, 0x01U, 0U, 0U};
    EXPECT_FALSE(cfg_valset<>::decode(view_of(header, 3U), decoded));
    EXPECT_FALSE(cfg_valset<>::decode(view_of(nullptr, 0U), decoded));

    header[0U] = 0x02U;
    EXPECT_FALSE(cfg_valset<>::matches(view_of(header, 4U)));
    EXPECT_FALSE(cfg_valset_transaction<>::matches(view_of(header, 4U)));

    header[0U] = 0x00U;
    message_view raw = view_of(header, 4U);
    raw.header.msg_id = UBX_ID_CFG_VALDEL;
    EXPECT_FALSE(cfg_valset<>::matches(raw));
    EXPECT_FALSE(cfg_valset<>::decode(raw, decoded));
    raw.header.msg_id = UBX_ID_CFG_VALSET;
    raw.header.msg_class = UBX_CLASS_ACK;
    EXPECT_FALSE(cfg_valset<>::matches(raw));
}

TEST(UbxMessageCfgValset, EncodeUnknownKeySizeLatchesInvalidArgument)
{
    cfg_valset<> v0{};
    ASSERT_EQ(v0.entries.push_back(make_entry(KEY_U2, config_value(static_cast<uint16_t>(1U)))), castle::status::ok);
    ASSERT_EQ(v0.entries.push_back(make_entry(0x00000001U, config_value(static_cast<uint8_t>(1U)))), castle::status::ok);
    uint8_t frame[64U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(v0, frame, sizeof(frame), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);

    cfg_valset_transaction<> v1{};
    ASSERT_EQ(v1.entries.push_back(make_entry(0xF0000001U, config_value(static_cast<uint8_t>(1U)))), castle::status::ok);
    written = 99U;
    EXPECT_EQ(encode_message(v1, frame, sizeof(frame), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageCfgValset, EncodeIntoTooSmallBufferReportsFull)
{
    cfg_valset<> value{};
    ASSERT_EQ(value.entries.push_back(make_entry(KEY_U8, config_value(static_cast<uint64_t>(1U)))), castle::status::ok);
    uint8_t frame[UBX_FRAME_OVERHEAD + 16U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageCfgValset, MaximumSixtyFourEntriesRoundTrip)
{
    cfg_valset<> original{};
    for (uint32_t index = 0U; index < 64U; ++index)
    {
        ASSERT_EQ(original.entries.push_back(make_entry(0x10000000U | index, config_value(static_cast<uint8_t>(index)))),
                  castle::status::ok);
    }
    EXPECT_TRUE(original.entries.full());

    uint8_t frame[UBX_FRAME_OVERHEAD + 4U + (64U * 5U)];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    cfg_valset<> decoded{};
    ASSERT_TRUE(cfg_valset<>::decode(raw, decoded));
    ASSERT_EQ(decoded.entries.size(), 64U);
    EXPECT_EQ(decoded.entries[63U].key_id, 0x1000003FU);
    EXPECT_EQ(decoded.entries[63U].value.as_u8(), 63U);
}

} // namespace
