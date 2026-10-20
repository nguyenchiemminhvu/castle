#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_span.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

mon_span_rf_block make_block(uint8_t seed)
{
    mon_span_rf_block block{};
    for (castle::size_type bin = 0U; bin < MON_SPAN_SPECTRUM_BINS; ++bin)
    {
        block.spectrum[bin] = static_cast<uint8_t>(bin + seed);
    }
    block.span = 0x01020304U + seed;
    block.res = 0x11121314U + seed;
    block.center = 0x21222324U + seed;
    block.pga = static_cast<uint8_t>(0x30U + seed);
    return block;
}

TEST(UbxMessageMonSpan, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_span<>::msg_class, UBX_CLASS_MON);
    EXPECT_EQ(mon_span<>::msg_id, UBX_ID_MON_SPAN);
    EXPECT_EQ(mon_span<>::header_length, 4U);
    EXPECT_EQ(mon_span<>::block_length, 272U);
    EXPECT_EQ(MON_SPAN_SPECTRUM_BINS, 256U);
}

TEST(UbxMessageMonSpan, RoundTripPreservesEveryField)
{
    mon_span<2U> message{};
    message.version = 0x5AU;
    message.num_rf_blocks = 9U; // ignored on encode: count comes from rf_blocks.size()
    ASSERT_EQ(message.rf_blocks.push_back(make_block(1U)), castle::status::ok);
    ASSERT_EQ(message.rf_blocks.push_back(make_block(2U)), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 4U + (2U * 272U)];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 1U], 2U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    mon_span<2U> out{};
    ASSERT_TRUE(mon_span<2U>::decode(raw, out));
    EXPECT_EQ(out.version, 0x5AU);
    EXPECT_EQ(out.num_rf_blocks, 2U);
    ASSERT_EQ(out.rf_blocks.size(), 2U);
    for (castle::size_type i = 0U; i < 2U; ++i)
    {
        CASTLE_CONST mon_span_rf_block expected = make_block(static_cast<uint8_t>(i + 1U));
        for (castle::size_type bin = 0U; bin < MON_SPAN_SPECTRUM_BINS; ++bin)
        {
            ASSERT_EQ(out.rf_blocks[i].spectrum[bin], expected.spectrum[bin]);
        }
        EXPECT_EQ(out.rf_blocks[i].span, expected.span);
        EXPECT_EQ(out.rf_blocks[i].res, expected.res);
        EXPECT_EQ(out.rf_blocks[i].center, expected.center);
        EXPECT_EQ(out.rf_blocks[i].pga, expected.pga);
    }
}

TEST(UbxMessageMonSpan, HandBuiltPayloadMatchesSpecOffsets)
{
    uint8_t payload[4U + 272U] = {};
    payload[0U] = 0U;
    payload[1U] = 1U;
    payload[2U] = 0xEEU; // reserved0 ignored
    payload[3U] = 0xEEU;
    for (castle::size_type i = 0U; i < 256U; ++i)
    {
        payload[4U + i] = static_cast<uint8_t>(255U - i);
    }
    castle::write_le32(&payload[260U], 0xA1A2A3A4U);
    castle::write_le32(&payload[264U], 0xB1B2B3B4U);
    castle::write_le32(&payload[268U], 0xC1C2C3C4U);
    payload[272U] = 0xD1U;
    payload[273U] = 0xEEU; // reserved1 ignored
    payload[275U] = 0xEEU;
    mon_span<> out{};
    ASSERT_TRUE(mon_span<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, sizeof(payload)), out));
    EXPECT_EQ(out.version, 0U);
    EXPECT_EQ(out.num_rf_blocks, 1U);
    ASSERT_EQ(out.rf_blocks.size(), 1U);
    EXPECT_EQ(out.rf_blocks[0U].spectrum[0U], 255U);
    EXPECT_EQ(out.rf_blocks[0U].spectrum[128U], 127U);
    EXPECT_EQ(out.rf_blocks[0U].spectrum[255U], 0U);
    EXPECT_EQ(out.rf_blocks[0U].span, 0xA1A2A3A4U);
    EXPECT_EQ(out.rf_blocks[0U].res, 0xB1B2B3B4U);
    EXPECT_EQ(out.rf_blocks[0U].center, 0xC1C2C3C4U);
    EXPECT_EQ(out.rf_blocks[0U].pga, 0xD1U);

    // Encoding writes zeros into both reserved fields.
    uint8_t frame[UBX_FRAME_OVERHEAD + sizeof(payload)];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(out, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 2U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 3U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 273U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 275U], 0U);
}

