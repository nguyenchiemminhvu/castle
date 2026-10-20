#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_timeutc.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_timeutc;

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
    value.t_acc = 0xA1B2C3D4U;
    value.nano = -987654321;
    value.year = 2026U;
    value.month = 10U;
    value.day = 20U;
    value.hour = 23U;
    value.min = 59U;
    value.sec = 60U;
    value.valid = 0x37U;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.t_acc, 0U);
    EXPECT_EQ(value.nano, 0);
    EXPECT_EQ(value.year, 0U);
    EXPECT_EQ(value.month, 0U);
    EXPECT_EQ(value.day, 0U);
    EXPECT_EQ(value.hour, 0U);
    EXPECT_EQ(value.min, 0U);
    EXPECT_EQ(value.sec, 0U);
    EXPECT_EQ(value.valid, 0U);
}

TEST(UbxMessageNavTimeutc, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_TIMEUTC);
    EXPECT_EQ(message::payload_length, 20U);
    EXPECT_EQ(message::VALID_TOW, 0x01U);
    EXPECT_EQ(message::VALID_WKN, 0x02U);
    EXPECT_EQ(message::VALID_UTC, 0x04U);
    EXPECT_EQ(message::VALID_UTC_STANDARD_MASK, 0xF0U);
    EXPECT_EQ(message::VALID_UTC_STANDARD_SHIFT, 4U);
}

TEST(UbxMessageNavTimeutc, UtcStandardEnumMatchesSpec)
{
    EXPECT_EQ(static_cast<uint8_t>(nav_timeutc_utc_standard::not_available), 0U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timeutc_utc_standard::crl), 1U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timeutc_utc_standard::nist), 2U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timeutc_utc_standard::usno), 3U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timeutc_utc_standard::bipm), 4U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timeutc_utc_standard::european_laboratories), 5U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timeutc_utc_standard::former_soviet_union), 6U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timeutc_utc_standard::ntsc), 7U);
    EXPECT_EQ(static_cast<uint8_t>(nav_timeutc_utc_standard::unknown), 15U);
}

TEST(UbxMessageNavTimeutc, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.t_acc, original.t_acc);
    EXPECT_EQ(decoded.nano, original.nano);
    EXPECT_EQ(decoded.year, original.year);
    EXPECT_EQ(decoded.month, original.month);
    EXPECT_EQ(decoded.day, original.day);
    EXPECT_EQ(decoded.hour, original.hour);
    EXPECT_EQ(decoded.min, original.min);
    EXPECT_EQ(decoded.sec, original.sec);
    EXPECT_EQ(decoded.valid, original.valid);
}

TEST(UbxMessageNavTimeutc, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 504000123U);
    castle::write_le32(&payload[4U], 25U);
    castle::write_le32s(&payload[8U], -12345);
    castle::write_le16(&payload[12U], 2099U);
    payload[14U] = 12U;
    payload[15U] = 31U;
    payload[16U] = 22U;
    payload[17U] = 58U;
    payload[18U] = 57U;
    payload[19U] = 0x27U;

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 504000123U);
    EXPECT_EQ(decoded.t_acc, 25U);
    EXPECT_EQ(decoded.nano, -12345);
    EXPECT_EQ(decoded.year, 2099U);
    EXPECT_EQ(decoded.month, 12U);
    EXPECT_EQ(decoded.day, 31U);
    EXPECT_EQ(decoded.hour, 22U);
    EXPECT_EQ(decoded.min, 58U);
    EXPECT_EQ(decoded.sec, 57U);
    EXPECT_EQ(decoded.valid, 0x27U);
    EXPECT_TRUE(decoded.valid_tow());
    EXPECT_TRUE(decoded.valid_wkn());
    EXPECT_TRUE(decoded.valid_utc());
    EXPECT_EQ(decoded.utc_standard(), nav_timeutc_utc_standard::nist);
}

TEST(UbxMessageNavTimeutc, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_NAV);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_NAV_TIMEUTC);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(castle::read_le32(&payload[0U]), original.i_tow);
    EXPECT_EQ(castle::read_le32(&payload[4U]), original.t_acc);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.nano);
    EXPECT_EQ(castle::read_le16(&payload[12U]), original.year);
    EXPECT_EQ(payload[14U], original.month);
    EXPECT_EQ(payload[15U], original.day);
    EXPECT_EQ(payload[16U], original.hour);
    EXPECT_EQ(payload[17U], original.min);
    EXPECT_EQ(payload[18U], original.sec);
    EXPECT_EQ(payload[19U], original.valid);
}

TEST(UbxMessageNavTimeutc, RejectsWrongClassOrId)
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
    raw.header.msg_id = UBX_ID_NAV_CLOCK; // same 20-byte length
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavTimeutc, RejectsShortAndLongPayloads)
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

TEST(UbxMessageNavTimeutc, ValidityBitsSetAndClear)
{
    message value{};
    EXPECT_FALSE(value.valid_tow());
    EXPECT_FALSE(value.valid_wkn());
    EXPECT_FALSE(value.valid_utc());

    value.valid = message::VALID_TOW;
    EXPECT_TRUE(value.valid_tow());
    EXPECT_FALSE(value.valid_wkn());
    EXPECT_FALSE(value.valid_utc());

    value.valid = message::VALID_WKN;
    EXPECT_FALSE(value.valid_tow());
    EXPECT_TRUE(value.valid_wkn());
    EXPECT_FALSE(value.valid_utc());

    value.valid = message::VALID_UTC;
    EXPECT_FALSE(value.valid_tow());
    EXPECT_FALSE(value.valid_wkn());
    EXPECT_TRUE(value.valid_utc());

    // utcStandard bits never leak into the validity flags.
    value.valid = message::VALID_UTC_STANDARD_MASK;
    EXPECT_FALSE(value.valid_tow());
    EXPECT_FALSE(value.valid_wkn());
    EXPECT_FALSE(value.valid_utc());
}

TEST(UbxMessageNavTimeutc, UtcStandardReadsUpperNibble)
{
    message value{};
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::not_available);

    value.valid = 0x07U; // validity bits only
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::not_available);

    value.valid = 0x37U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::usno);

    value.valid = 0x70U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::ntsc);

    value.valid = 0xF0U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::unknown);

    value.valid = 0x90U; // reserved value passes through unchanged
    EXPECT_EQ(static_cast<uint8_t>(value.utc_standard()), 9U);
}

TEST(UbxMessageNavTimeutc, EncodeIntoTooSmallBufferReportsFull)
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
