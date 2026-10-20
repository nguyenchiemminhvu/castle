#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_rf.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(CASTLE_CONST uint8_t* payload, uint16_t length, uint8_t msg_class = UBX_CLASS_MON,
                     uint8_t msg_id = UBX_ID_MON_RF)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

mon_rf_block distinctive_block(uint8_t seed)
{
    mon_rf_block block{};
    block.block_id = static_cast<uint8_t>(seed & 0x01U);
    block.flags = static_cast<uint8_t>(0x02U + (seed & 0x01U));
    block.ant_status = mon_antenna_status::ok;
    block.ant_power = mon_antenna_power::on;
    block.post_status = 0x10203040U + seed;
    block.noise_per_ms = static_cast<uint16_t>(0x0102U + seed);
    block.agc_cnt = static_cast<uint16_t>(0x0304U + seed);
    block.jam_ind = static_cast<uint8_t>(0x50U + seed);
    block.ofs_i = static_cast<int8_t>(-10 - seed);
    block.mag_i = static_cast<uint8_t>(0x60U + seed);
    block.ofs_q = static_cast<int8_t>(20 + seed);
    block.mag_q = static_cast<uint8_t>(0x70U + seed);
    return block;
}

void expect_block_eq(CASTLE_CONST mon_rf_block& actual, CASTLE_CONST mon_rf_block& expected)
{
    EXPECT_EQ(actual.block_id, expected.block_id);
    EXPECT_EQ(actual.flags, expected.flags);
    EXPECT_EQ(actual.ant_status, expected.ant_status);
    EXPECT_EQ(actual.ant_power, expected.ant_power);
    EXPECT_EQ(actual.post_status, expected.post_status);
    EXPECT_EQ(actual.noise_per_ms, expected.noise_per_ms);
    EXPECT_EQ(actual.agc_cnt, expected.agc_cnt);
    EXPECT_EQ(actual.jam_ind, expected.jam_ind);
    EXPECT_EQ(actual.ofs_i, expected.ofs_i);
    EXPECT_EQ(actual.mag_i, expected.mag_i);
    EXPECT_EQ(actual.ofs_q, expected.ofs_q);
    EXPECT_EQ(actual.mag_q, expected.mag_q);
}

/** Writes one spec-offset block at `base` (offset of `blockId`). */
void write_block(uint8_t* payload, castle::size_type base, uint8_t marker)
{
    payload[base + 0U] = 1U;          // blockId: L2/L5
    payload[base + 1U] = 0xFDU;       // flags: jammingState = 1, upper bits set
    payload[base + 2U] = 3U;          // antStatus: SHORT
    payload[base + 3U] = 2U;          // antPower: DONTKNOW
    castle::write_le32(&payload[base + 4U], 0xDEAD0000U + marker);
    payload[base + 8U] = 0xEEU;       // reserved1
    payload[base + 11U] = 0xEEU;
    castle::write_le16(&payload[base + 12U], static_cast<uint16_t>(0x0A00U + marker));
    castle::write_le16(&payload[base + 14U], static_cast<uint16_t>(0x1F00U + marker));
    payload[base + 16U] = marker;     // jamInd
    payload[base + 17U] = 0x80U;      // ofsI = -128
    payload[base + 18U] = 0xFFU;      // magI
    payload[base + 19U] = 0x7FU;      // ofsQ = 127
    payload[base + 20U] = 0x11U;      // magQ
    payload[base + 21U] = 0xEEU;      // reserved2
    payload[base + 23U] = 0xEEU;
}

