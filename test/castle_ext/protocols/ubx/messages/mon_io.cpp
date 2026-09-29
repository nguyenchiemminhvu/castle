#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_io.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageMonIoMirror, ValidMinimumLayoutDecodes)
{
    using message = mon_io<>;
    uint8_t payload[message::block_length] = {};
    message_view raw{message_header{message::msg_class, message::msg_id, static_cast<uint16_t>(sizeof(payload))}, array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)), checksum{}};
    message out;
    ASSERT_TRUE(message::matches(raw));
    ASSERT_TRUE(message::decode(raw, out));

    raw.header.msg_id = static_cast<uint8_t>(message::msg_id ^ 0x80U);
    EXPECT_FALSE(message::matches(raw));
    raw.header.msg_id = message::msg_id;
    raw.header.payload_length = static_cast<uint16_t>(sizeof(payload) + 1U);
    EXPECT_FALSE(message::decode(raw, out));
}

} // namespace

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageMonIoMirror, ClampsPortStorageAndSkipsTrailingPort)
{
    uint8_t payload[40U] = {};
    payload[0U] = 1U;
    payload[20U] = 2U;
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_IO, 40U},
                     array_view<CASTLE_CONST uint8_t>(payload, 40U), checksum{}};
    mon_io<1U> out;
    ASSERT_TRUE(mon_io<1U>::decode(raw, out));
    EXPECT_EQ(out.num_ports, 1U);
    EXPECT_EQ(out.ports[0U].rx_bytes, 1U);
}

} // namespace
