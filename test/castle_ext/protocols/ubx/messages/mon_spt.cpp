#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_spt.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using spt = mon_spt<4U, 8U>;

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

TEST(UbxMessageMonSpt, ConstantsMatchSpec)
{
    EXPECT_EQ(spt::msg_class, UBX_CLASS_MON);
    EXPECT_EQ(spt::msg_id, UBX_ID_MON_SPT);
    EXPECT_EQ(spt::header_length, 4U);
    EXPECT_EQ(spt::sensor_block_length, 4U);
    EXPECT_EQ(spt::result_block_length, 12U);
    EXPECT_EQ((mon_spt<>().sensors.capacity()), 4U);
    EXPECT_EQ((mon_spt<>().results.capacity()), 16U);
    EXPECT_EQ(mon_spt_sensor::DRV_VER_MAJ, 0x0FU);
    EXPECT_EQ(mon_spt_sensor::DRV_VER_MIN, 0xF0U);
}

TEST(UbxMessageMonSpt, EnumValuesMatchSpec)
{
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::st_lsm6ds0), 1U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::invensense_mpu6500), 2U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::bosch_bmi160), 3U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::st_lsm6ds3), 7U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::bosch_smi130), 9U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::invensense_mpu6515), 12U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::st_lsm6dsl), 13U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::bosch_smg130), 14U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::bosch_smi230), 15U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::bosch_bmi260), 16U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::st_icm330dlc), 17U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_sensor_id::st_icm330dhcx), 18U);

    EXPECT_EQ(static_cast<uint8_t>(mon_spt_test_state::not_started), 0U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_test_state::running), 1U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_test_state::error), 2U);
    EXPECT_EQ(static_cast<uint8_t>(mon_spt_test_state::finished), 3U);

    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::gyro_z), 5U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::gyro_temperature), 12U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::gyro_y), 13U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::gyro_x), 14U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::accel_x), 16U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::accel_y), 17U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::accel_z), 18U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::barometer), 19U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::magnetometer_x), 22U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::magnetometer_y), 23U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::magnetometer_z), 24U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_sensor_type::barometer_temperature), 25U);

    EXPECT_EQ(static_cast<uint16_t>(mon_spt_res_type::no_offset), 1U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_res_type::positive_offset), 2U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_res_type::negative_offset), 3U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_res_type::min_off_to_pos), 4U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_res_type::max_off_to_pos), 5U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_res_type::min_neg_to_pos), 6U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_res_type::max_neg_to_pos), 7U);
    EXPECT_EQ(static_cast<uint16_t>(mon_spt_res_type::self_test_passed), 8U);
}

TEST(UbxMessageMonSpt, RoundTripPreservesEveryField)
{
    spt message{};
    message.version = 0x01U;
    message.num_sensor = 50U; // ignored on encode
    message.num_res = 60U;    // ignored on encode
    for (uint8_t i = 0U; i < 2U; ++i)
    {
        mon_spt_sensor sensor{};
        sensor.sensor_id = static_cast<mon_spt_sensor_id>(0x10U + i);
        sensor.drv_ver = static_cast<uint8_t>(0x21U + i);
        sensor.test_state = static_cast<mon_spt_test_state>(i + 1U);
        sensor.drv_file_name = static_cast<uint8_t>('a' + i);
        ASSERT_EQ(message.sensors.push_back(sensor), castle::status::ok);
    }
    for (uint16_t i = 0U; i < 3U; ++i)
    {
        mon_spt_result result{};
        result.sensor_id_res = static_cast<uint16_t>(0x0301U + i);
        result.sensor_type = static_cast<mon_spt_sensor_type>(0x0402U + i);
        result.res_type = static_cast<mon_spt_res_type>(0x0503U + i);
        result.value = -0x06070809 + static_cast<int32_t>(i);
        ASSERT_EQ(message.results.push_back(result), castle::status::ok);
    }

    uint8_t frame[UBX_FRAME_OVERHEAD + 4U + 8U + 36U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 1U], 2U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 2U], 3U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    spt out{};
    ASSERT_TRUE(spt::decode(raw, out));
    EXPECT_EQ(out.version, 1U);
    EXPECT_EQ(out.num_sensor, 2U);
    EXPECT_EQ(out.num_res, 3U);
    ASSERT_EQ(out.sensors.size(), 2U);
    ASSERT_EQ(out.results.size(), 3U);
    for (castle::size_type i = 0U; i < 2U; ++i)
    {
        EXPECT_EQ(out.sensors[i].sensor_id, message.sensors[i].sensor_id);
        EXPECT_EQ(out.sensors[i].drv_ver, message.sensors[i].drv_ver);
        EXPECT_EQ(out.sensors[i].test_state, message.sensors[i].test_state);
        EXPECT_EQ(out.sensors[i].drv_file_name, message.sensors[i].drv_file_name);
    }
    for (castle::size_type i = 0U; i < 3U; ++i)
    {
        EXPECT_EQ(out.results[i].sensor_id_res, message.results[i].sensor_id_res);
        EXPECT_EQ(out.results[i].sensor_type, message.results[i].sensor_type);
        EXPECT_EQ(out.results[i].res_type, message.results[i].res_type);
        EXPECT_EQ(out.results[i].value, message.results[i].value);
    }
}

