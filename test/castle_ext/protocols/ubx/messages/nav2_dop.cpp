#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav2_dop.hpp"

namespace
{
using namespace castle_ext::protocols::ubx;
using namespace castle_ext::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageNav2DopMirror, InheritedPayloadLayoutDecodes)
{
    using message = nav2_dop;
    uint8_t payload[message::payload_length] = {};
    message_view raw{message_header{message::msg_class, message::msg_id, static_cast<uint16_t>(sizeof(payload))}, array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)), checksum{}};
    message out;
    EXPECT_TRUE(message::matches(raw));
    ASSERT_TRUE(message::decode(raw, out));

    raw.header.payload_length = static_cast<uint16_t>(sizeof(payload) - 1U);
    EXPECT_FALSE(message::decode(raw, out));
}

} // namespace
