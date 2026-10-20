#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/esf_alg.hpp"
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

esf_alg sample_alignment()
{
    esf_alg value{};
    value.i_tow = 0x11223344U;
    value.version = 0x01U;
    value.flags = 0x07U; // autoMntAlgOn + status 3 (coarse)
    value.error = 0x05U;
    value.yaw = 35999U;
    value.pitch = -8999;
    value.roll = 17999;
    return value;
}

TEST(UbxMessageEsfAlg, ConstantsMatchSpec)
{
    EXPECT_EQ(esf_alg::msg_class, 0x10U);
    EXPECT_EQ(esf_alg::msg_id, 0x14U);
    EXPECT_EQ(esf_alg::payload_length, 16U);
    EXPECT_EQ(esf_alg::FLAGS_AUTO_MNT_ALG_ON, 0x01U);
    EXPECT_EQ(esf_alg::FLAGS_STATUS, 0x0EU);
    EXPECT_EQ(esf_alg::ERROR_TILT_ALG_ERROR, 0x01U);
    EXPECT_EQ(esf_alg::ERROR_YAW_ALG_ERROR, 0x02U);
    EXPECT_EQ(esf_alg::ERROR_ANGLE_ERROR, 0x04U);
}

TEST(UbxMessageEsfAlg, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[16U] = {};
    castle::write_le32(&payload[0U], 0xA1B2C3D4U);
    payload[4U] = 0x01U;
    payload[5U] = 0x09U; // autoMntAlgOn + status 4 (fine)
    payload[6U] = 0x06U; // yawAlgError + angleError
    payload[7U] = 0xEEU; // reserved0 is ignored
    castle::write_le32(&payload[8U], 27000U);
    castle::write_le16(&payload[12U], static_cast<uint16_t>(-4500));
    castle::write_le16(&payload[14U], static_cast<uint16_t>(-18000));

    esf_alg out;
    message_view raw = make_view(UBX_CLASS_ESF, UBX_ID_ESF_ALG, payload, 16U);
    ASSERT_TRUE(esf_alg::matches(raw));
    ASSERT_TRUE(esf_alg::decode(raw, out));
    EXPECT_EQ(out.i_tow, 0xA1B2C3D4U);
    EXPECT_EQ(out.version, 0x01U);
    EXPECT_EQ(out.flags, 0x09U);
    EXPECT_EQ(out.error, 0x06U);
    EXPECT_EQ(out.yaw, 27000U);
    EXPECT_EQ(out.pitch, -4500);
    EXPECT_EQ(out.roll, -18000);
    EXPECT_TRUE(out.auto_mnt_alg_on());
    EXPECT_EQ(out.status(), esf_alg_status::fine);
    EXPECT_FALSE(out.tilt_alg_error());
    EXPECT_TRUE(out.yaw_alg_error());
    EXPECT_TRUE(out.angle_error());
    EXPECT_DOUBLE_EQ(out.yaw_deg(), 270.0);
    EXPECT_DOUBLE_EQ(out.pitch_deg(), -45.0);
    EXPECT_DOUBLE_EQ(out.roll_deg(), -180.0);
}

TEST(UbxMessageEsfAlg, RoundTripsEveryFieldAndZeroesReserved)
{
    CASTLE_CONST esf_alg in = sample_alignment();
    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(16U));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 7U], 0U);

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    esf_alg out;
    ASSERT_TRUE(esf_alg::decode(raw, out));
    EXPECT_EQ(out.i_tow, in.i_tow);
    EXPECT_EQ(out.version, in.version);
    EXPECT_EQ(out.flags, in.flags);
    EXPECT_EQ(out.error, in.error);
    EXPECT_EQ(out.yaw, in.yaw);
    EXPECT_EQ(out.pitch, in.pitch);
    EXPECT_EQ(out.roll, in.roll);
    EXPECT_EQ(out.status(), esf_alg_status::coarse);
}

