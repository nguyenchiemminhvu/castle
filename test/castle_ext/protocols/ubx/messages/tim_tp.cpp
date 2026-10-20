#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/tim_tp.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = tim_tp;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

message distinctive()
{
    message value{};
    value.tow_ms = 0x11223344U;
    value.tow_sub_ms = 0x89ABCDEFU;
    value.q_err = -1234567;
    value.week = 0x0A0BU;
    value.flags = 0x1BU;
    value.ref_info = 0x73U;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.tow_ms, 0U);
    EXPECT_EQ(value.tow_sub_ms, 0U);
    EXPECT_EQ(value.q_err, 0);
    EXPECT_EQ(value.week, 0U);
    EXPECT_EQ(value.flags, 0U);
    EXPECT_EQ(value.ref_info, 0U);
}

TEST(UbxMessageTimTp, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_TIM);
    EXPECT_EQ(message::msg_id, UBX_ID_TIM_TP);
    EXPECT_EQ(message::msg_class, 0x0DU);
    EXPECT_EQ(message::msg_id, 0x01U);
    EXPECT_EQ(message::payload_length, 16U);
    EXPECT_EQ(message::FLAGS_TIME_BASE, 0x01U);
    EXPECT_EQ(message::FLAGS_UTC, 0x02U);
    EXPECT_EQ(message::FLAGS_RAIM, 0x0CU);
    EXPECT_EQ(message::FLAGS_Q_ERR_INVALID, 0x10U);
    EXPECT_EQ(message::REF_INFO_TIME_REF_GNSS, 0x0FU);
    EXPECT_EQ(message::REF_INFO_UTC_STANDARD, 0xF0U);
}

TEST(UbxMessageTimTp, EnumValuesMatchSpec)
{
    EXPECT_EQ(static_cast<uint8_t>(tim_tp_time_base::gnss), 0U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tp_time_base::utc), 1U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tp_raim::not_available), 0U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tp_raim::not_active), 1U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tp_raim::active), 2U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tp_time_ref_gnss::gps), 0U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tp_time_ref_gnss::glonass), 1U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tp_time_ref_gnss::beidou), 2U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tp_time_ref_gnss::galileo), 3U);
    EXPECT_EQ(static_cast<uint8_t>(tim_tp_time_ref_gnss::unknown), 15U);
}

TEST(UbxMessageTimTp, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.tow_ms, original.tow_ms);
    EXPECT_EQ(decoded.tow_sub_ms, original.tow_sub_ms);
    EXPECT_EQ(decoded.q_err, original.q_err);
    EXPECT_EQ(decoded.week, original.week);
    EXPECT_EQ(decoded.flags, original.flags);
    EXPECT_EQ(decoded.ref_info, original.ref_info);
}

TEST(UbxMessageTimTp, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 345600000U);
    castle::write_le32(&payload[4U], 0x40000000U);
    castle::write_le32s(&payload[8U], -2500);
    castle::write_le16(&payload[12U], 2310U);
    payload[14U] = 0x17U; // timeBase=UTC, utc, raim=1 (not active), qErrInvalid
    payload[15U] = 0x2FU;

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.tow_ms, 345600000U);
    EXPECT_EQ(decoded.tow_sub_ms, 0x40000000U);
    EXPECT_EQ(decoded.q_err, -2500);
    EXPECT_EQ(decoded.week, 2310U);
    EXPECT_EQ(decoded.flags, 0x17U);
    EXPECT_EQ(decoded.ref_info, 0x2FU);
    EXPECT_EQ(decoded.time_base(), tim_tp_time_base::utc);
    EXPECT_TRUE(decoded.utc_available());
    EXPECT_EQ(decoded.raim(), tim_tp_raim::not_active);
    EXPECT_TRUE(decoded.q_err_invalid());
    EXPECT_EQ(decoded.time_ref_gnss(), tim_tp_time_ref_gnss::unknown);
    EXPECT_EQ(decoded.utc_standard(), nav_timeutc_utc_standard::nist);
    EXPECT_DOUBLE_EQ(decoded.tow_sub_ms_ms(), 0.25);
    EXPECT_DOUBLE_EQ(decoded.q_err_ns(), -2.5);
}

TEST(UbxMessageTimTp, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_TIM);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_TIM_TP);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), message::payload_length);
    EXPECT_EQ(castle::read_le32(&payload[0U]), original.tow_ms);
    EXPECT_EQ(castle::read_le32(&payload[4U]), original.tow_sub_ms);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.q_err);
    EXPECT_EQ(castle::read_le16(&payload[12U]), original.week);
    EXPECT_EQ(payload[14U], original.flags);
    EXPECT_EQ(payload[15U], original.ref_info);
}

