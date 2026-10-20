#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/cfg_valget.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

static CASTLE_CONSTEXPR uint32_t KEY_U1 = 0x10000001U;
static CASTLE_CONSTEXPR uint32_t KEY_U2 = 0x30000002U;
static CASTLE_CONSTEXPR uint32_t KEY_U4 = 0x40000003U;
static CASTLE_CONSTEXPR uint32_t KEY_U8 = 0x50000004U;

message_view view_of(CASTLE_CONST uint8_t* payload, castle::size_type length)
{
    return message_view{{UBX_CLASS_CFG, UBX_ID_CFG_VALGET, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

cfg_valget_entry make_entry(uint32_t key, uint64_t value)
{
    cfg_valget_entry entry{};
    entry.key_id = key;
    entry.value = value;
    return entry;
}

TEST(UbxMessageCfgValget, ConstantsMatchSpec)
{
    EXPECT_EQ(cfg_valget<>::msg_class, UBX_CLASS_CFG);
    EXPECT_EQ(cfg_valget<>::msg_id, 0x8BU);
    EXPECT_EQ(cfg_valget<>::version, 0x01U);
    EXPECT_EQ(cfg_valget<>::header_length, 4U);
    EXPECT_EQ(cfg_valget_poll<>::msg_class, UBX_CLASS_CFG);
    EXPECT_EQ(cfg_valget_poll<>::msg_id, 0x8BU);
    EXPECT_EQ(cfg_valget_poll<>::version, 0x00U);
    EXPECT_EQ(cfg_valget_poll<>::header_length, 4U);
    EXPECT_EQ(cfg_valget<>().entries.capacity(), 64U);
    EXPECT_EQ(cfg_valget_poll<>().keys.capacity(), 64U);
}

TEST(UbxMessageCfgValget, PollRoundTripPreservesEveryField)
{
    cfg_valget_poll<> original{};
    original.layer = config::UBX_CFG_VALGET_LAYER_DEFAULT;
    original.position = 0xA1B2U;
    ASSERT_EQ(original.keys.push_back(0x11223344U), castle::status::ok);
    ASSERT_EQ(original.keys.push_back(0x0FFFFFFFU), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 12U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));

    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_CFG_VALGET);
    EXPECT_EQ(payload[0U], cfg_valget_poll<>::version);
    EXPECT_EQ(payload[1U], config::UBX_CFG_VALGET_LAYER_DEFAULT);
    EXPECT_EQ(castle::read_le16(&payload[2U]), 0xA1B2U);
    EXPECT_EQ(castle::read_le32(&payload[4U]), 0x11223344U);
    EXPECT_EQ(castle::read_le32(&payload[8U]), 0x0FFFFFFFU);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(cfg_valget_poll<>::matches(raw));
    EXPECT_FALSE(cfg_valget<>::matches(raw));

    cfg_valget_poll<> decoded{};
    ASSERT_TRUE(cfg_valget_poll<>::decode(raw, decoded));
    EXPECT_EQ(decoded.layer, config::UBX_CFG_VALGET_LAYER_DEFAULT);
    EXPECT_EQ(decoded.position, 0xA1B2U);
    ASSERT_EQ(decoded.keys.size(), 2U);
    EXPECT_EQ(decoded.keys[0U], 0x11223344U);
    EXPECT_EQ(decoded.keys[1U], 0x0FFFFFFFU);
}

TEST(UbxMessageCfgValget, PollWithoutKeysIsHeaderOnly)
{
    cfg_valget_poll<> original{};
    uint8_t frame[UBX_FRAME_OVERHEAD + 4U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    cfg_valget_poll<> decoded{};
    ASSERT_TRUE(cfg_valget_poll<>::decode(raw, decoded));
    EXPECT_TRUE(decoded.keys.empty());
    EXPECT_EQ(decoded.layer, config::UBX_CFG_VALGET_LAYER_RAM);
}

TEST(UbxMessageCfgValget, PollPartialKeyIsRejected)
{
    uint8_t payload[7U] = {cfg_valget_poll<>::version, 1U, 0U, 0U, 1U, 2U, 3U};
    cfg_valget_poll<> decoded{};
    decoded.layer = 9U;
    EXPECT_FALSE(cfg_valget_poll<>::decode(view_of(payload, 7U), decoded));
    EXPECT_EQ(decoded.layer, config::UBX_CFG_VALGET_LAYER_RAM);
    EXPECT_TRUE(decoded.keys.empty());
}

TEST(UbxMessageCfgValget, PollKeysBeyondCapacityAreSkipped)
{
    uint8_t payload[16U] = {cfg_valget_poll<>::version, 2U, 0x05U, 0U};
    castle::write_le32(&payload[4U], 0xAU);
    castle::write_le32(&payload[8U], 0xBU);
    castle::write_le32(&payload[12U], 0xCU);
    cfg_valget_poll<2U> decoded{};
    ASSERT_TRUE(cfg_valget_poll<2U>::decode(view_of(payload, 16U), decoded));
    EXPECT_EQ(decoded.layer, config::UBX_CFG_VALGET_LAYER_FLASH);
    EXPECT_EQ(decoded.position, 5U);
    ASSERT_EQ(decoded.keys.size(), 2U);
    EXPECT_EQ(decoded.keys[1U], 0xBU);
}

TEST(UbxMessageCfgValget, ResponseRoundTripPreservesEveryField)
{
    cfg_valget<> original{};
    original.layer = config::UBX_CFG_VALGET_LAYER_BBR;
    original.position = 0x0140U;
    ASSERT_EQ(original.entries.push_back(make_entry(KEY_U1, 0x5AU)), castle::status::ok);
    ASSERT_EQ(original.entries.push_back(make_entry(KEY_U2, 0xC3D4U)), castle::status::ok);
    ASSERT_EQ(original.entries.push_back(make_entry(KEY_U4, 0x89ABCDEFU)), castle::status::ok);
    ASSERT_EQ(original.entries.push_back(make_entry(KEY_U8, 0xFEDCBA9876543210ULL)), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 4U + 5U + 6U + 8U + 12U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD], cfg_valget<>::version);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(cfg_valget<>::matches(raw));
    EXPECT_FALSE(cfg_valget_poll<>::matches(raw));

    cfg_valget<> decoded{};
    ASSERT_TRUE(cfg_valget<>::decode(raw, decoded));
    EXPECT_EQ(decoded.layer, config::UBX_CFG_VALGET_LAYER_BBR);
    EXPECT_EQ(decoded.position, 0x0140U);
    ASSERT_EQ(decoded.entries.size(), 4U);
    for (castle::size_type index = 0U; index < 4U; ++index)
    {
        EXPECT_EQ(decoded.entries[index].key_id, original.entries[index].key_id);
        EXPECT_EQ(decoded.entries[index].value, original.entries[index].value);
    }
}

TEST(UbxMessageCfgValget, ResponseDecodesFieldsAtSpecOffsets)
{
    uint8_t payload[4U + 5U + 6U + 8U + 12U] = {};
    payload[0U] = cfg_valget<>::version;
    payload[1U] = config::UBX_CFG_VALGET_LAYER_FLASH;
    castle::write_le16(&payload[2U], 0x1234U);
    castle::write_le32(&payload[4U], KEY_U1);
    payload[8U] = 0x11U;
    castle::write_le32(&payload[9U], KEY_U2);
    castle::write_le16(&payload[13U], 0x2233U);
    castle::write_le32(&payload[15U], KEY_U4);
    castle::write_le32(&payload[19U], 0x44556677U);
    castle::write_le32(&payload[23U], KEY_U8);
    castle::write_le64(&payload[27U], 0x8899AABBCCDDEEFFULL);

    cfg_valget<8U> decoded{};
    ASSERT_TRUE(cfg_valget<8U>::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.layer, config::UBX_CFG_VALGET_LAYER_FLASH);
    EXPECT_EQ(decoded.position, 0x1234U);
    ASSERT_EQ(decoded.entries.size(), 4U);
    EXPECT_EQ(decoded.entries[0U].key_id, KEY_U1);
    EXPECT_EQ(decoded.entries[0U].value, 0x11U);
    EXPECT_EQ(decoded.entries[1U].key_id, KEY_U2);
    EXPECT_EQ(decoded.entries[1U].value, 0x2233U);
    EXPECT_EQ(decoded.entries[2U].key_id, KEY_U4);
    EXPECT_EQ(decoded.entries[2U].value, 0x44556677U);
    EXPECT_EQ(decoded.entries[3U].key_id, KEY_U8);
    EXPECT_EQ(decoded.entries[3U].value, 0x8899AABBCCDDEEFFULL);
}

TEST(UbxMessageCfgValget, ResponseEntriesBeyondCapacityAreSkipped)
{
    uint8_t payload[4U + 5U + 6U + 5U] = {cfg_valget<>::version, 0U, 0U, 0U};
    castle::write_le32(&payload[4U], KEY_U1);
    payload[8U] = 0xAAU;
    castle::write_le32(&payload[9U], KEY_U2);
    castle::write_le16(&payload[13U], 0xBBCCU);
    castle::write_le32(&payload[15U], KEY_U1);
    payload[19U] = 0xDDU;

    cfg_valget<1U> decoded{};
    ASSERT_TRUE(cfg_valget<1U>::decode(view_of(payload, sizeof(payload)), decoded));
    ASSERT_EQ(decoded.entries.size(), 1U);
    EXPECT_EQ(decoded.entries[0U].value, 0xAAU);
}

TEST(UbxMessageCfgValget, ResponseRejectsUnknownKeySizeAndTruncation)
{
    cfg_valget<> decoded{};

    uint8_t unknown[9U] = {cfg_valget<>::version, 0U, 0U, 0U};
    castle::write_le32(&unknown[4U], 0x00000001U);
    decoded.position = 7U;
    EXPECT_FALSE(cfg_valget<>::decode(view_of(unknown, sizeof(unknown)), decoded));
    EXPECT_EQ(decoded.position, 0U);
    EXPECT_TRUE(decoded.entries.empty());

    uint8_t truncated[10U] = {cfg_valget<>::version, 0U, 0U, 0U};
    castle::write_le32(&truncated[4U], KEY_U4);
    EXPECT_FALSE(cfg_valget<>::decode(view_of(truncated, sizeof(truncated)), decoded));

    uint8_t partial_key[6U] = {cfg_valget<>::version, 0U, 0U, 0U, 1U, 0U};
    EXPECT_FALSE(cfg_valget<>::decode(view_of(partial_key, sizeof(partial_key)), decoded));
}

TEST(UbxMessageCfgValget, RejectsWrongClassIdShortHeaderAndVersion)
{
    uint8_t payload[4U] = {cfg_valget<>::version, 0U, 0U, 0U};
    cfg_valget<> response{};
    cfg_valget_poll<> poll{};

    message_view raw = view_of(payload, 4U);
    EXPECT_TRUE(cfg_valget<>::decode(raw, response));

    raw.header.msg_id = UBX_ID_CFG_VALSET;
    EXPECT_FALSE(cfg_valget<>::matches(raw));
    EXPECT_FALSE(cfg_valget<>::decode(raw, response));
    raw.header.msg_id = UBX_ID_CFG_VALGET;
    raw.header.msg_class = UBX_CLASS_NAV;
    EXPECT_FALSE(cfg_valget<>::matches(raw));

    EXPECT_FALSE(cfg_valget<>::decode(view_of(payload, 3U), response));
    EXPECT_FALSE(cfg_valget<>::decode(view_of(nullptr, 0U), response));
    EXPECT_FALSE(cfg_valget_poll<>::decode(view_of(nullptr, 0U), poll));

    // Response bytes are never accepted as a poll and vice versa; unknown versions match neither.
    EXPECT_FALSE(cfg_valget_poll<>::decode(view_of(payload, 4U), poll));
    payload[0U] = cfg_valget_poll<>::version;
    EXPECT_FALSE(cfg_valget<>::decode(view_of(payload, 4U), response));
    EXPECT_TRUE(cfg_valget_poll<>::decode(view_of(payload, 4U), poll));
    payload[0U] = 0x02U;
    EXPECT_FALSE(cfg_valget<>::matches(view_of(payload, 4U)));
    EXPECT_FALSE(cfg_valget_poll<>::matches(view_of(payload, 4U)));
}

TEST(UbxMessageCfgValget, EncodeIntoTooSmallBufferReportsFull)
{
    cfg_valget_poll<> poll{};
    ASSERT_EQ(poll.keys.push_back(KEY_U1), castle::status::ok);
    uint8_t frame[UBX_FRAME_OVERHEAD + 8U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(poll, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    cfg_valget<> response{};
    ASSERT_EQ(response.entries.push_back(make_entry(KEY_U8, 1U)), castle::status::ok);
    uint8_t response_frame[UBX_FRAME_OVERHEAD + 16U];
    written = 99U;
    EXPECT_EQ(encode_message(response, response_frame, sizeof(response_frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageCfgValget, ResponseEncodeWithUnknownKeySizeFails)
{
    cfg_valget<> response{};
    ASSERT_EQ(response.entries.push_back(make_entry(0x70000001U, 1U)), castle::status::ok);
    uint8_t frame[32U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(response, frame, sizeof(frame), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

} // namespace
