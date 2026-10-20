#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/esf_status.hpp"
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

esf_status_sensor sensor(uint8_t status1, uint8_t status2, uint8_t freq, uint8_t faults)
{
    esf_status_sensor result{};
    result.sens_status1 = status1;
    result.sens_status2 = status2;
    result.freq = freq;
    result.faults = faults;
    return result;
}

TEST(UbxMessageEsfStatus, MasksMatchSpecBitPositions)
{
    // Old masks were TYPE 0x1F / USED 0x20 / READY 0x40; the spec has type bits 5..0, used bit 6, ready bit 7.
    EXPECT_EQ(esf_status_sensor::SENS_STATUS1_TYPE, 0x3FU);
    EXPECT_EQ(esf_status_sensor::SENS_STATUS1_USED, 0x40U);
    EXPECT_EQ(esf_status_sensor::SENS_STATUS1_READY, 0x80U);
    EXPECT_EQ(esf_status_sensor::SENS_STATUS2_CALIB_STATUS, 0x03U);
    EXPECT_EQ(esf_status_sensor::SENS_STATUS2_TIME_STATUS, 0x0CU);
    EXPECT_EQ(esf_status_sensor::FAULTS_BAD_MEAS, 0x01U);
    EXPECT_EQ(esf_status_sensor::FAULTS_BAD_TTAG, 0x02U);
    EXPECT_EQ(esf_status_sensor::FAULTS_MISSING_MEAS, 0x04U);
    EXPECT_EQ(esf_status_sensor::FAULTS_NOISY_MEAS, 0x08U);
    EXPECT_EQ(esf_status<>::msg_class, 0x10U);
    EXPECT_EQ(esf_status<>::msg_id, 0x10U);
    EXPECT_EQ(esf_status<>::header_length, 16U);
    EXPECT_EQ(esf_status<>::block_length, 4U);
}

TEST(UbxMessageEsfStatus, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[24U] = {};
    castle::write_le32(&payload[0U], 100U);
    payload[4U] = 2U;
    for (castle::size_type index = 5U; index < 12U; ++index)
    {
        payload[index] = 0xAAU; // reserved0
    }
    payload[12U] = 2U; // fusionMode: suspended
    payload[13U] = 0xBBU;
    payload[14U] = 0xBBU;
    payload[15U] = 2U; // numSens
    payload[16U] = 0x61U; // corrected: type 33 (bits 5..0), used (bit 6), not ready
    payload[17U] = 0x0EU; // calibStatus 2, timeStatus 3
    payload[18U] = 10U;
    payload[19U] = 11U;   // badMeas, badTTag, noisyMeas
    payload[20U] = 0x8BU; // type 11, ready (bit 7), not used
    payload[21U] = 0x05U; // calibStatus 1, timeStatus 1
    payload[22U] = 200U;
    payload[23U] = 0x04U; // missingMeas

    message_view raw = make_view(UBX_CLASS_ESF, UBX_ID_ESF_STATUS, payload, 24U);
    esf_status<> out;
    ASSERT_TRUE(esf_status<>::matches(raw));
    ASSERT_TRUE(esf_status<>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 100U);
    EXPECT_EQ(out.version, 2U);
    EXPECT_EQ(out.fusion_mode, esf_fusion_mode::suspended);
    EXPECT_EQ(out.num_sens, 2U);
    ASSERT_EQ(out.sensors.size(), 2U);

    CASTLE_CONST esf_status_sensor& first = out.sensors[0U];
    EXPECT_EQ(first.sens_status1, 0x61U);
    EXPECT_EQ(first.type(), 33U);
    EXPECT_EQ(first.sens_status1 & esf_status_sensor::SENS_STATUS1_TYPE, 33);
    EXPECT_TRUE(first.used());
    EXPECT_FALSE(first.ready());
    EXPECT_EQ(first.calib_status(), esf_calib_status::calibrated);
    EXPECT_TRUE(first.calibrated());
    EXPECT_EQ(first.time_status(), esf_time_status::time_tag);
    EXPECT_EQ(first.freq, 10U);
    EXPECT_TRUE(first.bad_meas());
    EXPECT_TRUE(first.bad_ttag());
    EXPECT_FALSE(first.missing_meas());
    EXPECT_TRUE(first.noisy_meas());

    CASTLE_CONST esf_status_sensor& second = out.sensors[1U];
    EXPECT_EQ(second.type(), 11U);
    EXPECT_FALSE(second.used());
    EXPECT_TRUE(second.ready());
    EXPECT_EQ(second.calib_status(), esf_calib_status::calibrating);
    EXPECT_FALSE(second.calibrated());
    EXPECT_EQ(second.time_status(), esf_time_status::first_byte);
    EXPECT_EQ(second.freq, 200U);
    EXPECT_FALSE(second.bad_meas());
    EXPECT_FALSE(second.bad_ttag());
    EXPECT_TRUE(second.missing_meas());
    EXPECT_FALSE(second.noisy_meas());
}