TEST(UbxMessageMonRf, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_rf<>::msg_class, 0x0AU);
    EXPECT_EQ(mon_rf<>::msg_id, 0x38U);
    EXPECT_EQ(mon_rf<>::header_length, 4U);
    EXPECT_EQ(mon_rf<>::block_length, 24U);
    EXPECT_EQ(mon_rf_block::BLOCK_ID_L1, 0U);
    EXPECT_EQ(mon_rf_block::BLOCK_ID_L2_L5, 1U);
    EXPECT_EQ(static_cast<uint8_t>(mon_jamming_state::unknown), 0U);
    EXPECT_EQ(static_cast<uint8_t>(mon_jamming_state::ok), 1U);
    EXPECT_EQ(static_cast<uint8_t>(mon_jamming_state::warning), 2U);
    EXPECT_EQ(static_cast<uint8_t>(mon_jamming_state::critical), 3U);
}

TEST(UbxMessageMonRf, RoundTripThroughFrame)
{
    mon_rf<> message{};
    message.version = 0x00U;
    message.n_blocks = 99U; // ignored by encode: written from blocks.size()
    ASSERT_EQ(message.blocks.push_back(distinctive_block(1U)), castle::status::ok);
    ASSERT_EQ(message.blocks.push_back(distinctive_block(2U)), castle::status::ok);

    uint8_t frame[128U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(4U + (2U * 24U)));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 1U], 2U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    ASSERT_TRUE(mon_rf<>::matches(raw));

    mon_rf<> decoded{};
    ASSERT_TRUE(mon_rf<>::decode(raw, decoded));
    EXPECT_EQ(decoded.version, 0x00U);
    EXPECT_EQ(decoded.n_blocks, 2U);
    ASSERT_EQ(decoded.blocks.size(), 2U);
    expect_block_eq(decoded.blocks[0U], distinctive_block(1U));
    expect_block_eq(decoded.blocks[1U], distinctive_block(2U));
}

TEST(UbxMessageMonRf, HandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[4U + (2U * 24U)] = {};
    payload[0U] = 0x00U;
    payload[1U] = 2U;
    payload[2U] = 0xEEU; // reserved0
    payload[3U] = 0xEEU;
    payload[4U] = 0U;    // block 0: blockId L1, everything else zero
    write_block(payload, 4U + 24U, 0x42U);

    mon_rf<> decoded{};
    ASSERT_TRUE(mon_rf<>::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.n_blocks, 2U);
    ASSERT_EQ(decoded.blocks.size(), 2U);
    EXPECT_EQ(decoded.blocks[0U].block_id, mon_rf_block::BLOCK_ID_L1);
    EXPECT_EQ(decoded.blocks[0U].jamming_state(), mon_jamming_state::unknown);

    CASTLE_CONST mon_rf_block& block = decoded.blocks[1U];
    EXPECT_EQ(block.block_id, mon_rf_block::BLOCK_ID_L2_L5);
    EXPECT_EQ(block.flags, 0xFDU);
    EXPECT_EQ(block.jamming_state(), mon_jamming_state::ok);
    EXPECT_EQ(block.ant_status, mon_antenna_status::short_circuit);
    EXPECT_EQ(block.ant_power, mon_antenna_power::dont_know);
    EXPECT_EQ(block.post_status, 0xDEAD0042U);
    EXPECT_EQ(block.noise_per_ms, 0x0A42U);
    EXPECT_EQ(block.agc_cnt, 0x1F42U);
    EXPECT_EQ(block.jam_ind, 0x42U);
    EXPECT_EQ(block.ofs_i, -128);
    EXPECT_EQ(block.mag_i, 0xFFU);
    EXPECT_EQ(block.ofs_q, 127);
    EXPECT_EQ(block.mag_q, 0x11U);
}

TEST(UbxMessageMonRf, JammingStateAccessorCoversEveryValue)
{
    mon_rf_block block{};
    block.flags = 0xFCU; // bits outside jammingState are ignored
    EXPECT_EQ(block.jamming_state(), mon_jamming_state::unknown);
    block.flags = 0x01U;
    EXPECT_EQ(block.jamming_state(), mon_jamming_state::ok);
    block.flags = 0x02U;
    EXPECT_EQ(block.jamming_state(), mon_jamming_state::warning);
    block.flags = mon_rf_block::FLAGS_JAMMING_STATE;
    EXPECT_EQ(block.jamming_state(), mon_jamming_state::critical);
}

