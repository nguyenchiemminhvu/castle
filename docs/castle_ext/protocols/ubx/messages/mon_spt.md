# UBX MON-SPT

## Overview

Model of UBX-MON-SPT (0x0a 0x2f), the sensor production test report (polled). It is only supported when a sensor (IMU) is connected directly to the u-blox chip. A 4-byte header is followed by two repeated groups: `numSensor` 4-byte sensor blocks and then `numRes` 12-byte result blocks (`4 + numSensor·4 + numRes·12`). Each group is stored in its own fixed-capacity vector.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_spt.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/types.hpp](../../../../core/types.md)
- [castle/container/vector.hpp](../../../../container/vector.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/repeated_group.hpp](detail/repeated_group.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_spt_sensor_id` | `enum class : uint8_t`, spec-defined `sensorId` values (`st_lsm6ds0 = 1`, `invensense_mpu6500 = 2`, `bosch_bmi160 = 3`, `st_lsm6ds3 = 7`, `bosch_smi130 = 9`, `invensense_mpu6515 = 12`, `st_lsm6dsl = 13`, `bosch_smg130 = 14`, `bosch_smi230 = 15`, `bosch_bmi260 = 16`, `st_icm330dlc = 17`, `st_icm330dhcx = 18`). Other (reserved) values are kept as-is. |
| `mon_spt_test_state` | `enum class : uint8_t`: `not_started = 0`, `running = 1`, `error = 2`, `finished = 3`. |
| `mon_spt_sensor_type` | `enum class : uint16_t`: `gyro_z = 5`, `gyro_temperature = 12`, `gyro_y = 13`, `gyro_x = 14`, `accel_x = 16`, `accel_y = 17`, `accel_z = 18`, `barometer = 19`, `magnetometer_x = 22`, `magnetometer_y = 23`, `magnetometer_z = 24`, `barometer_temperature = 25`. |
| `mon_spt_res_type` | `enum class : uint16_t`: `no_offset = 1`, `positive_offset = 2`, `negative_offset = 3`, `min_off_to_pos = 4`, `max_off_to_pos = 5`, `min_neg_to_pos = 6`, `max_neg_to_pos = 7`, `self_test_passed = 8`. |
| `mon_spt_sensor` | One 4-byte sensor block (`wire_size = 4`). |
| `mon_spt_result` | One 12-byte result block (`wire_size = 12`). |
| `mon_spt<MaxSensors = 4, MaxResults = 16>` | Message model with two independent capacities. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `mon_spt::msg_class` / `msg_id` | `0x0A` / `0x2F` | UBX class / ID. |
| `mon_spt::header_length` | `4` | Header size. |
| `mon_spt::sensor_block_length` / `result_block_length` | `4` / `12` | Block sizes. |
| `mon_spt_sensor::DRV_VER_MAJ` | `0x0F` | `drvVer` bits 3..0: driver major version. |
| `mon_spt_sensor::DRV_VER_MIN` | `0xF0` | `drvVer` bits 7..4: driver minor version. |
| `mon_spt_sensor::DRV_VER_MIN_SHIFT` | `4` | Shift for `DRV_VER_MIN`. |

### `mon_spt` fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | 0 | Message version (`0x01`). |
| `num_sensor` | `uint8_t` | 1 | On-wire sensor count; may exceed `sensors.size()`. Encode writes `sensors.size()`. |
| `num_res` | `uint8_t` | 2 | On-wire result count; may exceed `results.size()`. Encode writes `results.size()`. |
| (reserved0) | `U1` | 3 | Skipped on decode, written as zero. |
| `sensors` | `castle::container::vector<mon_spt_sensor, MaxSensors>` | 4 + n·4 | Sensor blocks. |
| `results` | `castle::container::vector<mon_spt_result, MaxResults>` | 4 + numSensor·4 + n·12 | Result blocks. |

### `mon_spt_sensor` fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `sensor_id` | `mon_spt_sensor_id` | +0 | Sensor ID. |
| `drv_ver` | `uint8_t` | +1 | Driver version bitfield (`DRV_VER_*`). |
| `test_state` | `mon_spt_test_state` | +2 | Self-test state. |
| `drv_file_name` | `uint8_t` | +3 | `0` = driver loaded from image, else last character of the driver file name. |

### `mon_spt_result` fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `sensor_id_res` | `uint16_t` | +0 | Sensor ID (same values as `mon_spt_sensor_id`, U2 on the wire). |
| `sensor_type` | `mon_spt_sensor_type` | +2 | Sensor type and axis. |
| `res_type` | `mon_spt_res_type` | +4 | Meaning of `value`. |
| (reserved1) | `U1[2]` | +6 | Skipped on decode, written as zero. |
| `value` | `int32_t` | +8 | Test result value. |

### Functions

| Function | Description |
| --- | --- |
| `static bool mon_spt::matches(const message_view&)` | `true` for class `0x0A` / ID `0x2F`. |
| `static bool mon_spt::decode(const message_view&, mon_spt&)` | `false` for wrong class/ID or a length other than `4 + numSensor·4 + numRes·12`. |
| `void mon_spt::encode(payload_writer&) const` | Writes the header (counts from the vector sizes) and both groups. |
| `uint8_t mon_spt_sensor::drv_ver_maj() const` | Bits 3..0 of `drv_ver`. |
| `uint8_t mon_spt_sensor::drv_ver_min() const` | Bits 7..4 of `drv_ver`, shifted down. |


## Usage Example

```cpp
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void on_message(const message_view& raw) {
    mon_spt<> report{};
    if (mon_spt<>::decode(raw, report)) {
        for (const mon_spt_result& result : report.results) {
            if (result.res_type == mon_spt_res_type::self_test_passed) {
                bool passed = (result.value == 1);
                (void)passed;
            }
        }
    }
}

// Poll: encode_poll<mon_spt<>>(frame, sizeof(frame), written);
```

The full, runnable example is `samples/castle_ext/protocols/ubx/messages/mon_spt.cpp`.

## Constraints & Notes

- Payload length must be exactly `4 + numSensor·4 + numRes·12`, checked against both declared counts.
- Blocks beyond either capacity are skipped per group, so the result group is always read from its true wire offset even when sensors overflow.
- The spec names `drvVer` bits 3..0 `drvVerMaj` and bits 7..4 `drvVerMin`; the accessors follow the spec as written.
- `sensorIdRes` is U2 on the wire while `sensorId` is U1; `sensor_id_res` stays a raw `uint16_t`.
- No heap allocation: every member lives in the value itself (fixed-capacity Castle containers only); no exceptions, no RTTI.
- `decode()` sets `out` to a value-initialised object first and resets it again on any failure, so callers never observe partial state.
- `message_view` is non-owning; its payload must stay valid during `decode()`. Decoded values own their data and may outlive the frame.
- Thread-safety: none. Values are plain value types; concurrent access to one instance needs external synchronisation.
