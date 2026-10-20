#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_ver.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;
using castle::container::string_view;

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

void put_text(uint8_t* destination, CASTLE_CONST char* text)
{
    for (castle::size_type index = 0U; text[index] != '\0'; ++index)
    {
        destination[index] = static_cast<uint8_t>(text[index]);
    }
}

TEST(UbxMessageMonVer, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_ver<>::msg_class, UBX_CLASS_MON);
    EXPECT_EQ(mon_ver<>::msg_id, UBX_ID_MON_VER);
    EXPECT_EQ(mon_ver<>::sw_version_len, 30U);
    EXPECT_EQ(mon_ver<>::hw_version_len, 10U);
    EXPECT_EQ(mon_ver<>::extension_len, 30U);
    EXPECT_EQ(mon_ver<>::header_length, 40U);
    EXPECT_EQ(mon_ver<>::block_length, 30U);
    EXPECT_EQ(mon_ver<>().extensions.capacity(), 10U);
    EXPECT_EQ(mon_ver<>().sw_version.capacity(), 30U);
    EXPECT_EQ(mon_ver<>().hw_version.capacity(), 10U);
}

TEST(UbxMessageMonVer, RoundTripPreservesEveryField)
{
    mon_ver<4U> message{};
    message.sw_version = "EXT CORE 1.00 (a59682)";
    message.hw_version = "00190000";
    ASSERT_EQ(message.extensions.push_back("ROM BASE 0x118B2060"), castle::status::ok);
    ASSERT_EQ(message.extensions.push_back("FWVER=HPS 1.30"), castle::status::ok);
    ASSERT_EQ(message.extensions.push_back("PROTVER=33.30"), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 40U + 90U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    // NUL padding fills each CH[] field.
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 22U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 29U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 30U], static_cast<uint8_t>('0'));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 39U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 40U], static_cast<uint8_t>('R'));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 70U], static_cast<uint8_t>('F'));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 100U], static_cast<uint8_t>('P'));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    mon_ver<4U> out{};
    ASSERT_TRUE(mon_ver<4U>::decode(raw, out));
    EXPECT_EQ(out.sw_version, message.sw_version);
    EXPECT_EQ(out.hw_version, message.hw_version);
    ASSERT_EQ(out.extensions.size(), 3U);
    EXPECT_EQ(out.extensions[0U], string_view("ROM BASE 0x118B2060"));
    EXPECT_EQ(out.extensions[1U], string_view("FWVER=HPS 1.30"));
    EXPECT_EQ(out.extensions[2U], string_view("PROTVER=33.30"));
}

TEST(UbxMessageMonVer, HandBuiltPayloadMatchesSpecOffsets)
{
    uint8_t payload[100U] = {};
    put_text(&payload[0U], "ROM CORE 1.00");
    put_text(&payload[30U], "00190000");
    put_text(&payload[40U], "MOD=ZED-F9R");
    put_text(&payload[70U], "GPS;GLO;GAL;BDS");
    mon_ver<> out{};
    ASSERT_TRUE(mon_ver<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_VER, payload, 100U), out));
    EXPECT_EQ(out.sw_version, string_view("ROM CORE 1.00"));
    EXPECT_EQ(out.hw_version, string_view("00190000"));
    ASSERT_EQ(out.extensions.size(), 2U);
    EXPECT_EQ(out.extensions[0U], string_view("MOD=ZED-F9R"));
    EXPECT_EQ(out.extensions[1U], string_view("GPS;GLO;GAL;BDS"));
}

TEST(UbxMessageMonVer, FullWidthFieldsWithoutTerminatorAreKept)
{
    uint8_t payload[70U] = {};
    for (castle::size_type i = 0U; i < 70U; ++i)
    {
        payload[i] = static_cast<uint8_t>('A' + (i % 26U));
    }
    mon_ver<> out{};
    ASSERT_TRUE(mon_ver<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_VER, payload, 70U), out));
    EXPECT_EQ(out.sw_version.size(), 30U);
    EXPECT_EQ(out.sw_version[0U], 'A');
    EXPECT_EQ(out.sw_version[29U], static_cast<char>('A' + 3U));
    EXPECT_EQ(out.hw_version.size(), 10U);
    EXPECT_EQ(out.hw_version[0U], static_cast<char>('A' + 4U));
    ASSERT_EQ(out.extensions.size(), 1U);
    EXPECT_EQ(out.extensions[0U].size(), 30U);
    EXPECT_EQ(out.extensions[0U][0U], static_cast<char>('A' + 14U));
}