TEST(UbxMessageTimTp, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    payload[0U] = 1U;
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_TIM_TM2;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageTimTp, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    payload[0U] = 1U;

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

TEST(UbxMessageTimTp, TimeBaseAndUtcAvailable)
{
    message value{};
    EXPECT_EQ(value.time_base(), tim_tp_time_base::gnss);
    EXPECT_FALSE(value.utc_available());

    value.flags = message::FLAGS_TIME_BASE;
    EXPECT_EQ(value.time_base(), tim_tp_time_base::utc);
    EXPECT_FALSE(value.utc_available());

    value.flags = message::FLAGS_UTC;
    EXPECT_EQ(value.time_base(), tim_tp_time_base::gnss);
    EXPECT_TRUE(value.utc_available());

    value.flags = static_cast<uint8_t>(~message::FLAGS_TIME_BASE);
    EXPECT_EQ(value.time_base(), tim_tp_time_base::gnss);
    EXPECT_TRUE(value.utc_available());

    value.flags = static_cast<uint8_t>(~message::FLAGS_UTC);
    EXPECT_EQ(value.time_base(), tim_tp_time_base::utc);
    EXPECT_FALSE(value.utc_available());
}

TEST(UbxMessageTimTp, RaimAllValues)
{
    message value{};
    value.flags = 0x00U;
    EXPECT_EQ(value.raim(), tim_tp_raim::not_available);
    value.flags = 0x04U;
    EXPECT_EQ(value.raim(), tim_tp_raim::not_active);
    value.flags = 0x08U;
    EXPECT_EQ(value.raim(), tim_tp_raim::active);

    // Neighbouring bits do not leak into the RAIM field.
    value.flags = static_cast<uint8_t>(~message::FLAGS_RAIM);
    EXPECT_EQ(value.raim(), tim_tp_raim::not_available);
    value.flags = 0xFBU; // raim bits = 10
    EXPECT_EQ(value.raim(), tim_tp_raim::active);
    // Reserved value 3 is passed through.
    value.flags = message::FLAGS_RAIM;
    EXPECT_EQ(static_cast<uint8_t>(value.raim()), 3U);
}

TEST(UbxMessageTimTp, QErrInvalidSetAndClear)
{
    message value{};
    EXPECT_FALSE(value.q_err_invalid());
    value.flags = message::FLAGS_Q_ERR_INVALID;
    EXPECT_TRUE(value.q_err_invalid());
    value.flags = static_cast<uint8_t>(~message::FLAGS_Q_ERR_INVALID);
    EXPECT_FALSE(value.q_err_invalid());
}

TEST(UbxMessageTimTp, TimeRefGnssAllValues)
{
    message value{};
    value.ref_info = 0x00U;
    EXPECT_EQ(value.time_ref_gnss(), tim_tp_time_ref_gnss::gps);
    value.ref_info = 0x01U;
    EXPECT_EQ(value.time_ref_gnss(), tim_tp_time_ref_gnss::glonass);
    value.ref_info = 0x02U;
    EXPECT_EQ(value.time_ref_gnss(), tim_tp_time_ref_gnss::beidou);
    value.ref_info = 0x03U;
    EXPECT_EQ(value.time_ref_gnss(), tim_tp_time_ref_gnss::galileo);
    value.ref_info = 0x0FU;
    EXPECT_EQ(value.time_ref_gnss(), tim_tp_time_ref_gnss::unknown);

    // utcStandard nibble does not leak into timeRefGnss.
    value.ref_info = 0xF2U;
    EXPECT_EQ(value.time_ref_gnss(), tim_tp_time_ref_gnss::beidou);
    value.ref_info = 0xF0U;
    EXPECT_EQ(value.time_ref_gnss(), tim_tp_time_ref_gnss::gps);
}

TEST(UbxMessageTimTp, UtcStandardValues)
{
    message value{};
    value.ref_info = 0x00U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::not_available);
    value.ref_info = 0x10U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::crl);
    value.ref_info = 0x20U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::nist);
    value.ref_info = 0x30U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::usno);
    value.ref_info = 0x40U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::bipm);
    value.ref_info = 0x50U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::european_laboratories);
    value.ref_info = 0x60U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::former_soviet_union);
    value.ref_info = 0x70U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::ntsc);
    value.ref_info = 0xF0U;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::unknown);

    // timeRefGnss nibble does not leak into utcStandard.
    value.ref_info = 0x3FU;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::usno);
    value.ref_info = 0x0FU;
    EXPECT_EQ(value.utc_standard(), nav_timeutc_utc_standard::not_available);
}

TEST(UbxMessageTimTp, ScaledHelpers)
{
    message value{};
    EXPECT_EQ(value.tow_sub_ms_ms(), 0.0);
    EXPECT_EQ(value.q_err_ns(), 0.0);

    value.tow_sub_ms = 0x80000000U;
    EXPECT_DOUBLE_EQ(value.tow_sub_ms_ms(), 0.5);
    value.tow_sub_ms = 1U;
    EXPECT_DOUBLE_EQ(value.tow_sub_ms_ms(), 1.0 / 4294967296.0);
    value.tow_sub_ms = 0xC0000000U;
    EXPECT_DOUBLE_EQ(value.tow_sub_ms_ms(), 0.75);
    value.tow_sub_ms = 0xFFFFFFFFU;
    EXPECT_LT(value.tow_sub_ms_ms(), 1.0);
    EXPECT_DOUBLE_EQ(value.tow_sub_ms_ms(), 4294967295.0 / 4294967296.0);

    value.q_err = 1500;
    EXPECT_DOUBLE_EQ(value.q_err_ns(), 1.5);
    value.q_err = -1234567;
    EXPECT_NEAR(value.q_err_ns(), -1234.567, 1e-9);
    value.q_err = 1;
    EXPECT_DOUBLE_EQ(value.q_err_ns(), 0.001);
}

TEST(UbxMessageTimTp, EncodeIntoTooSmallBufferReportsFull)
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
