#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/detail/cfg_data.hpp"
#include "castle/utility/bytes.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;
using castle::container::vector;

static CASTLE_CONSTEXPR uint8_t TEST_ID = 0x7EU;

static CASTLE_CONSTEXPR uint32_t KEY_L = 0x10000001U;  // size code 1 -> 1 byte
static CASTLE_CONSTEXPR uint32_t KEY_U1 = 0x20000002U; // size code 2 -> 1 byte
static CASTLE_CONSTEXPR uint32_t KEY_U2 = 0x30000003U; // size code 3 -> 2 bytes
static CASTLE_CONSTEXPR uint32_t KEY_U4 = 0x40000004U; // size code 4 -> 4 bytes
static CASTLE_CONSTEXPR uint32_t KEY_U8 = 0x50000005U; // size code 5 -> 8 bytes
static CASTLE_CONSTEXPR uint32_t KEY_BAD = 0x60000006U; // size code 6 -> unknown

struct keys_message : detail::cfg_message<keys_message, TEST_ID, 0x02U>
{
    uint8_t tag = 0U;
    vector<uint32_t, 2U> keys{};

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.tag);
        stream.reserved(2U);
        detail::cfg_keys(stream, self.keys);
    }
};

struct data_message : detail::cfg_message<data_message, TEST_ID, 0x03U>
{
    vector<config::config_entry, 2U> entries{};

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.reserved(3U);
        detail::cfg_data(stream, self.entries);
    }
};

