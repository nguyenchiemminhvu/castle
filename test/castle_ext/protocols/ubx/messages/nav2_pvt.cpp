#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav2_pvt.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav2_pvt;
CASTLE_CONSTEXPR uint8_t expected_class = UBX_CLASS_NAV2;
CASTLE_CONSTEXPR uint8_t expected_id = UBX_ID_NAV2_PVT;
CASTLE_CONSTEXPR uint8_t other_class = UBX_CLASS_NAV;
CASTLE_CONSTEXPR castle::size_type length = 92U;

message_view view_of(CASTLE_CONST uint8_t* payload, castle::size_type size,
                     uint8_t msg_class = expected_class, uint8_t msg_id = expected_id)
{
    return message_view{{msg_class, msg_id, static_cast<uint16_t>(size)},
                        array_view<CASTLE_CONST uint8_t>(payload, size), checksum{}};
}

message make_message()
{
    message value;
    value.i_tow = 345600123U;
    value.year = 2026U;
    value.month = 10U;
    value.day = 20U;
    value.hour = 13U;
    value.minute = 47U;
    value.second = 59U;
    value.valid = 0x0FU;
    value.t_acc = 4321U;
    value.nano = -123456789;
    value.fix_type = nav_pvt_fix_type::gnss_dr;
    value.flags = 0xAFU;
    value.flags2 = 0xE0U;
    value.num_sv = 23U;
    value.lon = -1223456789;
    value.lat = 473456789;
    value.height = -12345;
    value.h_msl = 567890;
    value.h_acc = 1234U;
    value.v_acc = 2345U;
    value.vel_n = -1500;
    value.vel_e = 2500;
    value.vel_d = -350;
    value.g_speed = 2915;
    value.head_mot = 12345678;
    value.s_acc = 456U;
    value.head_acc = 98765U;
    value.p_dop = 156U;
    value.flags3 = 0x01U;
    value.head_veh = 23456789;
    value.mag_dec = -345;
    value.mag_acc = 78U;
    return value;
}

// Builds the payload of make_message() byte by byte at the spec offsets.
void build_payload(uint8_t* p, uint8_t reserved_fill)
{
    for (castle::size_type i = 0U; i < length; ++i)
    {
        p[i] = 0U;
    }
    castle::write_le32(&p[0U], 345600123U);
    castle::write_le16(&p[4U], 2026U);
    p[6U] = 10U;
    p[7U] = 20U;
    p[8U] = 13U;
    p[9U] = 47U;
    p[10U] = 59U;
    p[11U] = 0x0FU;
    castle::write_le32(&p[12U], 4321U);
    castle::write_le32s(&p[16U], -123456789);
    p[20U] = 4U;
    p[21U] = 0xAFU;
    p[22U] = 0xE0U;
    p[23U] = 23U;
    castle::write_le32s(&p[24U], -1223456789);
    castle::write_le32s(&p[28U], 473456789);
    castle::write_le32s(&p[32U], -12345);
    castle::write_le32s(&p[36U], 567890);
    castle::write_le32(&p[40U], 1234U);
    castle::write_le32(&p[44U], 2345U);
    castle::write_le32s(&p[48U], -1500);
    castle::write_le32s(&p[52U], 2500);
    castle::write_le32s(&p[56U], -350);
    castle::write_le32s(&p[60U], 2915);
    castle::write_le32s(&p[64U], 12345678);
    castle::write_le32(&p[68U], 456U);
    castle::write_le32(&p[72U], 98765U);
    castle::write_le16(&p[76U], 156U);
    p[78U] = 0x01U;
    for (castle::size_type i = 79U; i < 84U; ++i)
    {
        p[i] = reserved_fill;
    }
    castle::write_le32s(&p[84U], 23456789);
    castle::write_le16s(&p[88U], -345);
    castle::write_le16(&p[90U], 78U);
}

