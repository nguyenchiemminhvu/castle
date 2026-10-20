#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/inf.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;
using castle::container::string_view;

message_view view_of(uint8_t msg_id, CASTLE_CONST uint8_t* payload, castle::size_type length)
{
    return message_view{{UBX_CLASS_INF, msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

TEST(UbxMessageInf, ConstantsMatchSpec)
{
    EXPECT_EQ(inf_error<>::msg_class, UBX_CLASS_INF);
    EXPECT_EQ(inf_error<>::msg_id, 0x00U);
    EXPECT_EQ(inf_warning<>::msg_id, 0x01U);
    EXPECT_EQ(inf_notice<>::msg_id, 0x02U);
    EXPECT_EQ(inf_test<>::msg_id, 0x03U);
    EXPECT_EQ(inf_debug<>::msg_id, 0x04U);
    EXPECT_EQ(inf_error<>::subtype, inf_subtype::error);
    EXPECT_EQ(inf_warning<>::subtype, inf_subtype::warning);
    EXPECT_EQ(inf_notice<>::subtype, inf_subtype::notice);
    EXPECT_EQ(inf_test<>::subtype, inf_subtype::test);
    EXPECT_EQ(inf_debug<>::subtype, inf_subtype::debug);
    EXPECT_EQ(inf<>::msg_class, UBX_CLASS_INF);
    EXPECT_EQ(inf_error<16U>().text.capacity(), 16U);
}

TEST(UbxMessageInf, PerSeverityRoundTripWithoutTerminator)
{
    inf_warning<> original{};
    ASSERT_EQ(original.text.assign("low battery"), castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + 11U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_INF);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_INF_WARNING);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD], 'l');
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 10U], 'y');

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    inf_warning<> decoded{};
    ASSERT_TRUE(inf_warning<>::decode(raw, decoded));
    EXPECT_EQ(decoded.text, string_view("low battery"));
}

TEST(UbxMessageInf, EachSeverityMatchesOnlyItsOwnId)
{
    uint8_t payload[1U] = {'x'};
    for (uint8_t id = 0U; id <= 6U; ++id)
    {
        CASTLE_CONST message_view raw = view_of(id, payload, 1U);
        EXPECT_EQ(inf_error<>::matches(raw), id == UBX_ID_INF_ERROR);
        EXPECT_EQ(inf_warning<>::matches(raw), id == UBX_ID_INF_WARNING);
        EXPECT_EQ(inf_notice<>::matches(raw), id == UBX_ID_INF_NOTICE);
        EXPECT_EQ(inf_test<>::matches(raw), id == UBX_ID_INF_TEST);
        EXPECT_EQ(inf_debug<>::matches(raw), id == UBX_ID_INF_DEBUG);
        EXPECT_EQ(inf<>::matches(raw), id <= UBX_ID_INF_DEBUG);
    }
}

TEST(UbxMessageInf, PerSeverityRejectsOtherIdAndClass)
{
    uint8_t payload[2U] = {'o', 'k'};
    inf_debug<> decoded{};
    ASSERT_EQ(decoded.text.assign("stale"), castle::status::ok);
    EXPECT_FALSE(inf_debug<>::decode(view_of(UBX_ID_INF_TEST, payload, 2U), decoded));
    EXPECT_TRUE(decoded.text.empty());

    message_view other = view_of(UBX_ID_INF_DEBUG, payload, 2U);
    other.header.msg_class = UBX_CLASS_ACK;
    EXPECT_FALSE(inf_debug<>::matches(other));
    EXPECT_FALSE(inf_debug<>::decode(other, decoded));
}

TEST(UbxMessageInf, AnySeverityRecordsIdAndSubtype)
{
    uint8_t payload[3U] = {'a', 'b', 'c'};
    CASTLE_CONST inf_subtype expected[5U] = {
        inf_subtype::error, inf_subtype::warning, inf_subtype::notice, inf_subtype::test, inf_subtype::debug};
    for (uint8_t id = 0U; id <= UBX_ID_INF_DEBUG; ++id)
    {
        inf<> decoded{};
        ASSERT_TRUE(inf<>::decode(view_of(id, payload, 3U), decoded));
        EXPECT_EQ(decoded.message_id, id);
        EXPECT_EQ(decoded.subtype, expected[id]);
        EXPECT_EQ(decoded.text, string_view("abc"));
    }
}