TEST(UbxMessageMonSpan, BinFrequencyFollowsSpecFormula)
{
    mon_span_rf_block block{};
    block.center = 1575420000U;
    block.span = 128000000U;
    EXPECT_DOUBLE_EQ(block.bin_frequency_hz(128U), 1575420000.0);
    EXPECT_DOUBLE_EQ(block.bin_frequency_hz(0U), 1575420000.0 - 64000000.0);
    EXPECT_DOUBLE_EQ(block.bin_frequency_hz(255U), 1575420000.0 + (128000000.0 * 127.0 / 256.0));
    EXPECT_DOUBLE_EQ(block.bin_frequency_hz(129U), 1575420000.0 + 500000.0);
    block.span = 0U;
    EXPECT_DOUBLE_EQ(block.bin_frequency_hz(0U), 1575420000.0);
}

TEST(UbxMessageMonSpan, ZeroBlocksIsValid)
{
    uint8_t payload[4U] = {0x00U, 0x00U, 0x00U, 0x00U};
    mon_span<> out{};
    ASSERT_TRUE(mon_span<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, 4U), out));
    EXPECT_EQ(out.num_rf_blocks, 0U);
    EXPECT_TRUE(out.rf_blocks.empty());
}

TEST(UbxMessageMonSpan, RejectsWrongClassOrId)
{
    uint8_t payload[4U] = {};
    mon_span<> out{};
    EXPECT_FALSE(mon_span<>::matches(make_view(UBX_CLASS_NAV, UBX_ID_MON_SPAN, payload, 4U)));
    EXPECT_FALSE(mon_span<>::decode(make_view(UBX_CLASS_NAV, UBX_ID_MON_SPAN, payload, 4U), out));
    EXPECT_FALSE(mon_span<>::matches(make_view(UBX_CLASS_MON, UBX_ID_MON_RF, payload, 4U)));
    EXPECT_FALSE(mon_span<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_RF, payload, 4U), out));
    EXPECT_TRUE(mon_span<>::matches(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, 4U)));
}

TEST(UbxMessageMonSpan, DeclaredCountMustMatchLength)
{
    uint8_t payload[4U + (3U * 272U)] = {};
    payload[0U] = 7U;
    mon_span<4U> out{};

    // Length implies 2 blocks but header declares 1: rejected (the old code accepted this).
    payload[1U] = 1U;
    EXPECT_FALSE(mon_span<4U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, 4U + 544U), out));
    EXPECT_EQ(out.version, 0U);
    EXPECT_EQ(out.num_rf_blocks, 0U);

    // Header declares 3 but only 2 blocks present.
    payload[1U] = 3U;
    EXPECT_FALSE(mon_span<4U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, 4U + 544U), out));
    // Header declares 1 but zero blocks follow.
    payload[1U] = 1U;
    EXPECT_FALSE(mon_span<4U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, 4U), out));
    // Off-by-one lengths and a truncated header.
    EXPECT_FALSE(mon_span<4U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, 275U), out));
    EXPECT_FALSE(mon_span<4U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, 277U), out));
    EXPECT_FALSE(mon_span<4U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, 3U), out));
    EXPECT_FALSE(mon_span<4U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, 0U), out));

    // Matching count and length decodes.
    payload[1U] = 2U;
    EXPECT_TRUE(mon_span<4U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, 4U + 544U), out));
    EXPECT_EQ(out.version, 7U);
    EXPECT_EQ(out.rf_blocks.size(), 2U);
}

TEST(UbxMessageMonSpan, RejectsDeclaredLengthLongerThanView)
{
    uint8_t payload[100U] = {};
    payload[1U] = 1U;
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_SPAN, 276U}, array_view<CASTLE_CONST uint8_t>(payload, 100U), checksum{}};
    mon_span<> out{};
    EXPECT_FALSE(mon_span<>::decode(raw, out));
    EXPECT_EQ(out.num_rf_blocks, 0U);
    EXPECT_TRUE(out.rf_blocks.empty());
}

TEST(UbxMessageMonSpan, CapacityOverflowKeepsWireCount)
{
    uint8_t payload[4U + 544U] = {};
    payload[1U] = 2U;
    payload[272U] = 11U;       // block 0 pga
    payload[272U + 272U] = 22U; // block 1 pga
    mon_span<1U> out{};
    ASSERT_TRUE(mon_span<1U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPAN, payload, sizeof(payload)), out));
    EXPECT_EQ(out.num_rf_blocks, 2U);
    ASSERT_EQ(out.rf_blocks.size(), 1U);
    EXPECT_EQ(out.rf_blocks[0U].pga, 11U);
}

TEST(UbxMessageMonSpan, EncodeIntoTooSmallBufferReportsFull)
{
    mon_span<> message{};
    ASSERT_EQ(message.rf_blocks.push_back(make_block(0U)), castle::status::ok);
    uint8_t frame[UBX_FRAME_OVERHEAD + 276U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
