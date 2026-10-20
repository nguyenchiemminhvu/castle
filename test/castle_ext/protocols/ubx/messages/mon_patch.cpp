#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_patch.hpp"
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

mon_patch_entry make_entry(uint32_t seed)
{
    mon_patch_entry entry{};
    entry.patch_info = 0x10203040U + seed;
    entry.comparator = 0x50607080U + seed;
    entry.patch_address = 0x90A0B0C0U + seed;
    entry.patch_data = 0xD0E0F000U + seed;
    return entry;
}

TEST(UbxMessageMonPatch, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_patch<>::msg_class, UBX_CLASS_MON);
    EXPECT_EQ(mon_patch<>::msg_id, UBX_ID_MON_PATCH);
    EXPECT_EQ(mon_patch<>::header_length, 4U);
    EXPECT_EQ(mon_patch<>::block_length, 16U);
    EXPECT_EQ(mon_patch_entry::PATCH_INFO_ACTIVATED, 0x1U);
    EXPECT_EQ(mon_patch_entry::PATCH_INFO_LOCATION, 0x6U);
    EXPECT_EQ(mon_patch_entry::PATCH_INFO_LOCATION_SHIFT, 1U);
}

TEST(UbxMessageMonPatch, RoundTripPreservesEveryField)
{
    mon_patch<4U> message{};
    message.version = 0x0102U;
    message.n_entries = 77U; // ignored on encode: count comes from entries.size()
    ASSERT_EQ(message.entries.push_back(make_entry(1U)), castle::status::ok);
    ASSERT_EQ(message.entries.push_back(make_entry(2U)), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 36U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_PAYLOAD + 2U]), 2U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    mon_patch<4U> out{};
    ASSERT_TRUE(mon_patch<4U>::decode(raw, out));
    EXPECT_EQ(out.version, 0x0102U);
    EXPECT_EQ(out.n_entries, 2U);
    ASSERT_EQ(out.entries.size(), 2U);
    for (castle::size_type i = 0U; i < 2U; ++i)
    {
        CASTLE_CONST mon_patch_entry expected = make_entry(static_cast<uint32_t>(i + 1U));
        EXPECT_EQ(out.entries[i].patch_info, expected.patch_info);
        EXPECT_EQ(out.entries[i].comparator, expected.comparator);
        EXPECT_EQ(out.entries[i].patch_address, expected.patch_address);
        EXPECT_EQ(out.entries[i].patch_data, expected.patch_data);
    }
}

TEST(UbxMessageMonPatch, HandBuiltPayloadMatchesSpecOffsets)
{
    uint8_t payload[36U] = {};
    castle::write_le16(&payload[0U], 0x0001U);
    castle::write_le16(&payload[2U], 2U);
    castle::write_le32(&payload[4U], 0x00000003U);
    castle::write_le32(&payload[8U], 0x11111111U);
    castle::write_le32(&payload[12U], 0x22222222U);
    castle::write_le32(&payload[16U], 0x33333333U);
    castle::write_le32(&payload[20U], 0x00000006U);
    castle::write_le32(&payload[24U], 0x44444444U);
    castle::write_le32(&payload[28U], 0x55555555U);
    castle::write_le32(&payload[32U], 0x66666666U);
    mon_patch<> out{};
    ASSERT_TRUE(mon_patch<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 36U), out));
    EXPECT_EQ(out.version, 1U);
    EXPECT_EQ(out.n_entries, 2U);
    ASSERT_EQ(out.entries.size(), 2U);
    EXPECT_EQ(out.entries[0U].patch_info, 3U);
    EXPECT_EQ(out.entries[0U].comparator, 0x11111111U);
    EXPECT_EQ(out.entries[0U].patch_address, 0x22222222U);
    EXPECT_EQ(out.entries[0U].patch_data, 0x33333333U);
    EXPECT_TRUE(out.entries[0U].activated());
    EXPECT_EQ(out.entries[0U].location(), mon_patch_location::rom);
    EXPECT_EQ(out.entries[1U].patch_info, 6U);
    EXPECT_EQ(out.entries[1U].comparator, 0x44444444U);
    EXPECT_EQ(out.entries[1U].patch_address, 0x55555555U);
    EXPECT_EQ(out.entries[1U].patch_data, 0x66666666U);
    EXPECT_FALSE(out.entries[1U].activated());
    EXPECT_EQ(out.entries[1U].location(), mon_patch_location::file_system);
}