TEST(UbxMessageMonSpt, HandBuiltPayloadMatchesSpecOffsets)
{
    // numSensor = 2, numRes = 2: 4 + 8 + 24 = 36 bytes.
    uint8_t payload[36U] = {};
    payload[0U] = 0x01U;
    payload[1U] = 2U;
    payload[2U] = 2U;
    payload[3U] = 0xEEU; // reserved0 ignored
    // sensors at 4 + n*4
    payload[4U] = 3U;
    payload[5U] = 0x52U; // min 5, maj 2
    payload[6U] = 3U;
    payload[7U] = 0U;
    payload[8U] = 13U;
    payload[9U] = 0xA1U;
    payload[10U] = 2U;
    payload[11U] = static_cast<uint8_t>('7');
    // results at 4 + numSensor*4 + n*12 = 12 + n*12
    castle::write_le16(&payload[12U], 3U);
    castle::write_le16(&payload[14U], 14U);
    castle::write_le16(&payload[16U], 2U);
    payload[18U] = 0xEEU; // reserved1 ignored
    payload[19U] = 0xEEU;
    castle::write_le32s(&payload[20U], -123456);
    castle::write_le16(&payload[24U], 13U);
    castle::write_le16(&payload[26U], 25U);
    castle::write_le16(&payload[28U], 8U);
    castle::write_le32(&payload[32U], 1U);

    spt out{};
    ASSERT_TRUE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 36U), out));
    EXPECT_EQ(out.version, 1U);
    EXPECT_EQ(out.num_sensor, 2U);
    EXPECT_EQ(out.num_res, 2U);
    ASSERT_EQ(out.sensors.size(), 2U);
    EXPECT_EQ(out.sensors[0U].sensor_id, mon_spt_sensor_id::bosch_bmi160);
    EXPECT_EQ(out.sensors[0U].drv_ver, 0x52U);
    EXPECT_EQ(out.sensors[0U].drv_ver_maj(), 2U);
    EXPECT_EQ(out.sensors[0U].drv_ver_min(), 5U);
    EXPECT_EQ(out.sensors[0U].test_state, mon_spt_test_state::finished);
    EXPECT_EQ(out.sensors[0U].drv_file_name, 0U);
    EXPECT_EQ(out.sensors[1U].sensor_id, mon_spt_sensor_id::st_lsm6dsl);
    EXPECT_EQ(out.sensors[1U].drv_ver_maj(), 1U);
    EXPECT_EQ(out.sensors[1U].drv_ver_min(), 0xAU);
    EXPECT_EQ(out.sensors[1U].test_state, mon_spt_test_state::error);
    EXPECT_EQ(out.sensors[1U].drv_file_name, '7');
    ASSERT_EQ(out.results.size(), 2U);
    EXPECT_EQ(out.results[0U].sensor_id_res, 3U);
    EXPECT_EQ(out.results[0U].sensor_type, mon_spt_sensor_type::gyro_x);
    EXPECT_EQ(out.results[0U].res_type, mon_spt_res_type::positive_offset);
    EXPECT_EQ(out.results[0U].value, -123456);
    EXPECT_EQ(out.results[1U].sensor_id_res, 13U);
    EXPECT_EQ(out.results[1U].sensor_type, mon_spt_sensor_type::barometer_temperature);
    EXPECT_EQ(out.results[1U].res_type, mon_spt_res_type::self_test_passed);
    EXPECT_EQ(out.results[1U].value, 1);

    // Encoding writes zeros into reserved0 / reserved1.
    uint8_t frame[UBX_FRAME_OVERHEAD + 36U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(out, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 3U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 18U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 19U], 0U);
    EXPECT_EQ(castle::read_le32s(&frame[UBX_FRAME_INDEX_PAYLOAD + 20U]), -123456);
}

TEST(UbxMessageMonSpt, DriverVersionNibbles)
{
    mon_spt_sensor sensor{};
    EXPECT_EQ(sensor.drv_ver_maj(), 0U);
    EXPECT_EQ(sensor.drv_ver_min(), 0U);
    sensor.drv_ver = 0x0FU;
    EXPECT_EQ(sensor.drv_ver_maj(), 0xFU);
    EXPECT_EQ(sensor.drv_ver_min(), 0U);
    sensor.drv_ver = 0xF0U;
    EXPECT_EQ(sensor.drv_ver_maj(), 0U);
    EXPECT_EQ(sensor.drv_ver_min(), 0xFU);
    sensor.drv_ver = 0x9CU;
    EXPECT_EQ(sensor.drv_ver_maj(), 0xCU);
    EXPECT_EQ(sensor.drv_ver_min(), 0x9U);
}

