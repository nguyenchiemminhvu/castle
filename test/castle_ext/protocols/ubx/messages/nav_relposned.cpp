#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_relposned.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_relposned;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

message distinctive()
{
    message value{};
    value.version = 0x01U;
    value.ref_station_id = 0x0ABCU;
    value.i_tow = 0x11223344U;
    value.rel_pos_n = -101010;
    value.rel_pos_e = 202020;
    value.rel_pos_d = -303030;
    value.rel_pos_length = 404040;
    value.rel_pos_heading = -17999999;
    value.rel_pos_hp_n = -99;
    value.rel_pos_hp_e = 98;
    value.rel_pos_hp_d = -97;
    value.rel_pos_hp_length = 96;
    value.acc_n = 0x01020304U;
    value.acc_e = 0x05060708U;
    value.acc_d = 0x090A0B0CU;
    value.acc_length = 0x0D0E0F10U;
    value.acc_heading = 0x11121314U;
    value.flags = 0x000003FFU;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.version, 0U);
    EXPECT_EQ(value.ref_station_id, 0U);
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.rel_pos_n, 0);
    EXPECT_EQ(value.rel_pos_e, 0);
    EXPECT_EQ(value.rel_pos_d, 0);
    EXPECT_EQ(value.rel_pos_length, 0);
    EXPECT_EQ(value.rel_pos_heading, 0);
    EXPECT_EQ(value.rel_pos_hp_n, 0);
    EXPECT_EQ(value.rel_pos_hp_e, 0);
    EXPECT_EQ(value.rel_pos_hp_d, 0);
    EXPECT_EQ(value.rel_pos_hp_length, 0);
    EXPECT_EQ(value.acc_n, 0U);
    EXPECT_EQ(value.acc_e, 0U);
    EXPECT_EQ(value.acc_d, 0U);
    EXPECT_EQ(value.acc_length, 0U);
    EXPECT_EQ(value.acc_heading, 0U);
    EXPECT_EQ(value.flags, 0U);
}

TEST(UbxMessageNavRelposned, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_RELPOSNED);
    EXPECT_EQ(message::payload_length, 64U);
    EXPECT_EQ(message::FLAGS_GNSS_FIX_OK, 1U << 0U);
    EXPECT_EQ(message::FLAGS_DIFF_SOLN, 1U << 1U);
    EXPECT_EQ(message::FLAGS_REL_POS_VALID, 1U << 2U);
    EXPECT_EQ(message::FLAGS_CARR_SOLN_MASK, 3U << 3U);
    EXPECT_EQ(message::FLAGS_CARR_SOLN_SHIFT, 3U);
    EXPECT_EQ(message::FLAGS_IS_MOVING, 1U << 5U);
    EXPECT_EQ(message::FLAGS_REF_POS_MISS, 1U << 6U);
    EXPECT_EQ(message::FLAGS_REF_OBS_MISS, 1U << 7U);
    EXPECT_EQ(message::FLAGS_REL_POS_HEADING_VALID, 1U << 8U);
    EXPECT_EQ(message::FLAGS_REL_POS_NORMALIZED, 1U << 9U);
    EXPECT_EQ(static_cast<uint8_t>(nav_relposned_carr_soln::none), 0U);
    EXPECT_EQ(static_cast<uint8_t>(nav_relposned_carr_soln::floating), 1U);
    EXPECT_EQ(static_cast<uint8_t>(nav_relposned_carr_soln::fixed), 2U);
}

TEST(UbxMessageNavRelposned, RoundTripPreservesEveryField)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(message::matches(raw));

    message decoded{};
    ASSERT_TRUE(message::decode(raw, decoded));
    EXPECT_EQ(decoded.version, original.version);
    EXPECT_EQ(decoded.ref_station_id, original.ref_station_id);
    EXPECT_EQ(decoded.i_tow, original.i_tow);
    EXPECT_EQ(decoded.rel_pos_n, original.rel_pos_n);
    EXPECT_EQ(decoded.rel_pos_e, original.rel_pos_e);
    EXPECT_EQ(decoded.rel_pos_d, original.rel_pos_d);
    EXPECT_EQ(decoded.rel_pos_length, original.rel_pos_length);
    EXPECT_EQ(decoded.rel_pos_heading, original.rel_pos_heading);
    EXPECT_EQ(decoded.rel_pos_hp_n, original.rel_pos_hp_n);
    EXPECT_EQ(decoded.rel_pos_hp_e, original.rel_pos_hp_e);
    EXPECT_EQ(decoded.rel_pos_hp_d, original.rel_pos_hp_d);
    EXPECT_EQ(decoded.rel_pos_hp_length, original.rel_pos_hp_length);
    EXPECT_EQ(decoded.acc_n, original.acc_n);
    EXPECT_EQ(decoded.acc_e, original.acc_e);
    EXPECT_EQ(decoded.acc_d, original.acc_d);
    EXPECT_EQ(decoded.acc_length, original.acc_length);
    EXPECT_EQ(decoded.acc_heading, original.acc_heading);
    EXPECT_EQ(decoded.flags, original.flags);
}

