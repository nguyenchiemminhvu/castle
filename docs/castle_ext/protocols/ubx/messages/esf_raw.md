# UBX ESF-RAW

## Overview

Model of UBX-ESF-RAW (0x10 0x03), raw output from the IMU connected directly to the receiver (accelerometer,
gyroscope, temperature). Payload: `4 + N·8`, i.e. 4 reserved bytes and `N` blocks of `data` word + `sTtag`. `N` is not
a field; it is implied by the payload length (`detail::implied_group_count`).

## Header

`#include "castle_ext/protocols/ubx/messages/esf_raw.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/types.hpp](../../../../core/types.md)
- [castle/container/vector.hpp](../../../../container/vector.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/esf_data.hpp](detail/esf_data.md)
- [detail/repeated_group.hpp](detail/repeated_group.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `esf_raw_sample` | One block: `data_type` (bits 31..24), `data_value` (bits 23..0, sign-extended), `s_ttag`. `wire_size = 8`. |
| `esf_raw<MaxSamples = 16U>` | Message payload. |

### Constants

| Name | Value | Meaning |
| --- | --- | --- |
| `msg_class` / `msg_id` | `0x10` / `0x03` | Frame identity. |
| `header_length` / `block_length` | `4` / `8` | Wire sizes. |
| `esf_raw_sample::DATA_TYPE_MASK` | `0xFF` | 8-bit `dataType`. |

### Fields

| Offset | Field | Type | Description |
| --- | --- | --- | --- |
| 0 | reserved | 4 bytes | written as zero |
| 4 + n·8 | `samples[n].data_type` / `data_value` | `uint8_t` / `int32_t` | Data word. |
| 8 + n·8 | `samples[n].s_ttag` | `uint32_t` | Sensor time tag. |
| — | `num_samples` | `castle::size_type` | Block count implied by the length; may exceed `samples.size()`. Not written by `encode()`. |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | Class/ID check. |
| `static bool decode(const message_view&, esf_raw&)` | Requires `length ≥ 4` and `(length - 4) % 8 == 0`; `out` value-initialised on failure. |
| `void encode(payload_writer&) const` | Writes the reserved header and every stored sample (loop-back/testing). |

## Usage Example

```cpp
esf_raw<8U> imu;
if (esf_raw<8U>::decode(raw, imu))
{
    for (const esf_raw_sample& sample : imu.samples) { /* dispatch on sample.data_type */ }
}
```

Full walk-through: `samples/castle_ext/protocols/ubx/messages/esf_raw.cpp`.

## Constraints & Notes

- Capacity: `MaxSamples` blocks are stored; the rest are skipped, and `num_samples` reports the wire count.
- A header-only payload (4 bytes, zero samples) is valid.
- `encode()` latches `castle::status::out_of_range` for a `data_value` outside the signed 24-bit range.
- Value type: every member lives in the object; no heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