TEST(UbxMessageMonSpt, EmptyGroupsAreValid)
{
    uint8_t payload[16U] = {0x01U, 0U, 0U, 0U};
    spt out{};
    ASSERT_TRUE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 4U), out));
    EXPECT_TRUE(out.sensors.empty());
    EXPECT_TRUE(out.results.empty());

    // Sensors only.
    payload[1U] = 1U;
    payload[4U] = 2U;
    ASSERT_TRUE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 8U), out));
    EXPECT_EQ(out.sensors.size(), 1U);
    EXPECT_TRUE(out.results.empty());

    // Results only: result block starts directly at offset 4.
    payload[1U] = 0U;
    payload[2U] = 1U;
    payload[4U] = 9U;
    ASSERT_TRUE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 16U), out));
    EXPECT_TRUE(out.sensors.empty());
    ASSERT_EQ(out.results.size(), 1U);
    EXPECT_EQ(out.results[0U].sensor_id_res, 9U);
}

TEST(UbxMessageMonSpt, RejectsWrongClassOrId)
{
    uint8_t payload[4U] = {};
    spt out{};
    EXPECT_FALSE(spt::matches(make_view(UBX_CLASS_ESF, UBX_ID_MON_SPT, payload, 4U)));
    EXPECT_FALSE(spt::decode(make_view(UBX_CLASS_ESF, UBX_ID_MON_SPT, payload, 4U), out));
    EXPECT_FALSE(spt::matches(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 4U)));
    EXPECT_FALSE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_PATCH, payload, 4U), out));
    EXPECT_TRUE(spt::matches(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 4U)));
}

TEST(UbxMessageMonSpt, RejectsLengthMismatchAgainstBothCounts)
{
    uint8_t payload[64U] = {};
    payload[0U] = 1U;
    payload[1U] = 2U;
    payload[2U] = 1U; // expects 4 + 8 + 12 = 24
    spt out{};
    EXPECT_FALSE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 3U), out));
    EXPECT_FALSE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 4U), out));
    EXPECT_FALSE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 12U), out));
    EXPECT_FALSE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 23U), out));
    EXPECT_FALSE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 25U), out));
    EXPECT_FALSE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 36U), out));
    // Both counts contribute: 3 sensors + 1 result = 28 bytes is valid, 3 sensors + 2 results is not.
    payload[1U] = 3U;
    payload[2U] = 1U;
    EXPECT_TRUE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 28U), out));
    payload[1U] = 3U;
    payload[2U] = 2U;
    EXPECT_FALSE(spt::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 28U), out));
    EXPECT_EQ(out.version, 0U);
    EXPECT_EQ(out.num_sensor, 0U);
    EXPECT_EQ(out.num_res, 0U);
    EXPECT_TRUE(out.sensors.empty());
    EXPECT_TRUE(out.results.empty());
}

TEST(UbxMessageMonSpt, RejectsDeclaredLengthLongerThanView)
{
    uint8_t payload[8U] = {1U, 1U, 1U, 0U};
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_SPT, 20U}, array_view<CASTLE_CONST uint8_t>(payload, 8U), checksum{}};
    spt out{};
    EXPECT_FALSE(spt::decode(raw, out));
    EXPECT_EQ(out.num_sensor, 0U);
    EXPECT_TRUE(out.sensors.empty());
}

TEST(UbxMessageMonSpt, CapacityOverflowOnEitherGroupKeepsCursorAligned)
{
    // 3 sensors, 3 results: 4 + 12 + 36 = 52.
    uint8_t payload[52U] = {};
    payload[1U] = 3U;
    payload[2U] = 3U;
    payload[4U] = 1U;
    payload[8U] = 2U;
    payload[12U] = 3U;
    castle::write_le32s(&payload[16U + 8U], 100);
    castle::write_le32s(&payload[28U + 8U], 200);
    castle::write_le32s(&payload[40U + 8U], 300);

    mon_spt<1U, 2U> out{};
    ASSERT_TRUE((mon_spt<1U, 2U>::decode(make_view(UBX_CLASS_MON, UBX_ID_MON_SPT, payload, 52U), out)));
    EXPECT_EQ(out.num_sensor, 3U);
    EXPECT_EQ(out.num_res, 3U);
    ASSERT_EQ(out.sensors.size(), 1U);
    EXPECT_EQ(static_cast<uint8_t>(out.sensors[0U].sensor_id), 1U);
    // Results start after all three wire sensor blocks, not after the one stored.
    ASSERT_EQ(out.results.size(), 2U);
    EXPECT_EQ(out.results[0U].value, 100);
    EXPECT_EQ(out.results[1U].value, 200);
}

TEST(UbxMessageMonSpt, EncodeIntoTooSmallBufferReportsFull)
{
    spt message{};
    ASSERT_EQ(message.sensors.push_back(mon_spt_sensor{}), castle::status::ok);
    ASSERT_EQ(message.results.push_back(mon_spt_result{}), castle::status::ok);
    uint8_t frame[UBX_FRAME_OVERHEAD + 20U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