void expect_sample_fields(CASTLE_CONST message& value)
{
    EXPECT_EQ(value.i_tow, 345600123U);
    EXPECT_EQ(value.year, 2026U);
    EXPECT_EQ(value.month, 10U);
    EXPECT_EQ(value.day, 20U);
    EXPECT_EQ(value.hour, 13U);
    EXPECT_EQ(value.minute, 47U);
    EXPECT_EQ(value.second, 59U);
    EXPECT_EQ(value.valid, 0x0FU);
    EXPECT_EQ(value.t_acc, 4321U);
    EXPECT_EQ(value.nano, -123456789);
    EXPECT_EQ(value.fix_type, nav_pvt_fix_type::gnss_dr);
    EXPECT_EQ(value.flags, 0xAFU);
    EXPECT_EQ(value.flags2, 0xE0U);
    EXPECT_EQ(value.num_sv, 23U);
    EXPECT_EQ(value.lon, -1223456789);
    EXPECT_EQ(value.lat, 473456789);
    EXPECT_EQ(value.height, -12345);
    EXPECT_EQ(value.h_msl, 567890);
    EXPECT_EQ(value.h_acc, 1234U);
    EXPECT_EQ(value.v_acc, 2345U);
    EXPECT_EQ(value.vel_n, -1500);
    EXPECT_EQ(value.vel_e, 2500);
    EXPECT_EQ(value.vel_d, -350);
    EXPECT_EQ(value.g_speed, 2915);
    EXPECT_EQ(value.head_mot, 12345678);
    EXPECT_EQ(value.s_acc, 456U);
    EXPECT_EQ(value.head_acc, 98765U);
    EXPECT_EQ(value.p_dop, 156U);
    EXPECT_EQ(value.flags3, 0x01U);
    EXPECT_EQ(value.head_veh, 23456789);
    EXPECT_EQ(value.mag_dec, -345);
    EXPECT_EQ(value.mag_acc, 78U);
}

TEST(UbxMessageNav2Pvt, IdentityConstants)
{
    EXPECT_EQ(message::msg_class, expected_class);
    EXPECT_EQ(message::msg_id, expected_id);
    EXPECT_EQ(message::payload_length, length);
}

TEST(UbxMessageNav2Pvt, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[length];
    build_payload(payload, 0xA5U);
    message out;
    EXPECT_TRUE(message::matches(view_of(payload, length)));
    ASSERT_TRUE(message::decode(view_of(payload, length), out));
    expect_sample_fields(out);
}

TEST(UbxMessageNav2Pvt, EncodeMatchesSpecBytesAndRoundTrips)
{
    uint8_t expected[length];
    build_payload(expected, 0U);

    uint8_t frame[frame_size(length)];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(make_message(), frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(length));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], expected_class);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], expected_id);
    for (castle::size_type i = 0U; i < length; ++i)
    {
        EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + i], expected[i]) << "offset " << i;
    }

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    message out;
    ASSERT_TRUE(message::decode(raw, out));
    expect_sample_fields(out);
}

TEST(UbxMessageNav2Pvt, RejectsWrongClassOrId)
{
    uint8_t payload[length];
    build_payload(payload, 0U);
    message out = make_message();

    EXPECT_FALSE(message::matches(view_of(payload, length, other_class, expected_id)));
    EXPECT_FALSE(message::decode(view_of(payload, length, other_class, expected_id), out));
    EXPECT_EQ(out.i_tow, 0U);

    out = make_message();
    EXPECT_FALSE(message::matches(view_of(payload, length, expected_class, UBX_ID_NAV_STATUS)));
    EXPECT_FALSE(message::decode(view_of(payload, length, expected_class, UBX_ID_NAV_STATUS), out));
    EXPECT_EQ(out.lon, 0);
}

