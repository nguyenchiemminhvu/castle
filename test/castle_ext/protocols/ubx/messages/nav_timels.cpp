#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_timels.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_timels;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

message distinctive()
{
    message value{};
    value.i_tow = 0x11223344U;
    value.version = 0x5AU;
    value.src_of_curr_ls = nav_timels_src_of_curr_ls::galileo;
    value.curr_ls = 18;
    value.src_of_ls_change = nav_timels_src_of_ls_change::beidou;
    value.ls_change = -1;
    value.time_to_ls_event = -0x01234567;
    value.date_of_ls_gps_wn = 0x0912U;
    value.date_of_ls_gps_dn = 0x0007U;
    value.valid = 0x03U;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.version, 0U);
    EXPECT_EQ(value.src_of_curr_ls, nav_timels_src_of_curr_ls::firmware_default);
    EXPECT_EQ(value.curr_ls, 0);
    EXPECT_EQ(value.src_of_ls_change, nav_timels_src_of_ls_change::no_source);
    EXPECT_EQ(value.ls_change, 0);
    EXPECT_EQ(value.time_to_ls_event, 0);
    EXPECT_EQ(value.date_of_ls_gps_wn, 0U);
    EXPECT_EQ(value.date_of_ls_gps_dn, 0U);
    EXPECT_EQ(value.valid, 0U);
}

TEST(UbxMessageNavTimels, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_TIMELS);
    EXPECT_EQ(message::payload_length, 24U);
    EXPECT_EQ(message::VALID_CURR_LS, 0x01U);
    EXPECT_EQ(message::VALID_TIME_TO_LS_EVENT, 0x02U);
}

TEST(UbxMessageNavTimels, EnumValuesMatchSpec)
{
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_curr_ls::firmware_default), 0U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_curr_ls::gps_glonass), 1U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_curr_ls::gps), 2U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_curr_ls::sbas), 3U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_curr_ls::beidou), 4U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_curr_ls::galileo), 5U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_curr_ls::aided_data), 6U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_curr_ls::configured), 7U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_curr_ls::unknown), 255U);

    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_ls_change::no_source), 0U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_ls_change::gps), 2U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_ls_change::sbas), 3U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_ls_change::beidou), 4U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_ls_change::galileo), 5U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timels_src_of_ls_change::glonass), 6U);
}

TEST(UbxMessageNavTimels, RoundTripPreservesEveryField)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(message::matches(raw));
    EXPECT_EQ(raw.payload_length(), message::payload_length);

    message decoded{};
    ASSERT_TRUE(message::decode(raw, decoded));
    EXPECT_EQ(decoded.i_tow, original.i_tow);
    EXPECT_EQ(decoded.version, original.version);
    EXPECT_EQ(decoded.src_of_curr_ls, original.src_of_curr_ls);
    EXPECT_EQ(decoded.curr_ls, original.curr_ls);
    EXPECT_EQ(decoded.src_of_ls_change, original.src_of_ls_change);
    EXPECT_EQ(decoded.ls_change, original.ls_change);
    EXPECT_EQ(decoded.time_to_ls_event, original.time_to_ls_event);
    EXPECT_EQ(decoded.date_of_ls_gps_wn, original.date_of_ls_gps_wn);
    EXPECT_EQ(decoded.date_of_ls_gps_dn, original.date_of_ls_gps_dn);
    EXPECT_EQ(decoded.valid, original.valid);
}