TEST(UbxMessageNavRelposned, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    payload[0U] = 0x01U;
    payload[1U] = 0xEEU; // reserved0
    castle::write_le16(&payload[2U], 4095U);
    castle::write_le32(&payload[4U], 100001U);
    castle::write_le32s(&payload[8U], -200002);
    castle::write_le32s(&payload[12U], 300003);
    castle::write_le32s(&payload[16U], -400004);
    castle::write_le32s(&payload[20U], 500005);
    castle::write_le32s(&payload[24U], -600006);
    castle::write_le32(&payload[28U], 0xEEEEEEEEU); // reserved1
    castle::write_le8s(&payload[32U], -11);
    castle::write_le8s(&payload[33U], 22);
    castle::write_le8s(&payload[34U], -33);
    castle::write_le8s(&payload[35U], 44);
    castle::write_le32(&payload[36U], 700007U);
    castle::write_le32(&payload[40U], 800008U);
    castle::write_le32(&payload[44U], 900009U);
    castle::write_le32(&payload[48U], 1000010U);
    castle::write_le32(&payload[52U], 1100011U);
    castle::write_le32(&payload[56U], 0xEEEEEEEEU); // reserved2
    castle::write_le32(&payload[60U], 0x00000215U);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.version, 0x01U);
    EXPECT_EQ(decoded.ref_station_id, 4095U);
    EXPECT_EQ(decoded.i_tow, 100001U);
    EXPECT_EQ(decoded.rel_pos_n, -200002);
    EXPECT_EQ(decoded.rel_pos_e, 300003);
    EXPECT_EQ(decoded.rel_pos_d, -400004);
    EXPECT_EQ(decoded.rel_pos_length, 500005);
    EXPECT_EQ(decoded.rel_pos_heading, -600006);
    EXPECT_EQ(decoded.rel_pos_hp_n, -11);
    EXPECT_EQ(decoded.rel_pos_hp_e, 22);
    EXPECT_EQ(decoded.rel_pos_hp_d, -33);
    EXPECT_EQ(decoded.rel_pos_hp_length, 44);
    EXPECT_EQ(decoded.acc_n, 700007U);
    EXPECT_EQ(decoded.acc_e, 800008U);
    EXPECT_EQ(decoded.acc_d, 900009U);
    EXPECT_EQ(decoded.acc_length, 1000010U);
    EXPECT_EQ(decoded.acc_heading, 1100011U);
    EXPECT_EQ(decoded.flags, 0x00000215U);
    // 0x215 = gnssFixOK | relPosValid | carrSoln=2 | relPosNormalized.
    EXPECT_TRUE(decoded.gnss_fix_ok());
    EXPECT_FALSE(decoded.diff_soln());
    EXPECT_TRUE(decoded.rel_pos_valid());
    EXPECT_EQ(decoded.carr_soln(), nav_relposned_carr_soln::fixed);
    EXPECT_FALSE(decoded.is_moving());
    EXPECT_FALSE(decoded.rel_pos_heading_valid());
    EXPECT_TRUE(decoded.rel_pos_normalized());
}

TEST(UbxMessageNavRelposned, EncodesFieldsAtSpecOffsetsWithZeroReserved)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(payload[0U], original.version);
    EXPECT_EQ(payload[1U], 0U);
    EXPECT_EQ(castle::read_le16(&payload[2U]), original.ref_station_id);
    EXPECT_EQ(castle::read_le32(&payload[4U]), original.i_tow);
    EXPECT_EQ(castle::read_le32s(&payload[8U]), original.rel_pos_n);
    EXPECT_EQ(castle::read_le32s(&payload[12U]), original.rel_pos_e);
    EXPECT_EQ(castle::read_le32s(&payload[16U]), original.rel_pos_d);
    EXPECT_EQ(castle::read_le32s(&payload[20U]), original.rel_pos_length);
    EXPECT_EQ(castle::read_le32s(&payload[24U]), original.rel_pos_heading);
    EXPECT_EQ(castle::read_le32(&payload[28U]), 0U);
    EXPECT_EQ(castle::read_le8s(&payload[32U]), original.rel_pos_hp_n);
    EXPECT_EQ(castle::read_le8s(&payload[33U]), original.rel_pos_hp_e);
    EXPECT_EQ(castle::read_le8s(&payload[34U]), original.rel_pos_hp_d);
    EXPECT_EQ(castle::read_le8s(&payload[35U]), original.rel_pos_hp_length);
    EXPECT_EQ(castle::read_le32(&payload[36U]), original.acc_n);
    EXPECT_EQ(castle::read_le32(&payload[40U]), original.acc_e);
    EXPECT_EQ(castle::read_le32(&payload[44U]), original.acc_d);
    EXPECT_EQ(castle::read_le32(&payload[48U]), original.acc_length);
    EXPECT_EQ(castle::read_le32(&payload[52U]), original.acc_heading);
    EXPECT_EQ(castle::read_le32(&payload[56U]), 0U);
    EXPECT_EQ(castle::read_le32(&payload[60U]), original.flags);
}

