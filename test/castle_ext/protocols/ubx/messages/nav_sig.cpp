#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_sig.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageNavSigMirror, DecodesOneSignal)
{
    uint8_t payload[24U] = {};
    castle::write_le32(&payload[0U], 55U);
    payload[4U] = 1U;
    payload[5U] = 1U;
    payload[8U] = 6U;
    payload[9U] = 22U;
    payload[10U] = 7U;
    payload[11U] = 3U;
    castle::write_le16(&payload[12U], static_cast<uint16_t>(-15));
    payload[14U] = 40U;
    payload[15U] = 5U;
    payload[16U] = 2U;
    payload[17U] = 3U;
    castle::write_le16(&payload[18U], 0x1234U);

    message_view raw{{UBX_CLASS_NAV, UBX_ID_NAV_SIG, 24U},
                     array_view<CASTLE_CONST uint8_t>(payload, 24U), checksum{}};
    nav_sig<2U> out;
    ASSERT_TRUE(nav_sig<2U>::decode(raw, out));
    EXPECT_EQ(out.num_sigs, 1U);
    EXPECT_EQ(out.signals[0U].gnss_id, 6U);
    EXPECT_EQ(out.signals[0U].pr_res, -15);
    EXPECT_EQ(out.signals[0U].sig_flags, 0x1234U);
}

TEST(UbxMessageNavSigMirror, RejectsUnexpectedTotalLength)
{
    uint8_t payload[9U] = {};
    payload[5U] = 1U;
    message_view raw{{UBX_CLASS_NAV, UBX_ID_NAV_SIG, 9U},
                     array_view<CASTLE_CONST uint8_t>(payload, 9U), checksum{}};
    nav_sig<> out;
    EXPECT_FALSE(nav_sig<>::decode(raw, out));
}

TEST(UbxMessageNavSigMirror, ClampsSignalStorageAndSkipsTrailingBlock)
{
    uint8_t payload[40U] = {};
    payload[5U] = 2U;
    message_view raw{{UBX_CLASS_NAV, UBX_ID_NAV_SIG, 40U},
                     array_view<CASTLE_CONST uint8_t>(payload, 40U), checksum{}};
    nav_sig<1U> out;
    ASSERT_TRUE(nav_sig<1U>::decode(raw, out));
    EXPECT_EQ(out.num_sigs, 2U);
}

} // namespace
