#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/tim_tp.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageTimTpMirror, MatchesAndDecodesExactPayload)
{
    using message = tim_tp;
    uint8_t payload[message::payload_length] = {};
    message_view raw{message_header{message::msg_class, message::msg_id, static_cast<uint16_t>(sizeof(payload))}, array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)), checksum{}};
    message out;
    EXPECT_TRUE(message::matches(raw));
    ASSERT_TRUE(message::decode(raw, out));

    raw.header.msg_id = static_cast<uint8_t>(message::msg_id ^ 0xFFU);
    EXPECT_FALSE(message::matches(raw));
    raw.header.msg_id = message::msg_id;
    raw.header.payload_length = static_cast<uint16_t>(sizeof(payload) - 1U);
    EXPECT_FALSE(message::decode(raw, out));
}

} // namespace
