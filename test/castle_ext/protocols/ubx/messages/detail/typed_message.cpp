#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/detail/typed_message.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

static CASTLE_CONSTEXPR uint8_t TEST_CLASS = 0x7AU;
static CASTLE_CONSTEXPR uint8_t TEST_ID = 0x7BU;

struct typed_one : detail::typed_message<typed_one, TEST_CLASS, TEST_ID, 0x01U, 4U>
{
    uint16_t value = 0U;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(0x01U);
        stream.reserved(1U);
        stream.field(self.value);
    }
};

struct typed_two : detail::typed_message<typed_two, TEST_CLASS, TEST_ID, 0x02U, 4U>
{
    int16_t value = 0;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(0x02U);
        stream.constant(0x00U);
        stream.field(self.value);
    }
};

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

TEST(UbxMessageDetailTyped, MatchesOnlyItsOwnLayout)
{
    EXPECT_EQ(typed_one::message_type, 0x01U);
    EXPECT_EQ(typed_two::message_type, 0x02U);

    uint8_t one[4U] = {0x01U, 0x00U, 0x34U, 0x12U};
    uint8_t two[4U] = {0x02U, 0x00U, 0xFEU, 0xFFU};

    message_view raw_one = make_view(TEST_CLASS, TEST_ID, one, 4U);
    message_view raw_two = make_view(TEST_CLASS, TEST_ID, two, 4U);
    EXPECT_TRUE(typed_one::matches(raw_one));
    EXPECT_FALSE(typed_two::matches(raw_one));
    EXPECT_TRUE(typed_two::matches(raw_two));
    EXPECT_FALSE(typed_one::matches(raw_two));

    typed_one out_one{};
    typed_two out_two{};
    ASSERT_TRUE(typed_one::decode(raw_one, out_one));
    EXPECT_EQ(out_one.value, 0x1234U);
    ASSERT_TRUE(typed_two::decode(raw_two, out_two));
    EXPECT_EQ(out_two.value, -2);

    // Cross-decoding fails on the type constant.
    EXPECT_FALSE(typed_two::decode(raw_one, out_two));
}

TEST(UbxMessageDetailTyped, RejectsWrongIdentityLengthAndEmptyPayload)
{
    uint8_t one[5U] = {0x01U, 0x00U, 0x34U, 0x12U, 0x00U};
    EXPECT_FALSE(typed_one::matches(make_view(TEST_CLASS, 0x00U, one, 4U)));
    EXPECT_FALSE(typed_one::matches(make_view(0x00U, TEST_ID, one, 4U)));
    EXPECT_FALSE(typed_one::matches(make_view(TEST_CLASS, TEST_ID, one, 3U)));
    EXPECT_FALSE(typed_one::matches(make_view(TEST_CLASS, TEST_ID, one, 5U)));
    EXPECT_FALSE(typed_one::matches(message_view{{TEST_CLASS, TEST_ID, 0U}, array_view<CASTLE_CONST uint8_t>(), checksum{}}));

    // Version constant mismatch on a layout that fixes it.
    uint8_t two[4U] = {0x02U, 0x01U, 0x00U, 0x00U};
    typed_two out{};
    EXPECT_TRUE(typed_two::matches(make_view(TEST_CLASS, TEST_ID, two, 4U)));
    EXPECT_FALSE(typed_two::decode(make_view(TEST_CLASS, TEST_ID, two, 4U), out));
}

TEST(UbxMessageDetailTyped, EncodeWritesDiscriminator)
{
    typed_two in{};
    in.value = -300;
    uint8_t frame[16U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD], 0x02U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 1U], 0x00U);

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(typed_two::matches(raw));
    EXPECT_FALSE(typed_one::matches(raw));
}

} // namespace