TEST(UbxMessageEsfStatus, SensorAccessorsCoverEveryEnumValue)
{
    esf_status_sensor value{};
    EXPECT_EQ(value.calib_status(), esf_calib_status::not_calibrated);
    EXPECT_FALSE(value.calibrated());
    EXPECT_EQ(value.time_status(), esf_time_status::no_data);
    value.sens_status2 = 0x03U;
    EXPECT_EQ(value.calib_status(), esf_calib_status::calibrated_alt);
    EXPECT_TRUE(value.calibrated());
    EXPECT_EQ(value.time_status(), esf_time_status::no_data);
    value.sens_status2 = 0x08U;
    EXPECT_EQ(value.time_status(), esf_time_status::event_input);
    EXPECT_EQ(value.calib_status(), esf_calib_status::not_calibrated);
    value.sens_status2 = 0xF0U; // bits 7..4 are not part of either field
    EXPECT_EQ(value.time_status(), esf_time_status::no_data);
    EXPECT_EQ(value.calib_status(), esf_calib_status::not_calibrated);

    value.sens_status1 = 0xC0U;
    EXPECT_EQ(value.type(), 0U);
    EXPECT_TRUE(value.used());
    EXPECT_TRUE(value.ready());
    value.sens_status1 = 0x3FU;
    EXPECT_EQ(value.type(), 63U);
    EXPECT_FALSE(value.used());
    EXPECT_FALSE(value.ready());

    value.faults = 0xF0U;
    EXPECT_FALSE(value.bad_meas());
    EXPECT_FALSE(value.bad_ttag());
    EXPECT_FALSE(value.missing_meas());
    EXPECT_FALSE(value.noisy_meas());
}

TEST(UbxMessageEsfStatus, RoundTripsEveryFieldWithCountFromSize)
{
    esf_status<4U> in{};
    in.i_tow = 0x01020304U;
    in.version = 0x02U;
    in.fusion_mode = esf_fusion_mode::disabled;
    in.num_sens = 9U; // stale: encode writes sensors.size()
    ASSERT_EQ(in.sensors.push_back(sensor(0x45U, 0x09U, 50U, 0x01U)), castle::status::ok);
    ASSERT_EQ(in.sensors.push_back(sensor(0x8AU, 0x06U, 25U, 0x0AU)), castle::status::ok);
    ASSERT_EQ(in.sensors.push_back(sensor(0xD1U, 0x0FU, 1U, 0x0FU)), castle::status::ok);

    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(16U + (3U * 4U)));
    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(payload[12U], 3U);
    EXPECT_EQ(payload[15U], 3U);
    for (castle::size_type index = 5U; index < 12U; ++index)
    {
        EXPECT_EQ(payload[index], 0U);
    }
    EXPECT_EQ(payload[13U], 0U);
    EXPECT_EQ(payload[14U], 0U);

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    esf_status<4U> out;
    ASSERT_TRUE(esf_status<4U>::decode(raw, out));
    EXPECT_EQ(out.i_tow, in.i_tow);
    EXPECT_EQ(out.version, in.version);
    EXPECT_EQ(out.fusion_mode, esf_fusion_mode::disabled);
    EXPECT_EQ(out.num_sens, 3U);
    ASSERT_EQ(out.sensors.size(), 3U);
    for (castle::size_type index = 0U; index < 3U; ++index)
    {
        EXPECT_EQ(out.sensors[index].sens_status1, in.sensors[index].sens_status1);
        EXPECT_EQ(out.sensors[index].sens_status2, in.sensors[index].sens_status2);
        EXPECT_EQ(out.sensors[index].freq, in.sensors[index].freq);
        EXPECT_EQ(out.sensors[index].faults, in.sensors[index].faults);
    }
}

