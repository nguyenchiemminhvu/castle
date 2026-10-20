#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/detail/nav_gnss_time.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

static CASTLE_CONSTEXPR uint8_t TEST_ID = 0x7BU;

/** A bare derived message, exactly like nav_timebds/gal/qzss but without spec-named accessors. */
struct gnss_time_probe : detail::nav_gnss_time<gnss_time_probe, TEST_ID>
{
};

using message = gnss_time_probe;

message_view view_of(uint8_t msg_id, const uint8_t* payload, castle::size_type length)
{
    return message_view{{UBX_CLASS_NAV, msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

TEST(UbxMessageDetailNavGnssTime, BindsNavClassAndGivenId)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, TEST_ID);
    EXPECT_EQ(message::payload_length, 20U);
    EXPECT_EQ(message::VALID_TOW, 0x01U);
    EXPECT_EQ(message::VALID_WEEK, 0x02U);
    EXPECT_EQ(message::VALID_LEAP_S, 0x04U);
}

TEST(UbxMessageDetailNavGnssTime, LayoutCoversTwentyBytesAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 0x01020304U);
    castle::write_le32(&payload[4U], 0x05060708U);
    castle::write_le32s(&payload[8U], -0x090A0B0C);
    castle::write_le16s(&payload[12U], -0x0D0E);
    castle::write_le8s(&payload[14U], -0x0F);
    payload[15U] = 0x06U;
    castle::write_le32(&payload[16U], 0x11121314U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(TEST_ID, payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 0x01020304U);
    EXPECT_EQ(decoded.tow, 0x05060708U);
    EXPECT_EQ(decoded.f_tow, -0x090A0B0C);
    EXPECT_EQ(decoded.week, -0x0D0E);
    EXPECT_EQ(decoded.leap_s, -0x0F);
    EXPECT_EQ(decoded.valid, 0x06U);
    EXPECT_EQ(decoded.t_acc, 0x11121314U);

    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(decoded, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
    for (castle::size_type i = 0U; i < message::payload_length; ++i)
    {
        EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + i], payload[i]) << "byte " << i;
    }
}

TEST(UbxMessageDetailNavGnssTime, RejectsOtherIdAndWrongLength)
{
    uint8_t payload[message::payload_length + 1U] = {};
    payload[0U] = 1U;
    message decoded{};
    EXPECT_FALSE(message::matches(view_of(static_cast<uint8_t>(TEST_ID + 1U), payload, message::payload_length)));
    EXPECT_FALSE(message::decode(view_of(static_cast<uint8_t>(TEST_ID + 1U), payload, message::payload_length), decoded));
    EXPECT_FALSE(message::decode(view_of(TEST_ID, payload, message::payload_length - 1U), decoded));
    EXPECT_FALSE(message::decode(view_of(TEST_ID, payload, message::payload_length + 1U), decoded));
    EXPECT_EQ(decoded.i_tow, 0U);
}

TEST(UbxMessageDetailNavGnssTime, ValidityAccessors)
{
    message value{};
    value.valid = static_cast<uint8_t>(message::VALID_TOW | message::VALID_LEAP_S);
    EXPECT_TRUE(value.tow_valid());
    EXPECT_FALSE(value.week_valid());
    EXPECT_TRUE(value.leap_s_valid());

    value.valid = message::VALID_WEEK;
    EXPECT_FALSE(value.tow_valid());
    EXPECT_TRUE(value.week_valid());
    EXPECT_FALSE(value.leap_s_valid());
}

TEST(UbxMessageDetailNavGnssTime, TimeOfWeekCombinesWholeAndFraction)
{
    message value{};
    value.tow = 604799U;
    value.f_tow = 500000000;
    EXPECT_NEAR(value.time_of_week_s(), 604799.5, 1e-9);

    value.f_tow = -500000000;
    EXPECT_NEAR(value.time_of_week_s(), 604798.5, 1e-9);
}

} // namespace
