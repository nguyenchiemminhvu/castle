// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_spt.hpp
 * @brief UBX-MON-SPT (0x0a 0x2f): sensor production test.
 *
 * Polled output: a 4-byte header, `numSensor` 4-byte sensor blocks, then
 * `numRes` 12-byte result blocks (`4 + numSensor·4 + numRes·12`). Only
 * supported when a sensor (IMU) is connected directly to the u-blox chip.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_SPT_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_SPT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"
#include "castle_ext/protocols/ubx/messages/detail/repeated_group.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief Sensor IDs defined by the spec (`sensorId`; other values are reserved). */
enum class mon_spt_sensor_id : uint8_t
{
    st_lsm6ds0 = 1U,         ///< ST LSM6DS0 6-axis IMU with temperature sensor.
    invensense_mpu6500 = 2U, ///< Invensense MPU6500 6-axis IMU with temperature sensor.
    bosch_bmi160 = 3U,       ///< Bosch BMI160 6-axis IMU with temperature sensor.
    st_lsm6ds3 = 7U,         ///< ST LSM6DS3 6-axis IMU with temperature sensor.
    bosch_smi130 = 9U,       ///< Bosch SMI130 6-axis IMU with temperature sensor.
    invensense_mpu6515 = 12U, ///< Invensense MPU6515 6-axis inertial sensor.
    st_lsm6dsl = 13U,        ///< ST LSM6DSL 6-axis IMU with temperature sensor.
    bosch_smg130 = 14U,      ///< Bosch SMG130 3-axis gyroscope with temperature sensor.
    bosch_smi230 = 15U,      ///< Bosch SMI230 6-axis IMU with temperature sensor.
    bosch_bmi260 = 16U,      ///< Bosch BMI260 6-axis IMU with temperature sensor.
    st_icm330dlc = 17U,      ///< ST ICM330DLC 6-axis IMU with temperature sensor.
    st_icm330dhcx = 18U      ///< ST ICM330DHCX 6-axis IMU with 105 deg temperature sensor.
};

/** @brief State of one sensor's self-test (`testState`). */
enum class mon_spt_test_state : uint8_t
{
    not_started = 0U, ///< Test not yet started.
    running = 1U,     ///< Test started but not yet finished.
    error = 2U,       ///< Test did not finish due to an error during execution.
    finished = 3U     ///< Test finished normally; test data is available.
};

/** @brief Sensor type and axis a result refers to (`sensorType`; other values are reserved). */
enum class mon_spt_sensor_type : uint16_t
{
    gyro_z = 5U,
    gyro_temperature = 12U,
    gyro_y = 13U,
    gyro_x = 14U,
    accel_x = 16U,
    accel_y = 17U,
    accel_z = 18U,
    barometer = 19U,
    magnetometer_x = 22U,
    magnetometer_y = 23U,
    magnetometer_z = 24U,
    barometer_temperature = 25U
};

/** @brief Type of result stored in `value` (`resType`). */
enum class mon_spt_res_type : uint16_t
{
    no_offset = 1U,       ///< Measurement without self-test offset (raw, unscaled).
    positive_offset = 2U, ///< Measurement with positive self-test offset (raw, unscaled).
    negative_offset = 3U, ///< Measurement with negative self-test offset (raw, unscaled).
    min_off_to_pos = 4U,  ///< Minimum off-to-positive to pass self-test (from on-chip trimming).
    max_off_to_pos = 5U,  ///< Maximum off-to-positive to pass self-test (from on-chip trimming).
    min_neg_to_pos = 6U,  ///< Minimum negative-to-positive to pass self-test (from on-chip trimming).
    max_neg_to_pos = 7U,  ///< Maximum negative-to-positive to pass self-test (from on-chip trimming).
    self_test_passed = 8U ///< Sensor-reported decision: `value` 1 = passed, 0 = failed.
};