TEST(UbxMessageMonPatch, PatchInfoAccessorsDecodeEveryLocation)
{
    mon_patch_entry entry{};
    EXPECT_FALSE(entry.activated());
    EXPECT_EQ(entry.location(), mon_patch_location::efuse);
    entry.patch_info = 0x1U;
    EXPECT_TRUE(entry.activated());
    EXPECT_EQ(entry.location(), mon_patch_location::efuse);
    entry.patch_info = 0x2U;
    EXPECT_FALSE(entry.activated());
    EXPECT_EQ(entry.location(), mon_patch_location::rom);
    entry.patch_info = 0x4U;
    EXPECT_EQ(entry.location(), mon_patch_location::bbr);
    entry.patch_info = 0x6U;
    EXPECT_EQ(entry.location(), mon_patch_location::file_system);
    // Unrelated high bits never leak into either accessor.
    entry.patch_info = 0xFFFFFFF8U;
    EXPECT_FALSE(entry.activated());
    EXPECT_EQ(entry.location(), mon_patch_location::efuse);
    entry.patch_info = 0xFFFFFFFFU;
    EXPECT_TRUE(entry.activated());
    EXPECT_EQ(entry.location(), mon_patch_location::file_system);
}

TEST(UbxMessageMonPatch, ZeroEntriesIsValid)
{
    uint8_t payload[4U] = {0x01U, 0x00U, 0x00U, 0x00U};
    mon_patch<> out{};
    ASSERT_TRUE(mon_patch<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 4U), out));
    EXPECT_EQ(out.version, 1U);
    EXPECT_EQ(out.n_entries, 0U);
    EXPECT_TRUE(out.entries.empty());
}

TEST(UbxMessageMonPatch, RejectsWrongClassOrId)
{
    uint8_t payload[4U] = {};
    mon_patch<> out{};
    EXPECT_FALSE(mon_patch<>::matches(make_view(UBX_CLASS_NAV, UBX_ID_MON_PATCH, payload, 4U)));
    EXPECT_FALSE(mon_patch<>::decode(make_view(UBX_CLASS_NAV, UBX_ID_MON_PATCH, payload, 4U), out));
    EXPECT_FALSE(mon_patch<>::matches(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 4U)));
    EXPECT_FALSE(mon_patch<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 4U), out));
    EXPECT_TRUE(mon_patch<>::matches(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 4U)));
}

TEST(UbxMessageMonPatch, RejectsLengthMismatchAgainstDeclaredCount)
{
    uint8_t payload[40U] = {};
    castle::write_le16(&payload[0U], 1U);
    castle::write_le16(&payload[2U], 2U); // declares 2 entries -> 36 bytes
    mon_patch<> out{};
    EXPECT_FALSE(mon_patch<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 3U), out));
    EXPECT_FALSE(mon_patch<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 20U), out));
    EXPECT_FALSE(mon_patch<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 35U), out));
    EXPECT_FALSE(mon_patch<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 37U), out));
    EXPECT_FALSE(mon_patch<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 40U), out));
    EXPECT_EQ(out.version, 0U);
    EXPECT_EQ(out.n_entries, 0U);
    EXPECT_TRUE(mon_patch<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 36U), out));
}

TEST(UbxMessageMonPatch, RejectsDeclaredLengthLongerThanView)
{
    uint8_t payload[20U] = {};
    castle::write_le16(&payload[0U], 1U);
    castle::write_le16(&payload[2U], 2U);
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_PATCH, 36U}, array_view<CASTLE_CONST uint8_t>(payload, 20U), checksum{}};
    mon_patch<> out{};
    EXPECT_FALSE(mon_patch<>::decode(raw, out));
    EXPECT_EQ(out.n_entries, 0U);
    EXPECT_TRUE(out.entries.empty());
}

TEST(UbxMessageMonPatch, CapacityOverflowKeepsWireCount)
{
    uint8_t payload[52U] = {};
    castle::write_le16(&payload[2U], 3U);
    castle::write_le32(&payload[8U], 10U);
    castle::write_le32(&payload[24U], 20U);
    castle::write_le32(&payload[40U], 30U);
    mon_patch<2U> out{};
    ASSERT_TRUE(mon_patch<2U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 52U), out));
    EXPECT_EQ(out.n_entries, 3U);
    ASSERT_EQ(out.entries.size(), 2U);
    EXPECT_EQ(out.entries[0U].comparator, 10U);
    EXPECT_EQ(out.entries[1U].comparator, 20U);
}

TEST(UbxMessageMonPatch, EncodeIntoTooSmallBufferReportsFull)
{
    mon_patch<> message{};
    ASSERT_EQ(message.entries.push_back(make_entry(0U)), castle::status::ok);
    uint8_t frame[UBX_FRAME_OVERHEAD + 20U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
