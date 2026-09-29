#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/rxm_measx.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageRxmMeasxMirror, DecodesHeaderAndOneSatellite)
{
    uint8_t payload[68U] = {};
    payload[0U] = 1U;
    castle::write_le32(&payload[4U], 10U);
    castle::write_le32(&payload[8U], 20U);
    castle::write_le32(&payload[12U], 30U);
    castle::write_le32(&payload[20U], 40U);
    castle::write_le16(&payload[24U], 1U);
    castle::write_le16(&payload[26U], 2U);
    castle::write_le16(&payload[28U], 3U);
    castle::write_le16(&payload[32U], 4U);
    payload[34U] = 1U;
    payload[35U] = 2U;

    payload[44U] = 6U;
    payload[45U] = 22U;
    payload[46U] = 45U;
    payload[47U] = 3U;
    castle::write_le32(&payload[48U], static_cast<uint32_t>(-5));
    castle::write_le32(&payload[52U], static_cast<uint32_t>(7));
    castle::write_le16(&payload[56U], 123U);
    castle::write_le16(&payload[58U], 456U);
    castle::write_le32(&payload[60U], 789U);
    payload[64U] = 10U;
    payload[65U] = 11U;

    message_view raw{{UBX_CLASS_RXM, UBX_ID_RXM_MEASX, 68U},
                     array_view<CASTLE_CONST uint8_t>(payload, 68U), checksum{}};
    rxm_measx<2U> out;
    ASSERT_TRUE(rxm_measx<2U>::decode(raw, out));
    EXPECT_EQ(out.num_svs, 1U);
    EXPECT_EQ(out.gps_tow, 10U);
    EXPECT_EQ(out.svs[0U].gnss_id, 6U);
    EXPECT_EQ(out.svs[0U].doppler_ms, -5);
    EXPECT_EQ(out.svs[0U].code_phase, 789U);
}

TEST(UbxMessageRxmMeasxMirror, RejectsUnexpectedTotalLength)
{
    uint8_t payload[45U] = {};
    message_view raw{{UBX_CLASS_RXM, UBX_ID_RXM_MEASX, 45U},
                     array_view<CASTLE_CONST uint8_t>(payload, 45U), checksum{}};
    rxm_measx<> out;
    EXPECT_FALSE(rxm_measx<>::decode(raw, out));
}

TEST(UbxMessageRxmMeasxMirror, ClampsSatelliteStorageAndSkipsTrailingBlock)
{
    uint8_t payload[92U] = {};
    payload[34U] = 2U;
    message_view raw{{UBX_CLASS_RXM, UBX_ID_RXM_MEASX, 92U},
                     array_view<CASTLE_CONST uint8_t>(payload, 92U), checksum{}};
    rxm_measx<1U> out;
    ASSERT_TRUE(rxm_measx<1U>::decode(raw, out));
    EXPECT_EQ(out.num_svs, 2U);
}

} // namespace
