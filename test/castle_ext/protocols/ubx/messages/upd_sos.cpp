#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/upd_sos.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageUpdSosMirror, AcceptsAckAndRestoreCommands)
{
    uint8_t payload[8U] = {};
    payload[0U] = UPD_SOS_CMD_ACK;
    payload[4U] = UPD_SOS_RESP_ACK;

    message_view raw{{UBX_CLASS_UPD, UBX_ID_UPD_SOS, 8U},
                     array_view<CASTLE_CONST uint8_t>(payload, 8U), checksum{}};
    upd_sos_output out;
    ASSERT_TRUE(upd_sos_output::decode(raw, out));
    EXPECT_EQ(out.cmd, UPD_SOS_CMD_ACK);
    EXPECT_EQ(out.response, UPD_SOS_RESP_ACK);

    payload[0U] = UPD_SOS_CMD_RESTORE;
    ASSERT_TRUE(upd_sos_output::decode(raw, out));
    EXPECT_EQ(out.cmd, UPD_SOS_CMD_RESTORE);
}

TEST(UbxMessageUpdSosMirror, RejectsInvalidCommandAndShortPayload)
{
    uint8_t payload[8U] = {};
    message_view raw{{UBX_CLASS_UPD, UBX_ID_UPD_SOS, 8U},
                     array_view<CASTLE_CONST uint8_t>(payload, 8U), checksum{}};
    upd_sos_output out;
    EXPECT_FALSE(upd_sos_output::decode(raw, out));

    raw.header.payload_length = 7U;
    EXPECT_FALSE(upd_sos_output::decode(raw, out));
    EXPECT_TRUE(upd_sos_output::matches(raw));
}

} // namespace