message_view view_of(uint8_t msg_id, CASTLE_CONST uint8_t* payload, castle::size_type length)
{
    return message_view{{UBX_CLASS_CFG, msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

TEST(UbxMessageDetailCfgData, BaseConstants)
{
    EXPECT_EQ(keys_message::msg_class, UBX_CLASS_CFG);
    EXPECT_EQ(keys_message::msg_id, TEST_ID);
    EXPECT_EQ(keys_message::version, 0x02U);
    EXPECT_EQ(keys_message::header_length, 4U);
    EXPECT_EQ(CFG_MAX_KEYS, 64U);
}

TEST(UbxMessageDetailCfgData, MatchesRequiresClassIdHeaderAndVersion)
{
    uint8_t payload[4U] = {0x02U, 0U, 0U, 0U};
    message_view raw = view_of(TEST_ID, payload, 4U);
    EXPECT_TRUE(keys_message::matches(raw));
    EXPECT_FALSE(data_message::matches(raw)); // other version

    raw.header.msg_id = TEST_ID + 1U;
    EXPECT_FALSE(keys_message::matches(raw));
    raw.header.msg_id = TEST_ID;
    raw.header.msg_class = UBX_CLASS_NAV;
    EXPECT_FALSE(keys_message::matches(raw));
    raw.header.msg_class = UBX_CLASS_CFG;

    EXPECT_FALSE(keys_message::matches(view_of(TEST_ID, payload, 3U)));
    EXPECT_FALSE(keys_message::matches(view_of(TEST_ID, nullptr, 0U)));

    payload[0U] = 0x00U;
    EXPECT_FALSE(keys_message::matches(raw));
}

TEST(UbxMessageDetailCfgData, KeysRoundTripWritesVersionFirst)
{
    keys_message original{};
    original.tag = 0xA5U;
    ASSERT_EQ(original.keys.push_back(0x11223344U), castle::status::ok);
    ASSERT_EQ(original.keys.push_back(0x55667788U), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 12U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(payload[0U], 0x02U);
    EXPECT_EQ(payload[1U], 0xA5U);
    EXPECT_EQ(payload[2U], 0U);
    EXPECT_EQ(payload[3U], 0U);
    EXPECT_EQ(castle::read_le32(&payload[4U]), 0x11223344U);
    EXPECT_EQ(castle::read_le32(&payload[8U]), 0x55667788U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    keys_message decoded{};
    ASSERT_TRUE(keys_message::decode(raw, decoded));
    EXPECT_EQ(decoded.tag, 0xA5U);
    ASSERT_EQ(decoded.keys.size(), 2U);
    EXPECT_EQ(decoded.keys[0U], 0x11223344U);
    EXPECT_EQ(decoded.keys[1U], 0x55667788U);
}

TEST(UbxMessageDetailCfgData, HeaderOnlyPayloadDecodesToNoKeys)
{
    uint8_t payload[4U] = {0x02U, 0x07U, 0U, 0U};
    keys_message decoded{};
    ASSERT_TRUE(keys_message::decode(view_of(TEST_ID, payload, 4U), decoded));
    EXPECT_EQ(decoded.tag, 0x07U);
    EXPECT_TRUE(decoded.keys.empty());
}

TEST(UbxMessageDetailCfgData, KeysBeyondCapacityAreConsumedNotStored)
{
    uint8_t payload[16U] = {0x02U, 0x01U, 0U, 0U};
    castle::write_le32(&payload[4U], 1U);
    castle::write_le32(&payload[8U], 2U);
    castle::write_le32(&payload[12U], 3U);
    keys_message decoded{};
    ASSERT_TRUE(keys_message::decode(view_of(TEST_ID, payload, 16U), decoded));
    ASSERT_EQ(decoded.keys.size(), 2U);
    EXPECT_EQ(decoded.keys[0U], 1U);
    EXPECT_EQ(decoded.keys[1U], 2U);
}

TEST(UbxMessageDetailCfgData, PartialKeyFailsAndResets)
{
    uint8_t payload[10U] = {0x02U, 0x01U, 0U, 0U};
    castle::write_le32(&payload[4U], 1U);
    keys_message decoded{};
    decoded.tag = 9U;
    EXPECT_FALSE(keys_message::decode(view_of(TEST_ID, payload, 10U), decoded));
    EXPECT_EQ(decoded.tag, 0U);
    EXPECT_TRUE(decoded.keys.empty());
}

TEST(UbxMessageDetailCfgData, DecodeRejectsWrongVersionAndResets)
{
    uint8_t payload[8U] = {0x03U, 0x01U, 0U, 0U, 1U, 0U, 0U, 0U};
    keys_message decoded{};
    decoded.tag = 9U;
    EXPECT_FALSE(keys_message::decode(view_of(TEST_ID, payload, 8U), decoded));
    EXPECT_EQ(decoded.tag, 0U);
    EXPECT_FALSE(keys_message::decode(view_of(TEST_ID + 1U, payload, 8U), decoded));
}

TEST(UbxMessageDetailCfgData, DataDecodesEveryValueWidthAtSpecOffsets)
{
    uint8_t payload[4U + 5U + 6U] = {0x03U, 0U, 0U, 0U};
    castle::write_le32(&payload[4U], KEY_U1);
    payload[8U] = 0xC3U;
    castle::write_le32(&payload[9U], KEY_U2);
    castle::write_le16(&payload[13U], 0xBEEFU);
    data_message decoded{};
    ASSERT_TRUE(data_message::decode(view_of(TEST_ID, payload, sizeof(payload)), decoded));
    ASSERT_EQ(decoded.entries.size(), 2U);
    EXPECT_EQ(decoded.entries[0U].key_id, KEY_U1);
    EXPECT_EQ(decoded.entries[0U].value.raw, 0xC3U);
    EXPECT_EQ(decoded.entries[1U].key_id, KEY_U2);
    EXPECT_EQ(decoded.entries[1U].value.raw, 0xBEEFU);

    uint8_t wide[4U + 8U + 12U] = {0x03U, 0U, 0U, 0U};
    castle::write_le32(&wide[4U], KEY_U4);
    castle::write_le32(&wide[8U], 0xDEADBEEFU);
    castle::write_le32(&wide[12U], KEY_U8);
    castle::write_le64(&wide[16U], 0x0123456789ABCDEFULL);
    ASSERT_TRUE(data_message::decode(view_of(TEST_ID, wide, sizeof(wide)), decoded));
    ASSERT_EQ(decoded.entries.size(), 2U);
    EXPECT_EQ(decoded.entries[0U].value.raw, 0xDEADBEEFU);
    EXPECT_EQ(decoded.entries[1U].value.raw, 0x0123456789ABCDEFULL);
}

TEST(UbxMessageDetailCfgData, DataRoundTripUsesKeyWidths)
{
    data_message original{};
    config::config_entry entry{};
    entry.key_id = KEY_L;
    entry.value = config::config_value(true);
    ASSERT_EQ(original.entries.push_back(entry), castle::status::ok);
    entry.key_id = KEY_U8;
    entry.value = config::config_value(static_cast<int64_t>(-2));
    ASSERT_EQ(original.entries.push_back(entry), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 4U + 5U + 12U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(castle::read_le32(&payload[4U]), KEY_L);
    EXPECT_EQ(payload[8U], 1U);
    EXPECT_EQ(castle::read_le32(&payload[9U]), KEY_U8);
    EXPECT_EQ(castle::read_le64(&payload[13U]), 0xFFFFFFFFFFFFFFFEULL);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    data_message decoded{};
    ASSERT_TRUE(data_message::decode(raw, decoded));
    ASSERT_EQ(decoded.entries.size(), 2U);
    EXPECT_TRUE(decoded.entries[0U].value.as_bool());
    EXPECT_EQ(decoded.entries[1U].value.as_i64(), -2);
}

TEST(UbxMessageDetailCfgData, DataPairsBeyondCapacityAreConsumedNotStored)
{
    uint8_t payload[4U + (3U * 5U)] = {0x03U, 0U, 0U, 0U};
    for (castle::size_type index = 0U; index < 3U; ++index)
    {
        castle::write_le32(&payload[4U + (index * 5U)], KEY_U1);
        payload[8U + (index * 5U)] = static_cast<uint8_t>(0x10U + index);
    }
    data_message decoded{};
    ASSERT_TRUE(data_message::decode(view_of(TEST_ID, payload, sizeof(payload)), decoded));
    ASSERT_EQ(decoded.entries.size(), 2U);
    EXPECT_EQ(decoded.entries[0U].value.raw, 0x10U);
    EXPECT_EQ(decoded.entries[1U].value.raw, 0x11U);
}

TEST(UbxMessageDetailCfgData, DataRejectsUnknownSizeTruncatedValueAndPartialKey)
{
    data_message decoded{};

    uint8_t unknown[9U] = {0x03U, 0U, 0U, 0U};
    castle::write_le32(&unknown[4U], KEY_BAD);
    EXPECT_FALSE(data_message::decode(view_of(TEST_ID, unknown, sizeof(unknown)), decoded));
    EXPECT_TRUE(decoded.entries.empty());

    uint8_t truncated[4U + 4U + 3U] = {0x03U, 0U, 0U, 0U};
    castle::write_le32(&truncated[4U], KEY_U4);
    EXPECT_FALSE(data_message::decode(view_of(TEST_ID, truncated, sizeof(truncated)), decoded));
    EXPECT_TRUE(decoded.entries.empty());

    uint8_t partial[4U + 5U + 2U] = {0x03U, 0U, 0U, 0U};
    castle::write_le32(&partial[4U], KEY_U1);
    EXPECT_FALSE(data_message::decode(view_of(TEST_ID, partial, sizeof(partial)), decoded));
    EXPECT_TRUE(decoded.entries.empty());
}

TEST(UbxMessageDetailCfgData, EncodeUnknownKeySizeLatchesInvalidArgument)
{
    data_message original{};
    config::config_entry entry{};
    entry.key_id = KEY_U1;
    ASSERT_EQ(original.entries.push_back(entry), castle::status::ok);
    entry.key_id = KEY_BAD;
    ASSERT_EQ(original.entries.push_back(entry), castle::status::ok);

    uint8_t frame[64U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageDetailCfgData, EncodeIntoTooSmallBufferReportsFull)
{
    keys_message original{};
    ASSERT_EQ(original.keys.push_back(1U), castle::status::ok);
    uint8_t frame[UBX_FRAME_OVERHEAD + 8U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(original, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageDetailCfgData, ValueHelpers)
{
    uint8_t bytes[8U] = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U};
    CASTLE_CONST array_view<CASTLE_CONST uint8_t> view(bytes, 8U);
    payload_reader one(view);
    EXPECT_EQ(detail::read_cfg_value(one, 1U), 0x01U);
    payload_reader two(view);
    EXPECT_EQ(detail::read_cfg_value(two, 2U), 0x0201U);
    payload_reader four(view);
    EXPECT_EQ(detail::read_cfg_value(four, 4U), 0x04030201U);
    payload_reader eight(view);
    EXPECT_EQ(detail::read_cfg_value(eight, 8U), 0x0807060504030201ULL);
    EXPECT_TRUE(eight.ok());
    EXPECT_EQ(eight.remaining(), 0U);

    EXPECT_EQ(detail::cfg_raw_value(static_cast<uint64_t>(42U)), 42U);
    EXPECT_EQ(detail::cfg_raw_value(config::config_value(static_cast<int8_t>(-1))), 0xFFU);
}

TEST(UbxMessageDetailCfgData, TransactionFieldAction)
{
    cfg_transaction_field field{};
    EXPECT_EQ(field.action(), cfg_transaction_action::transactionless);

    field.set_action(cfg_transaction_action::start);
    EXPECT_EQ(field.transaction, 1U);
    EXPECT_EQ(field.action(), cfg_transaction_action::start);
    field.set_action(cfg_transaction_action::ongoing);
    EXPECT_EQ(field.action(), cfg_transaction_action::ongoing);
    field.set_action(cfg_transaction_action::apply);
    EXPECT_EQ(field.transaction, 3U);

    field.transaction = 0xF4U; // reserved upper bits set, action 0
    EXPECT_EQ(field.action(), cfg_transaction_action::transactionless);
    field.set_action(cfg_transaction_action::apply);
    EXPECT_EQ(field.transaction, 0xF7U);
    field.set_action(cfg_transaction_action::transactionless);
    EXPECT_EQ(field.transaction, 0xF4U);

    EXPECT_EQ(static_cast<uint8_t>(cfg_transaction_action::transactionless), 0U);
    EXPECT_EQ(static_cast<uint8_t>(cfg_transaction_action::start), 1U);
    EXPECT_EQ(static_cast<uint8_t>(cfg_transaction_action::ongoing), 2U);
    EXPECT_EQ(static_cast<uint8_t>(cfg_transaction_action::apply), 3U);
    EXPECT_EQ(cfg_transaction_field::TRANSACTION_ACTION, 0x03U);
}

} // namespace