TEST(UbxMessageNavRelposned, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV2;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_NAV_PVT;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavRelposned, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    message decoded = distinctive();
    // 40 bytes is the length of the obsolete version 0x00 layout; not accepted.
    EXPECT_FALSE(message::decode(view_of(payload, 40U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length + 1U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavRelposned, SingleBitAccessorsSetAndClear)
{
    struct case_t
    {
        uint32_t mask;
        bool (message::*accessor)() const;
    };
    const case_t cases[] = {
        {message::FLAGS_GNSS_FIX_OK, &message::gnss_fix_ok},
        {message::FLAGS_DIFF_SOLN, &message::diff_soln},
        {message::FLAGS_REL_POS_VALID, &message::rel_pos_valid},
        {message::FLAGS_IS_MOVING, &message::is_moving},
        {message::FLAGS_REF_POS_MISS, &message::ref_pos_miss},
        {message::FLAGS_REF_OBS_MISS, &message::ref_obs_miss},
        {message::FLAGS_REL_POS_HEADING_VALID, &message::rel_pos_heading_valid},
        {message::FLAGS_REL_POS_NORMALIZED, &message::rel_pos_normalized},
    };

    for (const case_t& entry : cases)
    {
        message value{};
        value.flags = entry.mask;
        EXPECT_TRUE((value.*entry.accessor)()) << "mask " << entry.mask;
        value.flags = ~entry.mask;
        EXPECT_FALSE((value.*entry.accessor)()) << "mask " << entry.mask;
        value.flags = 0U;
        EXPECT_FALSE((value.*entry.accessor)()) << "mask " << entry.mask;
    }
}

TEST(UbxMessageNavRelposned, CarrSolnExtractsBitsFourToThree)
{
    message value{};
    EXPECT_EQ(value.carr_soln(), nav_relposned_carr_soln::none);

    value.flags = 0x00000008U;
    EXPECT_EQ(value.carr_soln(), nav_relposned_carr_soln::floating);

    value.flags = 0x00000010U;
    EXPECT_EQ(value.carr_soln(), nav_relposned_carr_soln::fixed);

    // Neighbouring bits do not leak into the field.
    value.flags = ~message::FLAGS_CARR_SOLN_MASK;
    EXPECT_EQ(value.carr_soln(), nav_relposned_carr_soln::none);

    // The reserved value 3 is passed through unchanged.
    value.flags = 0xFFFFFFFFU;
    EXPECT_EQ(static_cast<uint8_t>(value.carr_soln()), 3U);
}

TEST(UbxMessageNavRelposned, CombinesCentimetreAndHighPrecisionParts)
{
    message value{};
    value.rel_pos_n = 87;          // 0.87 m
    value.rel_pos_hp_n = 3;        // +0.3 mm
    value.rel_pos_e = -87;         // -0.87 m
    value.rel_pos_hp_e = -99;      // -9.9 mm
    value.rel_pos_d = 1;           // 0.01 m
    value.rel_pos_hp_d = -99;      // -9.9 mm -> 0.1 mm net
    value.rel_pos_length = 123456; // 1234.56 m
    value.rel_pos_hp_length = 78;  // +7.8 mm
    value.rel_pos_heading = -9012345;
    value.acc_n = 80U;
    value.acc_e = 81U;
    value.acc_d = 150U;
    value.acc_length = 90U;
    value.acc_heading = 25000U;
    EXPECT_NEAR(value.rel_pos_n_m(), 0.8703, 1e-12);
    EXPECT_NEAR(value.rel_pos_e_m(), -0.8799, 1e-12);
    EXPECT_NEAR(value.rel_pos_d_m(), 0.0001, 1e-12);
    EXPECT_NEAR(value.rel_pos_length_m(), 1234.5678, 1e-9);
    EXPECT_NEAR(value.rel_pos_heading_deg(), -90.12345, 1e-12);
    EXPECT_NEAR(value.acc_n_m(), 0.008, 1e-15);
    EXPECT_NEAR(value.acc_e_m(), 0.0081, 1e-15);
    EXPECT_NEAR(value.acc_d_m(), 0.015, 1e-15);
    EXPECT_NEAR(value.acc_length_m(), 0.009, 1e-15);
    EXPECT_NEAR(value.acc_heading_deg(), 0.25, 1e-15);
}

TEST(UbxMessageNavRelposned, EncodeIntoTooSmallBufferReportsFull)
{
    const message value = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
