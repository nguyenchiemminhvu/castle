#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_spt.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // One IMU (driver v2.3) whose self-test finished, with two result items.
    mon_spt<2U, 4U> report{};
    report.version = 1U;
    mon_spt_sensor imu{};
    imu.sensor_id = mon_spt_sensor_id::bosch_bmi160;
    imu.drv_ver = static_cast<uint8_t>((3U << mon_spt_sensor::DRV_VER_MIN_SHIFT) | 2U);
    imu.test_state = mon_spt_test_state::finished;
    imu.drv_file_name = 0U;
    CASTLE_SAMPLE_CHECK(report.sensors.push_back(imu) == castle::status::ok);

    mon_spt_result gyro_x{};
    gyro_x.sensor_id_res = static_cast<uint16_t>(mon_spt_sensor_id::bosch_bmi160);
    gyro_x.sensor_type = mon_spt_sensor_type::gyro_x;
    gyro_x.res_type = mon_spt_res_type::positive_offset;
    gyro_x.value = -1234;
    CASTLE_SAMPLE_CHECK(report.results.push_back(gyro_x) == castle::status::ok);
    mon_spt_result verdict{};
    verdict.sensor_id_res = static_cast<uint16_t>(mon_spt_sensor_id::bosch_bmi160);
    verdict.sensor_type = mon_spt_sensor_type::accel_z;
    verdict.res_type = mon_spt_res_type::self_test_passed;
    verdict.value = 1;
    CASTLE_SAMPLE_CHECK(report.results.push_back(verdict) == castle::status::ok);

    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(report, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(4U + (1U * 4U) + (2U * 12U)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK((mon_spt<2U, 4U>::matches(raw)));

    mon_spt<2U, 4U> decoded{};
    CASTLE_SAMPLE_CHECK((mon_spt<2U, 4U>::decode(raw, decoded)));
    CASTLE_SAMPLE_CHECK(decoded.version == 1U);
    CASTLE_SAMPLE_CHECK(decoded.num_sensor == 1U);
    CASTLE_SAMPLE_CHECK(decoded.num_res == 2U);
    CASTLE_SAMPLE_CHECK(decoded.sensors[0U].sensor_id == mon_spt_sensor_id::bosch_bmi160);
    CASTLE_SAMPLE_CHECK(decoded.sensors[0U].drv_ver_maj() == 2U);
    CASTLE_SAMPLE_CHECK(decoded.sensors[0U].drv_ver_min() == 3U);
    CASTLE_SAMPLE_CHECK(decoded.sensors[0U].test_state == mon_spt_test_state::finished);
    CASTLE_SAMPLE_CHECK(decoded.results[0U].sensor_type == mon_spt_sensor_type::gyro_x);
    CASTLE_SAMPLE_CHECK(decoded.results[0U].res_type == mon_spt_res_type::positive_offset);
    CASTLE_SAMPLE_CHECK(decoded.results[0U].value == -1234);
    CASTLE_SAMPLE_CHECK(decoded.results[1U].res_type == mon_spt_res_type::self_test_passed);
    CASTLE_SAMPLE_CHECK(decoded.results[1U].value == 1);

    // Too small an output buffer is reported, not overrun.
    CASTLE_SAMPLE_CHECK(encode_message(report, frame, written - 1U, written) == castle::status::full);

    // Request the message with an empty poll frame.
    CASTLE_SAMPLE_CHECK((encode_poll<mon_spt<2U, 4U>>(frame, sizeof(frame), written) == castle::status::ok));
    return 0;
}
