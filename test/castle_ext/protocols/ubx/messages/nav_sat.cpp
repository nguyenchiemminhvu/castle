#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_sat.hpp"

namespace
{
using namespace castle_ext::protocols::ubx;
using namespace castle_ext::protocols::ubx::messages;
using castle::container::array_view;
TEST(UbxMessageNavSatMirror, DecodesOneSatellite)
{
    uint8_t payload[20U] = {};
    write_le32(&payload[0U], 123U);
    payload[4U] = 1U;
    payload[5U] = 1U;
    payload[8U] = 6U;
    payload[9U] = 22U;
    payload[10U] = 35U;
    payload[11U] = static_cast<uint8_t>(-12);
    write_le16(&payload[12U], 1234U);
    write_le16(&payload[14U], static_cast<uint16_t>(-45));
    write_le32(&payload[16U], 0xA5A5A5A5U);

    message_view raw{{UBX_CLASS_NAV, UBX_ID_NAV_SAT, 20U},
                     array_view<CASTLE_CONST uint8_t>(payload, 20U), checksum{}};
    nav_sat<2U> out;
    ASSERT_TRUE(nav_sat<2U>::decode(raw, out));
    EXPECT_EQ(out.num_svs, 1U);
    EXPECT_EQ(out.svs[0U].gnss_id, 6U);
    EXPECT_EQ(out.svs[0U].sv_id, 22U);
    EXPECT_EQ(out.svs[0U].cno, 35U);
    EXPECT_EQ(out.svs[0U].elev, -12);
    EXPECT_EQ(out.svs[0U].azim, 1234);
    EXPECT_EQ(out.svs[0U].pr_res, -45);
    EXPECT_EQ(out.svs[0U].flags, 0xA5A5A5A5U);
}

TEST(UbxMessageNavSatMirror, RejectsUnexpectedTotalLength)
{
 uint8_t payload[9U] = {}; payload[5U]=1U; message_view raw{{UBX_CLASS_NAV,UBX_ID_NAV_SAT,9U},array_view<CASTLE_CONST uint8_t>(payload,9U),checksum{}}; nav_sat<> out; EXPECT_FALSE(nav_sat<>::decode(raw,out));
}
}

namespace
{
using namespace castle_ext::protocols::ubx;
using namespace castle_ext::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageNavSatMirror, ClampsSatelliteStorageAndSkipsTrailingBlock)
{
    uint8_t payload[32U] = {};
    payload[5U] = 2U;
    message_view raw{{UBX_CLASS_NAV, UBX_ID_NAV_SAT, 32U},
                     array_view<CASTLE_CONST uint8_t>(payload, 32U), checksum{}};
    nav_sat<1U> out;
    ASSERT_TRUE(nav_sat<1U>::decode(raw, out));
    EXPECT_EQ(out.num_svs, 2U);
}

} // namespace