/** @brief One sensor block of UBX-MON-SPT. */
struct mon_spt_sensor
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 4U;

    /** @name `drvVer` bits @{ */
    static CASTLE_CONSTEXPR uint8_t DRV_VER_MAJ = 0x0FU; ///< Bits 3..0: driver major version.
    static CASTLE_CONSTEXPR uint8_t DRV_VER_MIN = 0xF0U; ///< Bits 7..4: driver minor version.
    static CASTLE_CONSTEXPR uint8_t DRV_VER_MIN_SHIFT = 4U;
    /** @} */

    mon_spt_sensor_id sensor_id = static_cast<mon_spt_sensor_id>(0U); ///< Sensor ID (raw value kept if reserved).
    uint8_t drv_ver = 0U;  ///< Driver version bitfield; see `DRV_VER_*`.
    mon_spt_test_state test_state = mon_spt_test_state::not_started;
    uint8_t drv_file_name = 0U; ///< 0 = driver loaded from image, else last character of the driver file name.

    /** @brief Returns the driver major version (bits 3..0). */
    CASTLE_NODISCARD uint8_t drv_ver_maj() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(drv_ver & DRV_VER_MAJ);
    }

    /** @brief Returns the driver minor version (bits 7..4). */
    CASTLE_NODISCARD uint8_t drv_ver_min() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>((drv_ver & DRV_VER_MIN) >> DRV_VER_MIN_SHIFT);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.sensor_id);
        stream.field(self.drv_ver);
        stream.field(self.test_state);
        stream.field(self.drv_file_name);
    }
};

/** @brief One result block of UBX-MON-SPT. */
struct mon_spt_result
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 12U;

    uint16_t sensor_id_res = 0U; ///< Sensor ID (values as `mon_spt_sensor_id`, on the wire as U2).
    mon_spt_sensor_type sensor_type = static_cast<mon_spt_sensor_type>(0U);
    mon_spt_res_type res_type = static_cast<mon_spt_res_type>(0U);
    int32_t value = 0; ///< Test result value; meaning given by `res_type`.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.sensor_id_res);
        stream.field(self.sensor_type);
        stream.field(self.res_type);
        stream.reserved(2U);
        stream.field(self.value);
    }
};

/** @brief UBX-MON-SPT output payload. */
template <castle::size_type MaxSensors = 4U, castle::size_type MaxResults = 16U>
struct mon_spt
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_SPT;
    static CASTLE_CONSTEXPR castle::size_type header_length = 4U;
    static CASTLE_CONSTEXPR castle::size_type sensor_block_length = mon_spt_sensor::wire_size;
    static CASTLE_CONSTEXPR castle::size_type result_block_length = mon_spt_result::wire_size;

    uint8_t version = 0U;    ///< Message version (0x01 for this version).
    uint8_t num_sensor = 0U; ///< On-wire sensor block count; may exceed `sensors.size()`.
    uint8_t num_res = 0U;    ///< On-wire result block count; may exceed `results.size()`.
    castle::container::vector<mon_spt_sensor, MaxSensors> sensors{};
    castle::container::vector<mon_spt_result, MaxResults> results{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header, up to `MaxSensors` sensor blocks and up to
     * `MaxResults` result blocks.
     * @return `false` unless the payload length is exactly `4 + numSensor·4 + numRes·12`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_spt& out) CASTLE_NOEXCEPT
    {
        out = mon_spt{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.num_sensor, out.num_res, reader);
        CASTLE_CONST castle::size_type results_offset =
            header_length + (static_cast<castle::size_type>(out.num_sensor) * sensor_block_length);
        if (!detail::has_group_length(raw.payload_length(), results_offset, out.num_res, result_block_length))
        {
            out = mon_spt{};
            return false;
        }

        detail::read_group(reader, out.num_sensor, out.sensors);
        detail::read_group(reader, out.num_res, out.results);
        if (!reader.ok())
        {
            out = mon_spt{};
            return false;
        }
        return true;
    }

    /** @brief Writes the header and every stored block (both counts are taken from the vector sizes). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t sensor_count = static_cast<uint8_t>(sensors.size());
        CASTLE_CONST uint8_t result_count = static_cast<uint8_t>(results.size());
        layout_header(*this, sensor_count, result_count, writer);
        detail::write_group(writer, sensors);
        detail::write_group(writer, results);
    }

private:
    /** @brief Header layout; counts are passed separately so `encode()` need not copy the message. */
    template <typename Self, typename Count, typename Stream>
    static void layout_header(Self& self, Count& num_sensor, Count& num_res, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.field(num_sensor);
        stream.field(num_res);
        stream.reserved(1U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_SPT_HPP
