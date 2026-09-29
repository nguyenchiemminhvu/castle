#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/esf_status.hpp"

namespace
{
using namespace castle_ext::protocols::ubx;
using namespace castle_ext::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageEsfStatusMirror, DecodesOneSensorAndDerivedCapacityPath)
{
    uint8_t payload[20U] = {};
    write_le32(&payload[0U], 100U);
    payload[4U] = 1U;
    payload[12U] = 2U; // fusion mode after reserved bytes
    payload[15U] = 1U; // num_sens
    payload[16U] = 0x61U;
    payload[17U] = 0x0EU;
    payload[18U] = 10U;
    payload[19U] = 11U;
    message_view raw{{UBX_CLASS_ESF, UBX_ID_ESF_STATUS, 20U}, array_view<CASTLE_CONST uint8_t>(payload, 20U), checksum{}};
    esf_status<> out;
    ASSERT_TRUE(esf_status<>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 100U);
    EXPECT_EQ(out.num_sens, 1U);
    EXPECT_EQ(out.sensors[0U].sens_status1, 0x61U);
    EXPECT_EQ(out.sensors[0U].sens_status1 & esf_status_sensor::USED, esf_status_sensor::USED);
    EXPECT_EQ(out.sensors[0U].sens_status1 & esf_status_sensor::READY, esf_status_sensor::READY);
    EXPECT_EQ(out.sensors[0U].sens_status1 & esf_status_sensor::TYPE_MASK, 1U);
}

TEST(UbxMessageEsfStatusMirror, RejectsPayloadShorterThanDeclaredSensorCount)
{
    uint8_t payload[16U] = {};
    payload[15U] = 1U;
    message_view raw{{UBX_CLASS_ESF, UBX_ID_ESF_STATUS, 16U}, array_view<CASTLE_CONST uint8_t>(payload, 16U), checksum{}};
    esf_status<> out;
    EXPECT_FALSE(esf_status<>::decode(raw, out));
}

} // namespace

namespace
{
using namespace castle_ext::protocols::ubx;
using namespace castle_ext::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageEsfStatusMirror, ClampsSensorStorageAndSkipsTheRest)
{
    uint8_t payload[24U] = {};
    payload[15U] = 2U;
    payload[16U] = 1U;
    payload[20U] = 2U;
    message_view raw{{UBX_CLASS_ESF, UBX_ID_ESF_STATUS, 24U},
                     array_view<CASTLE_CONST uint8_t>(payload, 24U), checksum{}};
    esf_status<1U> out;
    ASSERT_TRUE(esf_status<1U>::decode(raw, out));
    EXPECT_EQ(out.num_sens, 2U);
    EXPECT_EQ(out.sensors[0U].sens_status1, 1U);
}

} // namespace
