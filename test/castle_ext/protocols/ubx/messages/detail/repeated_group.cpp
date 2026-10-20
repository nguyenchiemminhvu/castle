#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/detail/repeated_group.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

struct block
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 3U;

    uint8_t id = 0U;
    uint16_t value = 0U;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.id);
        stream.field(self.value);
    }
};

TEST(UbxMessageDetailGroup, LengthHelpers)
{
    EXPECT_TRUE(detail::has_group_length(14U, 8U, 2U, 3U));
    EXPECT_FALSE(detail::has_group_length(13U, 8U, 2U, 3U));
    EXPECT_TRUE(detail::has_group_length(8U, 8U, 0U, 3U));

    castle::size_type count = 99U;
    EXPECT_TRUE(detail::implied_group_count(14U, 2U, 3U, count));
    EXPECT_EQ(count, 4U);
    EXPECT_TRUE(detail::implied_group_count(2U, 2U, 3U, count));
    EXPECT_EQ(count, 0U);

    count = 99U;
    EXPECT_FALSE(detail::implied_group_count(13U, 2U, 3U, count));
    EXPECT_EQ(count, 0U);
    count = 99U;
    EXPECT_FALSE(detail::implied_group_count(1U, 2U, 3U, count));
    EXPECT_EQ(count, 0U);
}

TEST(UbxMessageDetailGroup, ReadGroupStoresAndSkipsOverflow)
{
    uint8_t payload[10U] = {};
    for (uint8_t index = 0U; index < 3U; ++index)
    {
        payload[index * 3U] = static_cast<uint8_t>(index + 1U);
        castle::write_le16(&payload[(index * 3U) + 1U], static_cast<uint16_t>(0x1000U * (index + 1U)));
    }
    payload[9U] = 0x5AU; // trailing field after the group

    payload_reader all(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    castle::container::vector<block, 4U> wide;
    detail::read_group(all, 3U, wide);
    ASSERT_TRUE(all.ok());
    ASSERT_EQ(wide.size(), 3U);
    EXPECT_EQ(wide[0U].id, 1U);
    EXPECT_EQ(wide[2U].value, 0x3000U);
    EXPECT_EQ(all.read_u8(), 0x5AU);

    payload_reader capped(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    castle::container::vector<block, 2U> narrow;
    static_cast<void>(narrow.push_back(block{}));
    static_cast<void>(narrow.push_back(block{}));
    detail::read_group(capped, 3U, narrow); // cleared first, then filled
    ASSERT_TRUE(capped.ok());
    ASSERT_EQ(narrow.size(), 2U);
    EXPECT_EQ(narrow[1U].id, 2U);
    EXPECT_EQ(capped.position(), 9U); // third block skipped, cursor on the trailer
    EXPECT_EQ(capped.read_u8(), 0x5AU);

    payload_reader none(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    detail::read_group(none, 0U, wide);
    EXPECT_TRUE(wide.empty());
    EXPECT_EQ(none.position(), 0U);
}

TEST(UbxMessageDetailGroup, ReadGroupLatchesShortPayload)
{
    uint8_t payload[4U] = {1U, 0x10U, 0x00U, 2U};
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    castle::container::vector<block, 4U> out;
    detail::read_group(reader, 2U, out);
    EXPECT_FALSE(reader.ok());

    payload_reader skipping(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    castle::container::vector<block, 1U> one;
    detail::read_group(skipping, 2U, one); // stored 1, skip of 3 overruns
    EXPECT_EQ(one.size(), 1U);
    EXPECT_FALSE(skipping.ok());
}

TEST(UbxMessageDetailGroup, WriteGroupRoundTrip)
{
    castle::container::vector<block, 3U> in;
    block first{};
    first.id = 9U;
    first.value = 0xA1B2U;
    block second{};
    second.id = 8U;
    second.value = 0x0102U;
    static_cast<void>(in.push_back(first));
    static_cast<void>(in.push_back(second));

    uint8_t buffer[6U] = {};
    payload_writer writer(array_view<uint8_t>(buffer, sizeof(buffer)));
    detail::write_group(writer, in);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.size(), 6U);
    EXPECT_EQ(buffer[0U], 9U);
    EXPECT_EQ(castle::read_le16(&buffer[1U]), 0xA1B2U);
    EXPECT_EQ(buffer[3U], 8U);

    payload_reader reader(array_view<CASTLE_CONST uint8_t>(buffer, sizeof(buffer)));
    castle::container::vector<block, 3U> out;
    detail::read_group(reader, 2U, out);
    ASSERT_EQ(out.size(), 2U);
    EXPECT_EQ(out[1U].value, 0x0102U);

    uint8_t tight[5U] = {};
    payload_writer overflow(array_view<uint8_t>(tight, sizeof(tight)));
    detail::write_group(overflow, in);
    EXPECT_EQ(overflow.status(), castle::status::full);
}

TEST(UbxMessageDetailGroup, TailHelpers)
{
    uint8_t payload[6U] = {0xEEU, 'd', 'a', 't', 'a', '!'};
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    reader.skip(1U);

    castle::container::vector<uint8_t, 3U> tail;
    detail::read_tail(reader, tail);
    ASSERT_TRUE(reader.ok());
    ASSERT_EQ(tail.size(), 3U); // capacity-limited
    EXPECT_EQ(tail[0U], static_cast<uint8_t>('d'));
    EXPECT_EQ(tail[2U], static_cast<uint8_t>('t'));
    EXPECT_EQ(reader.remaining(), 0U); // rest skipped

    payload_reader full(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    castle::container::vector<char, 8U> text;
    detail::read_tail(full, text);
    EXPECT_EQ(text.size(), 6U);
    EXPECT_EQ(text[5U], '!');

    uint8_t buffer[3U] = {};
    payload_writer writer(array_view<uint8_t>(buffer, sizeof(buffer)));
    detail::write_tail(writer, tail);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(buffer[1U], static_cast<uint8_t>('a'));

    payload_writer overflow(array_view<uint8_t>(buffer, 2U));
    detail::write_tail(overflow, tail);
    EXPECT_EQ(overflow.status(), castle::status::full);
}

} // namespace