TEST(UbxMessageMonRf, HeaderOnlyPayloadDecodesWithNoBlocks)
{
    uint8_t payload[4U] = {0x00U, 0U, 0U, 0U};
    mon_rf<> decoded{};
    ASSERT_TRUE(mon_rf<>::decode(view_of(payload, 4U), decoded));
    EXPECT_EQ(decoded.n_blocks, 0U);
    EXPECT_TRUE(decoded.blocks.empty());
}

TEST(UbxMessageMonRf, CapacityOverflowKeepsWireCountAndStoresPrefix)
{
    uint8_t payload[4U + (3U * 24U)] = {};
    payload[1U] = 3U;
    write_block(payload, 4U, 0x01U);
    write_block(payload, 28U, 0x02U);
    write_block(payload, 52U, 0x03U);

    mon_rf<1U> decoded{};
    ASSERT_TRUE(mon_rf<1U>::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.n_blocks, 3U);
    ASSERT_EQ(decoded.blocks.size(), 1U);
    EXPECT_EQ(decoded.blocks[0U].post_status, 0xDEAD0001U);
    EXPECT_EQ(decoded.blocks[0U].jam_ind, 0x01U);
}

TEST(UbxMessageMonRf, RejectsGroupLengthMismatch)
{
    uint8_t payload[4U + (2U * 24U) + 1U] = {};
    payload[1U] = 2U;
    write_block(payload, 4U, 0x01U);

    mon_rf<> decoded{};
    EXPECT_FALSE(mon_rf<>::decode(view_of(payload, 4U + 24U), decoded)); // one block short
    EXPECT_EQ(decoded.n_blocks, 0U);
    EXPECT_TRUE(decoded.blocks.empty());
    EXPECT_FALSE(mon_rf<>::decode(view_of(payload, sizeof(payload)), decoded)); // one byte long
    EXPECT_EQ(decoded.n_blocks, 0U);

    payload[1U] = 1U;
    EXPECT_FALSE(mon_rf<>::decode(view_of(payload, 4U + 23U), decoded)); // partial block
    EXPECT_TRUE(decoded.blocks.empty());
}

TEST(UbxMessageMonRf, RejectsShortHeader)
{
    uint8_t payload[3U] = {0x00U, 0U, 0U};
    mon_rf<> decoded{};
    EXPECT_FALSE(mon_rf<>::decode(view_of(payload, 3U), decoded));
    message_view empty{{UBX_CLASS_MON, UBX_ID_MON_RF, 0U}, array_view<CASTLE_CONST uint8_t>(), checksum{}};
    EXPECT_FALSE(mon_rf<>::decode(empty, decoded));
}

TEST(UbxMessageMonRf, RejectsWrongClassOrId)
{
    uint8_t payload[4U] = {0x00U, 0U, 0U, 0U};
    mon_rf<> decoded{};
    EXPECT_FALSE(mon_rf<>::matches(view_of(payload, 4U, UBX_CLASS_NAV, UBX_ID_MON_RF)));
    EXPECT_FALSE(mon_rf<>::decode(view_of(payload, 4U, UBX_CLASS_NAV, UBX_ID_MON_RF), decoded));
    EXPECT_FALSE(mon_rf<>::matches(view_of(payload, 4U, UBX_CLASS_MON, UBX_ID_MON_HW3)));
    EXPECT_FALSE(mon_rf<>::decode(view_of(payload, 4U, UBX_CLASS_MON, UBX_ID_MON_HW3), decoded));
}

TEST(UbxMessageMonRf, EncodeIntoTooSmallBufferReportsFull)
{
    mon_rf<> message{};
    ASSERT_EQ(message.blocks.push_back(distinctive_block(1U)), castle::status::ok);
    uint8_t frame[frame_size(4U + 23U)] = {};
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