TEST(UbxMessageInf, AnySeverityRejectsOtherIdAndClassAndResets)
{
    uint8_t payload[1U] = {'A'};
    inf<> decoded{};
    ASSERT_TRUE(inf<>::decode(view_of(UBX_ID_INF_DEBUG, payload, 1U), decoded));

    EXPECT_FALSE(inf<>::decode(view_of(0x05U, payload, 1U), decoded));
    EXPECT_EQ(decoded.message_id, UBX_ID_INF_ERROR);
    EXPECT_EQ(decoded.subtype, inf_subtype::error);
    EXPECT_TRUE(decoded.text.empty());

    message_view other = view_of(UBX_ID_INF_ERROR, payload, 1U);
    other.header.msg_class = UBX_CLASS_NAV;
    EXPECT_FALSE(inf<>::matches(other));
    EXPECT_FALSE(inf<>::decode(other, decoded));
}

TEST(UbxMessageInf, TextStopsAtNul)
{
    uint8_t payload[8U] = {'E', 'R', 'R', '!', '\0', 'X', 'X', 'X'};
    inf<8U> any{};
    ASSERT_TRUE(inf<8U>::decode(view_of(UBX_ID_INF_WARNING, payload, 8U), any));
    EXPECT_EQ(any.text, string_view("ERR!"));
    inf_warning<8U> one{};
    ASSERT_TRUE(inf_warning<8U>::decode(view_of(UBX_ID_INF_WARNING, payload, 8U), one));
    EXPECT_EQ(one.text.size(), 4U);

    uint8_t leading[3U] = {'\0', 'Y', 'Z'};
    ASSERT_TRUE(inf<8U>::decode(view_of(UBX_ID_INF_ERROR, leading, 3U), any));
    EXPECT_TRUE(any.text.empty());
}

TEST(UbxMessageInf, TextIsClampedToCapacity)
{
    uint8_t payload[6U] = {'A', 'B', 'C', 'D', 'E', 'F'};
    inf<4U> any{};
    ASSERT_TRUE(inf<4U>::decode(view_of(UBX_ID_INF_DEBUG, payload, 6U), any));
    EXPECT_EQ(any.text, string_view("ABCD"));
    EXPECT_TRUE(any.text.full());

    inf_debug<6U> exact{};
    ASSERT_TRUE(inf_debug<6U>::decode(view_of(UBX_ID_INF_DEBUG, payload, 6U), exact));
    EXPECT_EQ(exact.text, string_view("ABCDEF"));
}

TEST(UbxMessageInf, EmptyFrameDecodesToEmptyText)
{
    uint8_t frame[UBX_FRAME_OVERHEAD];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(UBX_CLASS_INF, UBX_ID_INF_NOTICE, array_view<CASTLE_CONST uint8_t>(),
                           frame, sizeof(frame), written),
              castle::status::ok);
    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_EQ(raw.payload_length(), 0U);
    EXPECT_EQ(raw.payload.data(), nullptr);

    inf<> any{};
    ASSERT_TRUE(inf<>::decode(raw, any));
    EXPECT_EQ(any.subtype, inf_subtype::notice);
    EXPECT_TRUE(any.text.empty());

    inf_notice<> one{};
    ASSERT_TRUE(inf_notice<>::decode(raw, one));
    EXPECT_TRUE(one.text.empty());

    // Encoding an empty string yields the same empty frame.
    uint8_t encoded[UBX_FRAME_OVERHEAD];
    ASSERT_EQ(encode_message(one, encoded, sizeof(encoded), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD);
    for (castle::size_type index = 0U; index < UBX_FRAME_OVERHEAD; ++index)
    {
        EXPECT_EQ(encoded[index], frame[index]);
    }
}

TEST(UbxMessageInf, EncodeIntoTooSmallBufferReportsFull)
{
    inf_test<> value{};
    ASSERT_EQ(value.text.assign("abc"), castle::status::ok);
    uint8_t frame[UBX_FRAME_OVERHEAD + 3U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
