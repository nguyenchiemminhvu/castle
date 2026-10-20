#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/esf_ins.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

esf_ins sample_dynamics()
{
    esf_ins value{};
    value.bitfield0 = 0x00002A01U; // version 1, yAngRate, xAccel, zAccel valid
    value.i_tow = 0x0A0B0C0DU;
    value.x_ang_rate = 0x01020304;
    value.y_ang_rate = -0x01020305;
    value.z_ang_rate = 0x05060708;
    value.x_accel = -0x090A0B0C;
    value.y_accel = 0x0D0E0F10;
    value.z_accel = -0x11121314;
    return value;
}

TEST(UbxMessageEsfIns, BitfieldMasksMatchSpecBitPositions)
{
    // Old masks were 1..32 (bits 0..5); spec places them at bits 8..13 above the version byte.
    EXPECT_EQ(esf_ins::BF0_VERSION, 0x000000FFU);
    EXPECT_EQ(esf_ins::BF0_X_ANG_RATE_VALID, 1U << 8U);
    EXPECT_EQ(esf_ins::BF0_Y_ANG_RATE_VALID, 1U << 9U);
    EXPECT_EQ(esf_ins::BF0_Z_ANG_RATE_VALID, 1U << 10U);
    EXPECT_EQ(esf_ins::BF0_X_ACCEL_VALID, 1U << 11U);
    EXPECT_EQ(esf_ins::BF0_Y_ACCEL_VALID, 1U << 12U);
    EXPECT_EQ(esf_ins::BF0_Z_ACCEL_VALID, 1U << 13U);
    EXPECT_EQ(esf_ins::msg_class, 0x10U);
    EXPECT_EQ(esf_ins::msg_id, 0x15U);
    EXPECT_EQ(esf_ins::payload_length, 36U);
}

TEST(UbxMessageEsfIns, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[36U] = {};
    castle::write_le32(&payload[0U], 0x00003F01U); // version 1, every axis valid
    castle::write_le32(&payload[4U], 0xDEADBEEFU); // reserved0 is ignored
    castle::write_le32(&payload[8U], 403200000U);
    castle::write_le32(&payload[12U], static_cast<uint32_t>(12345));
    castle::write_le32(&payload[16U], static_cast<uint32_t>(-23456));
    castle::write_le32(&payload[20U], static_cast<uint32_t>(34567));
    castle::write_le32(&payload[24U], static_cast<uint32_t>(-981));
    castle::write_le32(&payload[28U], static_cast<uint32_t>(55));
    castle::write_le32(&payload[32U], static_cast<uint32_t>(-66));

    message_view raw = make_view(UBX_CLASS_ESF, UBX_ID_ESF_INS, payload, 36U);
    esf_ins out;
    ASSERT_TRUE(esf_ins::matches(raw));
    ASSERT_TRUE(esf_ins::decode(raw, out));
    EXPECT_EQ(out.bitfield0, 0x00003F01U);
    EXPECT_EQ(out.version(), 1U);
    EXPECT_EQ(out.i_tow, 403200000U);
    EXPECT_EQ(out.x_ang_rate, 12345);
    EXPECT_EQ(out.y_ang_rate, -23456);
    EXPECT_EQ(out.z_ang_rate, 34567);
    EXPECT_EQ(out.x_accel, -981);
    EXPECT_EQ(out.y_accel, 55);
    EXPECT_EQ(out.z_accel, -66);
    EXPECT_TRUE(out.x_ang_rate_valid());
    EXPECT_TRUE(out.y_ang_rate_valid());
    EXPECT_TRUE(out.z_ang_rate_valid());
    EXPECT_TRUE(out.x_accel_valid());
    EXPECT_TRUE(out.y_accel_valid());
    EXPECT_TRUE(out.z_accel_valid());
    EXPECT_DOUBLE_EQ(out.x_ang_rate_deg_s(), 12.345);
    EXPECT_DOUBLE_EQ(out.y_ang_rate_deg_s(), -23.456);
    EXPECT_DOUBLE_EQ(out.z_ang_rate_deg_s(), 34.567);
    EXPECT_DOUBLE_EQ(out.x_accel_m_s2(), -9.81);
    EXPECT_DOUBLE_EQ(out.y_accel_m_s2(), 0.55);
    EXPECT_DOUBLE_EQ(out.z_accel_m_s2(), -0.66);
}

TEST(UbxMessageEsfIns, RoundTripsEveryFieldAndZeroesReserved)
{
    CASTLE_CONST esf_ins in = sample_dynamics();
    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(36U));
    for (castle::size_type index = 4U; index < 8U; ++index)
    {
        EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + index], 0U);
    }

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    esf_ins out;
    ASSERT_TRUE(esf_ins::decode(raw, out));
    EXPECT_EQ(out.bitfield0, in.bitfield0);
    EXPECT_EQ(out.i_tow, in.i_tow);
    EXPECT_EQ(out.x_ang_rate, in.x_ang_rate);
    EXPECT_EQ(out.y_ang_rate, in.y_ang_rate);
    EXPECT_EQ(out.z_ang_rate, in.z_ang_rate);
    EXPECT_EQ(out.x_accel, in.x_accel);
    EXPECT_EQ(out.y_accel, in.y_accel);
    EXPECT_EQ(out.z_accel, in.z_accel);
    EXPECT_FALSE(out.x_ang_rate_valid());
    EXPECT_TRUE(out.y_ang_rate_valid());
    EXPECT_FALSE(out.z_ang_rate_valid());
    EXPECT_TRUE(out.x_accel_valid());
    EXPECT_FALSE(out.y_accel_valid());
    EXPECT_TRUE(out.z_accel_valid());
}

