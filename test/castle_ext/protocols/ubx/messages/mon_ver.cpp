#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_ver.hpp"

namespace
{
using namespace castle_ext::protocols::ubx;
using namespace castle_ext::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageMonVerMirror, CopiesSoftwareHardwareAndExtensions)
{
    uint8_t payload[100U] = {};
    for (castle::size_type i = 0U; i < 30U; ++i)
    {
        payload[i] = static_cast<uint8_t>('A' + (i % 26U));
    }
    for (castle::size_type i = 0U; i < 10U; ++i)
    {
        payload[30U + i] = static_cast<uint8_t>('0' + i);
    }
    for (castle::size_type i = 40U; i < sizeof(payload); ++i)
    {
        payload[i] = static_cast<uint8_t>('a' + ((i - 40U) % 26U));
    }

    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_VER, 100U}, array_view<CASTLE_CONST uint8_t>(payload, 100U), checksum{}};
    mon_ver<2U> out;
    ASSERT_TRUE(mon_ver<2U>::decode(raw, out));
    EXPECT_EQ(out.sw_version[0U], 'A');
    EXPECT_EQ(out.hw_version[0U], '0');
    EXPECT_EQ(out.extension_count, 2U);
    EXPECT_EQ(out.extensions[0U][0U], 'a');
    EXPECT_EQ(out.extensions[1U][29U], static_cast<char>('a' + (59U % 26U)));
}

TEST(UbxMessageMonVerMirror, RejectsExtensionAlignmentError)
{
    uint8_t payload[41U] = {};
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_VER, 41U}, array_view<CASTLE_CONST uint8_t>(payload, 41U), checksum{}};
    mon_ver<> out;
    EXPECT_FALSE(mon_ver<>::decode(raw, out));
}

} // namespace

namespace
{
using namespace castle_ext::protocols::ubx;
using namespace castle_ext::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageMonVerMirror, ClampsExtensionCount)
{
    uint8_t payload[100U] = {};
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_VER, 100U},
                     array_view<CASTLE_CONST uint8_t>(payload, 100U), checksum{}};
    mon_ver<1U> out;
    ASSERT_TRUE(mon_ver<1U>::decode(raw, out));
    EXPECT_EQ(out.extension_count, 1U);
}

} // namespace
