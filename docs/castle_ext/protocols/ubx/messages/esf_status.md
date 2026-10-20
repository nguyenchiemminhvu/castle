# UBX ESF-STATUS

## Overview

Model of UBX-ESF-STATUS (0x10 0x10), the periodic/polled sensor-fusion status: the fusion mode plus one status block
per sensor. Payload: `16 + numSens·4`.

**Change:** the `sensStatus1` masks were wrong. `TYPE_MASK 0x1F` / `USED 0x20` / `READY 0x40` are replaced by
spec-named `SENS_STATUS1_TYPE 0x3F` (bits 5..0), `SENS_STATUS1_USED 0x40` (bit 6) and `SENS_STATUS1_READY 0x80`
(bit 7). Also: `CALIB_STATUS_MASK` / `TIME_STATUS_MASK` were renamed `SENS_STATUS2_*`, `FAULTS_*` and accessors
were added, `fusion_mode` is now the `esf_fusion_mode` enum, storage is a `vector`, `num_sens` is validated exactly,
and `encode()` was added.

## Header

`#include "castle_ext/protocols/ubx/messages/esf_status.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/types.hpp](../../../../core/types.md)
- [castle/container/vector.hpp](../../../../container/vector.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/repeated_group.hpp](detail/repeated_group.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `esf_fusion_mode : uint8_t` | `initialization` (0), `fusion` (1), `suspended` (2), `disabled` (3). |
| `esf_calib_status : uint8_t` | `not_calibrated` (0), `calibrating` (1), `calibrated` (2), `calibrated_alt` (3). |
| `esf_time_status : uint8_t` | `no_data` (0), `first_byte` (1), `event_input` (2), `time_tag` (3). |
| `esf_status_sensor` | One sensor block, `wire_size = 4`. |
| `esf_status<MaxSensors = 32U>` | Message payload. |

### Constants

| Name | Value | Meaning |
| --- | --- | --- |
| `msg_class` / `msg_id` | `0x10` / `0x10` | Frame identity. |
| `header_length` / `block_length` | `16` / `4` | Wire sizes. |
| `SENS_STATUS1_TYPE` / `_USED` / `_READY` | `0x3F` / `0x40` / `0x80` | `sensStatus1` fields. |
| `SENS_STATUS2_CALIB_STATUS` / `_TIME_STATUS` | `0x03` / `0x0C` | `sensStatus2` fields. |
| `FAULTS_BAD_MEAS` / `_BAD_TTAG` / `_MISSING_MEAS` / `_NOISY_MEAS` | `0x01` / `0x02` / `0x04` / `0x08` | `faults` bits. |

### Fields

| Offset | Field | Type | Description |
| --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms |
| 4 | `version` | `uint8_t` | 0x02 for this version |
| 5 | reserved | 7 bytes | written as zero |
| 12 | `fusion_mode` | `esf_fusion_mode` | Fusion filter mode. |
| 13 | reserved | 2 bytes | written as zero |
| 15 | `num_sens` | `uint8_t` | On-wire sensor count; `encode()` writes `sensors.size()`. |
| 16 + n·4 | `sensors[n].sens_status1` / `sens_status2` / `freq` [Hz] / `faults` | `uint8_t` each | Sensor block. |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | Class/ID check. |
| `static bool decode(const message_view&, esf_status&)` | Requires length `16 + numSens·4` exactly; `out` value-initialised on failure. |
| `void encode(payload_writer&) const` | Header + stored sensors (loop-back/testing). |
| `uint8_t type() const`, `bool used() const`, `bool ready() const` | `sensStatus1` fields. |
| `esf_calib_status calib_status() const`, `bool calibrated() const` | `sensStatus2` bits 1..0; `calibrated()` is true for 10 and 11. |
| `esf_time_status time_status() const` | `sensStatus2` bits 3..2. |
| `bool bad_meas() / bad_ttag() / missing_meas() / noisy_meas() const` | `faults` bits 0..3. |

## Usage Example

```cpp
esf_status<8U> status;
if (esf_status<8U>::decode(raw, status) && (status.fusion_mode == esf_fusion_mode::fusion))
{
    for (const esf_status_sensor& sensor : status.sensors) { bool ok = sensor.used() && sensor.calibrated(); }
}
```

Full walk-through: `samples/castle_ext/protocols/ubx/messages/esf_status.cpp`.

## Constraints & Notes

- Capacity: `MaxSensors` blocks are stored; the rest are skipped, and `num_sens` keeps the wire count.
- Shorter or longer payloads than `16 + numSens·4` are rejected.
- Value type: every member lives in the object; no heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