TEST(UbxMessageMonVer, NoExtensionsIsValid)
{
    uint8_t payload[40U] = {};
    put_text(&payload[0U], "SW");
    put_text(&payload[30U], "HW");
    mon_ver<> out{};
    ASSERT_TRUE(mon_ver<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_VER, payload, 40U), out));
    EXPECT_EQ(out.sw_version, string_view("SW"));
    EXPECT_EQ(out.hw_version, string_view("HW"));
    EXPECT_TRUE(out.extensions.empty());
}

TEST(UbxMessageMonVer, RejectsWrongClassOrId)
{
    uint8_t payload[40U] = {};
    mon_ver<> out{};
    EXPECT_FALSE(mon_ver<>::matches(make_view(UBX_CLASS_NAV, UBX_ID_MON_VER, payload, 40U)));
    EXPECT_FALSE(mon_ver<>::decode(make_view(UBX_CLASS_NAV, UBX_ID_MON_VER, payload, 40U), out));
    EXPECT_FALSE(mon_ver<>::matches(make_view(UBX_CLASS_MON, UBX_ID_MON_IO, payload, 40U)));
    EXPECT_FALSE(mon_ver<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_IO, payload, 40U), out));
    EXPECT_TRUE(mon_ver<>::matches(make_view(UBX_CLASS_MON, UBX_ID_MON_VER, payload, 40U)));
}

TEST(UbxMessageMonVer, RejectsShortAndMisalignedLengthsAndResets)
{
    uint8_t payload[100U] = {};
    put_text(&payload[0U], "SW");
    mon_ver<> out{};
    for (uint16_t length : {0U, 39U, 41U, 69U, 71U, 99U})
    {
        out.sw_version = "stale";
        ASSERT_EQ(out.extensions.push_back("stale"), castle::status::ok);
        EXPECT_FALSE(mon_ver<>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_VER, payload, length), out)) << length;
        EXPECT_TRUE(out.sw_version.empty()) << length;
        EXPECT_TRUE(out.extensions.empty()) << length;
    }
}

TEST(UbxMessageMonVer, RejectsDeclaredLengthLongerThanView)
{
    uint8_t payload[50U] = {};
    put_text(&payload[0U], "SW");
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_VER, 70U}, array_view<CASTLE_CONST uint8_t>(payload, 50U), checksum{}};
    mon_ver<> out{};
    EXPECT_FALSE(mon_ver<>::decode(raw, out));
    EXPECT_TRUE(out.sw_version.empty());
    EXPECT_TRUE(out.extensions.empty());
}

TEST(UbxMessageMonVer, CapacityOverflowKeepsFirstExtensions)
{
    uint8_t payload[130U] = {};
    put_text(&payload[40U], "first");
    put_text(&payload[70U], "second");
    put_text(&payload[100U], "third");
    mon_ver<2U> out{};
    ASSERT_TRUE(mon_ver<2U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_VER, payload, 130U), out));
    ASSERT_EQ(out.extensions.size(), 2U);
    EXPECT_EQ(out.extensions[0U], string_view("first"));
    EXPECT_EQ(out.extensions[1U], string_view("second"));
}

TEST(UbxMessageMonVer, EncodePollAndTooSmallBuffer)
{
    uint8_t frame[UBX_FRAME_OVERHEAD + 70U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_poll<mon_ver<>>(frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_MON);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_MON_VER);

    mon_ver<> message{};
    ASSERT_EQ(message.extensions.push_back("x"), castle::status::ok);
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
    EXPECT_EQ(encode_message(message, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