TEST(UbxMessageEsfStatus, FusionModeCoversEverySpecValue)
{
    uint8_t payload[16U] = {};
    esf_status<> out;
    CASTLE_CONST esf_fusion_mode modes[] = {esf_fusion_mode::initialization, esf_fusion_mode::fusion,
                                            esf_fusion_mode::suspended, esf_fusion_mode::disabled};
    for (CASTLE_CONST esf_fusion_mode mode : modes)
    {
        payload[12U] = static_cast<uint8_t>(mode);
        ASSERT_TRUE(esf_status<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_STATUS, payload, 16U), out));
        EXPECT_EQ(out.fusion_mode, mode);
        EXPECT_TRUE(out.sensors.empty());
    }
}

TEST(UbxMessageEsfStatus, RejectsPayloadNotMatchingDeclaredSensorCount)
{
    uint8_t payload[28U] = {};
    payload[0U] = 1U;
    payload[15U] = 2U;
    esf_status<> out;
    out.i_tow = 9U;
    EXPECT_FALSE(esf_status<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_STATUS, payload, 16U), out));
    EXPECT_EQ(out.i_tow, 0U);
    EXPECT_EQ(out.num_sens, 0U);
    EXPECT_FALSE(esf_status<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_STATUS, payload, 20U), out));
    EXPECT_FALSE(esf_status<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_STATUS, payload, 23U), out));
    EXPECT_FALSE(esf_status<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_STATUS, payload, 25U), out));
    EXPECT_FALSE(esf_status<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_STATUS, payload, 28U), out)); // longer
    EXPECT_FALSE(esf_status<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_STATUS, payload, 15U), out));
    EXPECT_FALSE(esf_status<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_STATUS, payload, 0U), out));
    EXPECT_TRUE(esf_status<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_STATUS, payload, 24U), out));
    EXPECT_EQ(out.i_tow, 1U);
}

TEST(UbxMessageEsfStatus, ClampsSensorStorageAndSkipsTheRest)
{
    uint8_t payload[24U] = {};
    payload[15U] = 2U;
    payload[16U] = 1U;
    payload[20U] = 2U;
    esf_status<1U> out;
    ASSERT_TRUE(esf_status<1U>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_STATUS, payload, 24U), out));
    EXPECT_EQ(out.num_sens, 2U);
    ASSERT_EQ(out.sensors.size(), 1U);
    EXPECT_EQ(out.sensors[0U].sens_status1, 1U);
    EXPECT_EQ(out.sensors[0U].type(), 1U);
}

TEST(UbxMessageEsfStatus, RejectsWrongClassAndId)
{
    uint8_t payload[16U] = {};
    esf_status<> out;
    EXPECT_FALSE(esf_status<>::matches(make_view(UBX_CLASS_NAV, UBX_ID_ESF_STATUS, payload, 16U)));
    EXPECT_FALSE(esf_status<>::decode(make_view(UBX_CLASS_NAV, UBX_ID_ESF_STATUS, payload, 16U), out));
    EXPECT_FALSE(esf_status<>::matches(make_view(UBX_CLASS_ESF, UBX_ID_ESF_INS, payload, 16U)));
    EXPECT_FALSE(esf_status<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_INS, payload, 16U), out));
}

TEST(UbxMessageEsfStatus, EncodeIntoTooSmallBufferReportsFull)
{
    esf_status<2U> in{};
    ASSERT_EQ(in.sensors.push_back(sensor(1U, 2U, 3U, 4U)), castle::status::ok);
    uint8_t frame[frame_size(20U)] = {};
    castle::size_type written = 1U;
    EXPECT_EQ(encode_message(in, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
}

} // namespace