TEST(UbxMessageEsfAlg, StatusDecodesEverySpecValueIndependentlyOfAutoBit)
{
    esf_alg value{};
    CASTLE_CONST esf_alg_status statuses[] = {esf_alg_status::user_defined, esf_alg_status::roll_pitch_ongoing,
                                              esf_alg_status::roll_pitch_yaw_ongoing, esf_alg_status::coarse,
                                              esf_alg_status::fine};
    for (CASTLE_CONST esf_alg_status status : statuses)
    {
        value.flags = static_cast<uint8_t>(static_cast<uint8_t>(status) << 1U);
        EXPECT_EQ(value.status(), status);
        EXPECT_FALSE(value.auto_mnt_alg_on());
        value.flags = static_cast<uint8_t>(value.flags | esf_alg::FLAGS_AUTO_MNT_ALG_ON);
        EXPECT_EQ(value.status(), status);
        EXPECT_TRUE(value.auto_mnt_alg_on());
    }
    value.flags = 0xF0U; // bits above status do not leak into it
    EXPECT_EQ(static_cast<uint8_t>(value.status()), 0U);
    EXPECT_FALSE(value.auto_mnt_alg_on());
}

TEST(UbxMessageEsfAlg, ErrorBitsSetAndClearIndividually)
{
    esf_alg value{};
    EXPECT_FALSE(value.tilt_alg_error());
    EXPECT_FALSE(value.yaw_alg_error());
    EXPECT_FALSE(value.angle_error());
    value.error = esf_alg::ERROR_TILT_ALG_ERROR;
    EXPECT_TRUE(value.tilt_alg_error());
    EXPECT_FALSE(value.yaw_alg_error());
    EXPECT_FALSE(value.angle_error());
    value.error = esf_alg::ERROR_YAW_ALG_ERROR;
    EXPECT_FALSE(value.tilt_alg_error());
    EXPECT_TRUE(value.yaw_alg_error());
    EXPECT_FALSE(value.angle_error());
    value.error = esf_alg::ERROR_ANGLE_ERROR;
    EXPECT_FALSE(value.tilt_alg_error());
    EXPECT_FALSE(value.yaw_alg_error());
    EXPECT_TRUE(value.angle_error());
    value.error = 0xF8U;
    EXPECT_FALSE(value.tilt_alg_error());
    EXPECT_FALSE(value.yaw_alg_error());
    EXPECT_FALSE(value.angle_error());
}

TEST(UbxMessageEsfAlg, ScaledHelpersApplyHundredthsOfDegree)
{
    esf_alg value{};
    value.yaw = 36000U;
    value.pitch = 9000;
    value.roll = -1;
    EXPECT_DOUBLE_EQ(value.yaw_deg(), 360.0);
    EXPECT_DOUBLE_EQ(value.pitch_deg(), 90.0);
    EXPECT_DOUBLE_EQ(value.roll_deg(), -0.01);
    value.yaw = 0U;
    EXPECT_DOUBLE_EQ(value.yaw_deg(), 0.0);
}

TEST(UbxMessageEsfAlg, RejectsWrongClassIdAndLength)
{
    uint8_t payload[17U] = {};
    payload[0U] = 0x55U;
    esf_alg out = sample_alignment();

    EXPECT_FALSE(esf_alg::matches(make_view(UBX_CLASS_NAV, UBX_ID_ESF_ALG, payload, 16U)));
    EXPECT_FALSE(esf_alg::decode(make_view(UBX_CLASS_NAV, UBX_ID_ESF_ALG, payload, 16U), out));
    EXPECT_EQ(out.i_tow, 0U);

    out = sample_alignment();
    EXPECT_FALSE(esf_alg::matches(make_view(UBX_CLASS_ESF, UBX_ID_ESF_INS, payload, 16U)));
    EXPECT_FALSE(esf_alg::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_INS, payload, 16U), out));
    EXPECT_EQ(out.yaw, 0U);

    out = sample_alignment();
    EXPECT_FALSE(esf_alg::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_ALG, payload, 15U), out));
    EXPECT_EQ(out.flags, 0U);
    EXPECT_FALSE(esf_alg::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_ALG, payload, 17U), out));
    EXPECT_FALSE(esf_alg::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_ALG, payload, 0U), out));
    EXPECT_TRUE(esf_alg::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_ALG, payload, 16U), out));
    EXPECT_EQ(out.i_tow, 0x55U);
}

TEST(UbxMessageEsfAlg, EncodeIntoTooSmallBufferReportsFull)
{
    uint8_t frame[frame_size(16U)] = {};
    castle::size_type written = 7U;
    EXPECT_EQ(encode_message(sample_alignment(), frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(sample_alignment(), frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
}

} // namespace