TEST(UbxMessageNav2Pvt, RejectsShortAndLongPayloads)
{
    uint8_t payload[length + 1U];
    build_payload(payload, 0U);
    payload[length] = 0U;
    message out = make_message();

    EXPECT_FALSE(message::decode(view_of(payload, length - 1U), out));
    EXPECT_EQ(out.i_tow, 0U);
    EXPECT_EQ(out.mag_acc, 0U);

    out = make_message();
    EXPECT_FALSE(message::decode(view_of(payload, length + 1U), out));
    EXPECT_EQ(out.i_tow, 0U);
    EXPECT_EQ(out.fix_type, nav_pvt_fix_type::no_fix);

    EXPECT_FALSE(message::decode(view_of(payload, 0U), out));
}

TEST(UbxMessageNav2Pvt, EncodeIntoTooSmallBufferIsFull)
{
    uint8_t frame[frame_size(length)];
    castle::size_type written = 123U;
    EXPECT_EQ(encode_message(make_message(), frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(make_message(), frame, 4U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageNav2Pvt, ValidBits)
{
    message value;
    EXPECT_FALSE(value.valid_date());
    EXPECT_FALSE(value.valid_time());
    EXPECT_FALSE(value.fully_resolved());
    EXPECT_FALSE(value.valid_mag());

    value.valid = message::VALID_DATE;
    EXPECT_TRUE(value.valid_date());
    EXPECT_FALSE(value.valid_time());
    value.valid = message::VALID_TIME;
    EXPECT_TRUE(value.valid_time());
    EXPECT_FALSE(value.valid_date());
    value.valid = message::VALID_FULLY;
    EXPECT_TRUE(value.fully_resolved());
    EXPECT_FALSE(value.valid_mag());
    value.valid = message::VALID_MAG_DEC;
    EXPECT_TRUE(value.valid_mag());
    EXPECT_FALSE(value.fully_resolved());
}

TEST(UbxMessageNav2Pvt, FlagsBitsAndFields)
{
    message value;
    EXPECT_FALSE(value.fix_ok());
    EXPECT_FALSE(value.diff_soln());
    EXPECT_FALSE(value.head_veh_valid());
    EXPECT_EQ(value.psm_state(), nav_pvt_psm_state::not_active);
    EXPECT_EQ(value.carr_soln(), nav_pvt_carr_soln::none);

    value.flags = message::FLAGS_GNSS_FIX_OK;
    EXPECT_TRUE(value.fix_ok());
    EXPECT_FALSE(value.diff_soln());
    value.flags = message::FLAGS_DIFF_SOLN;
    EXPECT_TRUE(value.diff_soln());
    EXPECT_FALSE(value.fix_ok());
    value.flags = message::FLAGS_HEAD_VEH_VALID;
    EXPECT_TRUE(value.head_veh_valid());
    EXPECT_EQ(value.psm_state(), nav_pvt_psm_state::not_active);
    EXPECT_EQ(value.carr_soln(), nav_pvt_carr_soln::none);

    // psmState occupies bits 4..2 and must not leak into neighbouring bits.
    value.flags = static_cast<uint8_t>(5U << 2U);
    EXPECT_EQ(value.psm_state(), nav_pvt_psm_state::inactive);
    EXPECT_FALSE(value.head_veh_valid());
    EXPECT_FALSE(value.diff_soln());
    value.flags = static_cast<uint8_t>(0x23U | (2U << 2U));
    EXPECT_EQ(value.psm_state(), nav_pvt_psm_state::acquisition);
    value.flags = static_cast<uint8_t>(1U << 2U);
    EXPECT_EQ(value.psm_state(), nav_pvt_psm_state::enabled);
    value.flags = static_cast<uint8_t>(3U << 2U);
    EXPECT_EQ(value.psm_state(), nav_pvt_psm_state::tracking);
    value.flags = static_cast<uint8_t>(4U << 2U);
    EXPECT_EQ(value.psm_state(), nav_pvt_psm_state::power_optimized_tracking);
    EXPECT_EQ(value.flags & message::FLAGS_PSM_STATE_MASK, 0x10);

    value.flags = 0x40U;
    EXPECT_EQ(value.carr_soln(), nav_pvt_carr_soln::floating);
    value.flags = static_cast<uint8_t>(0x80U | 0x3FU);
    EXPECT_EQ(value.carr_soln(), nav_pvt_carr_soln::fixed);
    EXPECT_EQ(value.flags & message::FLAGS_CARR_SOLN_MASK, 0x80);

    // Values decoded from the wire sample: flags = 0xAF.
    CASTLE_CONST message sample = make_message();
    EXPECT_TRUE(sample.fix_ok());
    EXPECT_TRUE(sample.diff_soln());
    EXPECT_EQ(sample.psm_state(), nav_pvt_psm_state::tracking);
    EXPECT_TRUE(sample.head_veh_valid());
    EXPECT_EQ(sample.carr_soln(), nav_pvt_carr_soln::fixed);
}

TEST(UbxMessageNav2Pvt, Flags2AndFlags3Bits)
{
    message value;
    EXPECT_FALSE(value.confirmed_avai());
    EXPECT_FALSE(value.confirmed_date());
    EXPECT_FALSE(value.confirmed_time());
    EXPECT_FALSE(value.invalid_llh());

    value.flags2 = message::FLAGS2_CONFIRMED_AVAI;
    EXPECT_TRUE(value.confirmed_avai());
    EXPECT_FALSE(value.confirmed_date());
    EXPECT_FALSE(value.confirmed_time());
    value.flags2 = message::FLAGS2_CONFIRMED_DATE;
    EXPECT_TRUE(value.confirmed_date());
    EXPECT_FALSE(value.confirmed_avai());
    value.flags2 = message::FLAGS2_CONFIRMED_TIME;
    EXPECT_TRUE(value.confirmed_time());
    EXPECT_FALSE(value.confirmed_date());
    value.flags2 = 0x1FU;
    EXPECT_FALSE(value.confirmed_avai());
    EXPECT_FALSE(value.confirmed_date());
    EXPECT_FALSE(value.confirmed_time());

    value.flags3 = message::FLAGS3_INVALID_LLH;
    EXPECT_TRUE(value.invalid_llh());
    value.flags3 = 0xFEU;
    EXPECT_FALSE(value.invalid_llh());
}

TEST(UbxMessageNav2Pvt, ScaledHelpers)
{
    CASTLE_CONST message value = make_message();
    EXPECT_DOUBLE_EQ(value.longitude_deg(), -122.3456789);
    EXPECT_DOUBLE_EQ(value.latitude_deg(), 47.3456789);
    EXPECT_DOUBLE_EQ(value.height_m(), -12.345);
    EXPECT_DOUBLE_EQ(value.h_msl_m(), 567.89);
    EXPECT_DOUBLE_EQ(value.h_acc_m(), 1.234);
    EXPECT_DOUBLE_EQ(value.v_acc_m(), 2.345);
    EXPECT_DOUBLE_EQ(value.vel_n_m_s(), -1.5);
    EXPECT_DOUBLE_EQ(value.vel_e_m_s(), 2.5);
    EXPECT_DOUBLE_EQ(value.vel_d_m_s(), -0.35);
    EXPECT_DOUBLE_EQ(value.g_speed_m_s(), 2.915);
    EXPECT_DOUBLE_EQ(value.s_acc_m_s(), 0.456);
    EXPECT_DOUBLE_EQ(value.head_mot_deg(), 123.45678);
    EXPECT_DOUBLE_EQ(value.head_acc_deg(), 0.98765);
    EXPECT_DOUBLE_EQ(value.head_veh_deg(), 234.56789);
    EXPECT_DOUBLE_EQ(value.pdop_value(), 1.56);
    EXPECT_DOUBLE_EQ(value.mag_dec_deg(), -3.45);
    EXPECT_DOUBLE_EQ(value.mag_acc_deg(), 0.78);

    CASTLE_CONST message zero;
    EXPECT_DOUBLE_EQ(zero.longitude_deg(), 0.0);
    EXPECT_DOUBLE_EQ(zero.head_mot_deg(), 0.0);
}

} // namespace