TEST(UbxMessageNavTimels, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 504000123U);
    payload[4U] = 0x00U;
    payload[5U] = 0xEEU; // reserved0 is ignored
    payload[6U] = 0xEEU;
    payload[7U] = 0xEEU;
    payload[8U] = 0x02U;
    castle::write_le8s(&payload[9U], 18);
    payload[10U] = 0x06U;
    castle::write_le8s(&payload[11U], 1);
    castle::write_le32s(&payload[12U], 1209600);
    castle::write_le16(&payload[16U], 2400U);
    castle::write_le16(&payload[18U], 1U);
    payload[20U] = 0xEEU; // reserved1 is ignored
    payload[21U] = 0xEEU;
    payload[22U] = 0xEEU;
    payload[23U] = 0x02U;

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 504000123U);
    EXPECT_EQ(decoded.version, 0U);
    EXPECT_EQ(decoded.src_of_curr_ls, nav_timels_src_of_curr_ls::gps);
    EXPECT_EQ(decoded.curr_ls, 18);
    EXPECT_EQ(decoded.src_of_ls_change, nav_timels_src_of_ls_change::glonass);
    EXPECT_EQ(decoded.ls_change, 1);
    EXPECT_EQ(decoded.time_to_ls_event, 1209600);
    EXPECT_EQ(decoded.date_of_ls_gps_wn, 2400U);
    EXPECT_EQ(decoded.date_of_ls_gps_dn, 1U);
    EXPECT_EQ(decoded.valid, 0x02U);
    EXPECT_FALSE(decoded.valid_curr_ls());
    EXPECT_TRUE(decoded.valid_time_to_ls_event());
}

TEST(UbxMessageNavTimels, DecodesUnlistedSourceValuesVerbatim)
{
    uint8_t payload[message::payload_length] = {};
    payload[8U] = 0xFFU;
    payload[10U] = 0x01U; // not listed for srcOfLsChange

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.src_of_curr_ls, nav_timels_src_of_curr_ls::unknown);
    EXPECT_EQ(static_cast<uint8_t>(decoded.src_of_ls_change), 0x01U);
}

TEST(UbxMessageNavTimels, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_NAV);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_NAV_TIMELS);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(castle::read_le32(&payload[0U]), original.i_tow);
    EXPECT_EQ(payload[4U], original.version);
    EXPECT_EQ(payload[5U], 0U);
    EXPECT_EQ(payload[6U], 0U);
    EXPECT_EQ(payload[7U], 0U);
    EXPECT_EQ(payload[8U], static_cast<uint8_t>(original.src_of_curr_ls));
    EXPECT_EQ(static_cast<int8_t>(payload[9U]), original.curr_ls);
    EXPECT_EQ(payload[10U], static_cast<uint8_t>(original.src_of_ls_change));
    EXPECT_EQ(static_cast<int8_t>(payload[11U]), original.ls_change);
    EXPECT_EQ(castle::read_le32s(&payload[12U]), original.time_to_ls_event);
    EXPECT_EQ(castle::read_le16(&payload[16U]), original.date_of_ls_gps_wn);
    EXPECT_EQ(castle::read_le16(&payload[18U]), original.date_of_ls_gps_dn);
    EXPECT_EQ(payload[20U], 0U);
    EXPECT_EQ(payload[21U], 0U);
    EXPECT_EQ(payload[22U], 0U);
    EXPECT_EQ(payload[23U], original.valid);
}

TEST(UbxMessageNavTimels, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 7U);
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV2;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_NAV_TIMEUTC;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavTimels, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    castle::write_le32(&payload[0U], 7U);

    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length + 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, 0U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavTimels, ValidityBitsSetAndClear)
{
    message value{};
    EXPECT_FALSE(value.valid_curr_ls());
    EXPECT_FALSE(value.valid_time_to_ls_event());

    value.valid = message::VALID_CURR_LS;
    EXPECT_TRUE(value.valid_curr_ls());
    EXPECT_FALSE(value.valid_time_to_ls_event());

    value.valid = message::VALID_TIME_TO_LS_EVENT;
    EXPECT_FALSE(value.valid_curr_ls());
    EXPECT_TRUE(value.valid_time_to_ls_event());

    value.valid = 0xFCU; // undefined bits only
    EXPECT_FALSE(value.valid_curr_ls());
    EXPECT_FALSE(value.valid_time_to_ls_event());
}

TEST(UbxMessageNavTimels, EncodeIntoTooSmallBufferReportsFull)
{
    const message value = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    written = 99U;
    EXPECT_EQ(encode_message(value, frame, UBX_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
