#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/cfg_valget.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using namespace castle::protocols::ubx::config;
using castle::container::array_view;

static message_view make_view(uint8_t* payload, castle::size_type size)
{
    return message_view{{UBX_CLASS_CFG, UBX_ID_CFG_VALGET, static_cast<uint16_t>(size)},
                        array_view<CASTLE_CONST uint8_t>(payload, size), checksum{}};
}

TEST(UbxMessageCfgValgetMirror, DecodesOneByteTwoByteFourByteAndEightByteEntries)
{
    uint8_t payload[4U + (4U + 1U) + (4U + 2U) + (4U + 4U) + (4U + 8U)] = {};
    payload[0U] = UBX_VALGET_VERSION_RESP;
    payload[1U] = UBX_CFG_VALGET_LAYER_RAM;
    payload[2U] = 0x34U;
    payload[3U] = 0x12U;

    castle::size_type pos = 4U;
    const uint32_t keys[] = {
        (1UL << 28U) | 0x00000001U, (3UL << 28U) | 0x00000002U, (4UL << 28U) | 0x00000003U, (5UL << 28U) | 0x00000004U};
    const uint8_t sizes[] = {1U, 2U, 4U, 8U};
    const uint64_t values[] = {0x11U, 0x2233U, 0x44556677U, 0x8899AABBCCDDEEFFULL};

    for (castle::size_type i = 0U; i < 4U; ++i)
    {
        castle::write_le32(&payload[pos], keys[i]);
        pos += 4U;
        switch (sizes[i])
        {
            case 1U: payload[pos++] = static_cast<uint8_t>(values[i]); break;
            case 2U: castle::write_le16(&payload[pos], static_cast<uint16_t>(values[i])); pos += 2U; break;
            case 4U: castle::write_le32(&payload[pos], static_cast<uint32_t>(values[i])); pos += 4U; break;
            case 8U: castle::write_le64(&payload[pos], values[i]); pos += 8U; break;
            default: break;
        }
    }

    auto raw = make_view(payload, sizeof(payload));
    cfg_valget<8U> out;
    ASSERT_TRUE(cfg_valget<8U>::decode(raw, out));
    EXPECT_EQ(out.version, UBX_VALGET_VERSION_RESP);
    EXPECT_EQ(out.layer, UBX_CFG_VALGET_LAYER_RAM);
    EXPECT_EQ(out.position, 0x1234U);
    EXPECT_EQ(out.entry_count, 4U);
    EXPECT_EQ(out.entries[0U].value, 0x11U);
    EXPECT_EQ(out.entries[1U].value, 0x2233U);
    EXPECT_EQ(out.entries[2U].value, 0x44556677U);
    EXPECT_EQ(out.entries[3U].value, 0x8899AABBCCDDEEFFULL);
}

TEST(UbxMessageCfgValgetMirror, StopsAtUnknownKeyAndRejectsTruncatedValue)
{
    uint8_t payload[12U] = {};
    payload[0U] = 1U;
    castle::write_le32(&payload[4U], 0xFFFFFFFFU);
    auto raw = make_view(payload, 8U);
    cfg_valget<> out;
    ASSERT_TRUE(cfg_valget<>::decode(raw, out));
    EXPECT_EQ(out.entry_count, 0U);

    castle::write_le32(&payload[4U], (1UL << 28U) | 0x00000001U);
    raw.header.payload_length = 8U;
    EXPECT_FALSE(cfg_valget<>::decode(raw, out));
}

TEST(UbxMessageCfgValgetMirror, ValueSizeDelegatesToConfigurationKeyMetadata)
{
    EXPECT_EQ(cfg_valget<>::value_size((1UL << 28U) | 0x00000001U), 1U);
    EXPECT_EQ(cfg_valget<>::value_size(0x00000001U), 0U);
}

} // namespace

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageCfgValgetMirror, ClampsEntryStorageCapacity)
{
    uint8_t payload[14U] = {};
    payload[0U] = 1U;
    castle::write_le32(&payload[4U], (1UL << 28U) | 1U);
    payload[8U] = 0xAAU;
    castle::write_le32(&payload[9U], (2UL << 28U) | 2U);
    payload[13U] = 0xBBU;

    message_view raw{{UBX_CLASS_CFG, UBX_ID_CFG_VALGET, 14U},
                     array_view<CASTLE_CONST uint8_t>(payload, 14U), checksum{}};
    cfg_valget<1U> out;
    ASSERT_TRUE(cfg_valget<1U>::decode(raw, out));
    EXPECT_EQ(out.entry_count, 1U);
    EXPECT_EQ(out.entries[0U].value, 0xAAU);
}

} // namespace
