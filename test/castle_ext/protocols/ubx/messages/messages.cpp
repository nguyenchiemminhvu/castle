#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/messages.hpp"

namespace
{

using namespace castle::protocols::ubx::messages;

TEST(UbxMessagesMirror, AggregateHeaderExposesBuiltInMessageTypes)
{
    EXPECT_GT(sizeof(ack_ack), 0U);
    EXPECT_GT(sizeof(ack_nak), 0U);
    EXPECT_GT(sizeof(cfg_valget<>), 0U);
    EXPECT_GT(sizeof(esf_ins), 0U);
    EXPECT_GT(sizeof(esf_meas<>), 0U);
    EXPECT_GT(sizeof(esf_status<>), 0U);
    EXPECT_GT(sizeof(inf<>), 0U);
    EXPECT_GT(sizeof(mon_io<>), 0U);
    EXPECT_GT(sizeof(mon_span<>), 0U);
    EXPECT_GT(sizeof(mon_txbuf), 0U);
    EXPECT_GT(sizeof(mon_ver<>), 0U);
    EXPECT_GT(sizeof(nav_att), 0U);
    EXPECT_GT(sizeof(nav_clock), 0U);
    EXPECT_GT(sizeof(nav_dop), 0U);
    EXPECT_GT(sizeof(nav_eell), 0U);
    EXPECT_GT(sizeof(nav_odo), 0U);
    EXPECT_GT(sizeof(nav_pvt), 0U);
    EXPECT_GT(sizeof(nav_sat<>), 0U);
    EXPECT_GT(sizeof(nav_sig<>), 0U);
    EXPECT_GT(sizeof(nav_status), 0U);
    EXPECT_GT(sizeof(nav_timegps), 0U);
    EXPECT_GT(sizeof(nav_timeutc), 0U);
    EXPECT_GT(sizeof(rxm_measx<>), 0U);
    EXPECT_GT(sizeof(sec_crc), 0U);
    EXPECT_GT(sizeof(sec_sig), 0U);
    EXPECT_GT(sizeof(tim_tp), 0U);
    EXPECT_GT(sizeof(upd_sos_output), 0U);
}

} // namespace
