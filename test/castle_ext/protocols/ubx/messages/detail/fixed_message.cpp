#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/detail/fixed_message.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

static CASTLE_CONSTEXPR uint8_t TEST_CLASS = 0x7AU;
static CASTLE_CONSTEXPR uint8_t TEST_ID = 0x7BU;

struct fixed_probe : detail::fixed_message<fixed_probe, TEST_CLASS, TEST_ID, 8U>
{
    uint16_t a = 0U;
    int32_t b = 0;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(0x42U);
        stream.field(self.a);
        stream.field(self.b);
        stream.reserved(1U);
    }
};

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

TEST(UbxMessageDetailFixed, ExposesIdentityAndLength)
{
    EXPECT_EQ(fixed_probe::msg_class, TEST_CLASS);
    EXPECT_EQ(fixed_probe::msg_id, TEST_ID);
    EXPECT_EQ(fixed_probe::payload_length, 8U);
}

TEST(UbxMessageDetailFixed, DecodesSpecOffsets)
{
    uint8_t payload[8U] = {};
    payload[0U] = 0x42U;
    castle::write_le16(&payload[1U], 0xBEEFU);
    castle::write_le32(&payload[3U], static_cast<uint32_t>(-77));
    payload[7U] = 0xFFU; // reserved content is ignored

    fixed_probe out{};
    ASSERT_TRUE(fixed_probe::decode(make_view(TEST_CLASS, TEST_ID, payload, 8U), out));
    EXPECT_EQ(out.a, 0xBEEFU);
    EXPECT_EQ(out.b, -77);
}

TEST(UbxMessageDetailFixed, RejectsWrongIdentityLengthAndConstant)
{
    uint8_t payload[9U] = {0x42U, 0x01U, 0x00U, 0x05U, 0U, 0U, 0U, 0U, 0U};
    fixed_probe out{};
    out.a = 99U;

    EXPECT_FALSE(fixed_probe::matches(make_view(TEST_CLASS, 0x00U, payload, 8U)));
    EXPECT_FALSE(fixed_probe::decode(make_view(TEST_CLASS, 0x00U, payload, 8U), out));
    EXPECT_EQ(out.a, 0U);

    out.a = 99U;
    EXPECT_FALSE(fixed_probe::decode(make_view(0x00U, TEST_ID, payload, 8U), out));
    EXPECT_EQ(out.a, 0U);

    EXPECT_FALSE(fixed_probe::decode(make_view(TEST_CLASS, TEST_ID, payload, 7U), out));
    EXPECT_FALSE(fixed_probe::decode(make_view(TEST_CLASS, TEST_ID, payload, 9U), out));

    payload[0U] = 0x43U;
    out.a = 99U;
    EXPECT_TRUE(fixed_probe::matches(make_view(TEST_CLASS, TEST_ID, payload, 8U)));
    EXPECT_FALSE(fixed_probe::decode(make_view(TEST_CLASS, TEST_ID, payload, 8U), out));
    EXPECT_EQ(out.a, 0U); // reset after a mid-layout failure
    EXPECT_EQ(out.b, 0);
}

TEST(UbxMessageDetailFixed, EncodeRoundTripsThroughFrame)
{
    fixed_probe in{};
    in.a = 0x1357U;
    in.b = -123456;

    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(8U));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD], 0x42U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 7U], 0U);

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    fixed_probe out{};
    ASSERT_TRUE(fixed_probe::decode(raw, out));
    EXPECT_EQ(out.a, in.a);
    EXPECT_EQ(out.b, in.b);

    uint8_t small[frame_size(8U) - 1U] = {};
    EXPECT_EQ(encode_message(in, small, sizeof(small), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