TEST(UbxMessageEsfIns, VersionIsIndependentOfValidityBits)
{
    esf_ins value{};
    value.bitfield0 = 0xFFFFFF00U;
    EXPECT_EQ(value.version(), 0U);
    value.bitfield0 = 0x000000FFU;
    EXPECT_EQ(value.version(), 0xFFU);
    EXPECT_FALSE(value.x_ang_rate_valid());
    EXPECT_FALSE(value.y_ang_rate_valid());
    EXPECT_FALSE(value.z_ang_rate_valid());
    EXPECT_FALSE(value.x_accel_valid());
    EXPECT_FALSE(value.y_accel_valid());
    EXPECT_FALSE(value.z_accel_valid());
    value.bitfield0 = 0xFFFFC0FFU; // everything except bits 13..8
    EXPECT_FALSE(value.x_ang_rate_valid());
    EXPECT_FALSE(value.z_accel_valid());
}

TEST(UbxMessageEsfIns, EachValidityBitSetsOnlyItsAccessor)
{
    esf_ins value{};
    value.bitfield0 = esf_ins::BF0_X_ANG_RATE_VALID;
    EXPECT_TRUE(value.x_ang_rate_valid());
    EXPECT_FALSE(value.y_ang_rate_valid());
    value.bitfield0 = esf_ins::BF0_Y_ANG_RATE_VALID;
    EXPECT_TRUE(value.y_ang_rate_valid());
    EXPECT_FALSE(value.z_ang_rate_valid());
    value.bitfield0 = esf_ins::BF0_Z_ANG_RATE_VALID;
    EXPECT_TRUE(value.z_ang_rate_valid());
    EXPECT_FALSE(value.x_accel_valid());
    value.bitfield0 = esf_ins::BF0_X_ACCEL_VALID;
    EXPECT_TRUE(value.x_accel_valid());
    EXPECT_FALSE(value.y_accel_valid());
    value.bitfield0 = esf_ins::BF0_Y_ACCEL_VALID;
    EXPECT_TRUE(value.y_accel_valid());
    EXPECT_FALSE(value.z_accel_valid());
    value.bitfield0 = esf_ins::BF0_Z_ACCEL_VALID;
    EXPECT_TRUE(value.z_accel_valid());
    EXPECT_FALSE(value.x_ang_rate_valid());
    EXPECT_EQ(value.version(), 0U);
}

TEST(UbxMessageEsfIns, ScaledHelpersUseSpecScales)
{
    esf_ins value{};
    value.x_ang_rate = 1;
    value.y_ang_rate = -1000;
    value.z_ang_rate = 0;
    value.x_accel = 1;
    value.y_accel = -100;
    value.z_accel = 0;
    EXPECT_DOUBLE_EQ(value.x_ang_rate_deg_s(), 0.001);
    EXPECT_DOUBLE_EQ(value.y_ang_rate_deg_s(), -1.0);
    EXPECT_DOUBLE_EQ(value.z_ang_rate_deg_s(), 0.0);
    EXPECT_DOUBLE_EQ(value.x_accel_m_s2(), 0.01);
    EXPECT_DOUBLE_EQ(value.y_accel_m_s2(), -1.0);
    EXPECT_DOUBLE_EQ(value.z_accel_m_s2(), 0.0);
}

TEST(UbxMessageEsfIns, RejectsWrongClassIdAndLength)
{
    uint8_t payload[37U] = {};
    payload[8U] = 0x42U;
    esf_ins out = sample_dynamics();

    EXPECT_FALSE(esf_ins::matches(make_view(UBX_CLASS_NAV, UBX_ID_ESF_INS, payload, 36U)));
    EXPECT_FALSE(esf_ins::decode(make_view(UBX_CLASS_NAV, UBX_ID_ESF_INS, payload, 36U), out));
    EXPECT_EQ(out.i_tow, 0U);
    out = sample_dynamics();
    EXPECT_FALSE(esf_ins::matches(make_view(UBX_CLASS_ESF, UBX_ID_ESF_ALG, payload, 36U)));
    EXPECT_FALSE(esf_ins::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_ALG, payload, 36U), out));
    EXPECT_EQ(out.bitfield0, 0U);
    out = sample_dynamics();
    EXPECT_FALSE(esf_ins::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_INS, payload, 35U), out));
    EXPECT_EQ(out.z_accel, 0);
    EXPECT_FALSE(esf_ins::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_INS, payload, 37U), out));
    ASSERT_TRUE(esf_ins::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_INS, payload, 36U), out));
    EXPECT_EQ(out.i_tow, 0x42U);
}

TEST(UbxMessageEsfIns, EncodeIntoTooSmallBufferReportsFull)
{
    uint8_t frame[frame_size(36U)] = {};
    castle::size_type written = 3U;
    EXPECT_EQ(encode_message(sample_dynamics(), frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(sample_dynamics(), frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
}

} // namespace
