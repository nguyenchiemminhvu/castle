# UBX CFG-SPT

## Overview

Typed model of UBX-CFG-SPT (class 0x06, ID 0x64), a get/set message that configures and starts a sensor production test. The test uses the built-in self-test of an attached sensor and reports its results in UBX-MON-SPT. The message is only supported when a sensor is directly connected to the receiver.

## Header

`#include "castle_ext/protocols/ubx/messages/cfg_spt.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `cfg_spt` | Payload model; derives from `detail::fixed_message<cfg_spt, UBX_CLASS_CFG, UBX_ID_CFG_SPT, 12U>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `cfg_spt::msg_class` | `uint8_t` | `UBX_CLASS_CFG` (0x06) | UBX class. |
| `cfg_spt::msg_id` | `uint8_t` | `UBX_ID_CFG_SPT` (0x64) | UBX message ID. |
| `cfg_spt::payload_length` | `castle::size_type` | `12U` | Exact payload length. |
| `cfg_spt::version` | `uint8_t` | `0x00` | Message version (payload byte 0). |

### Fields

| Offset | Field | Type | Description |
| --- | --- | --- | --- |
| 0 | (version) | `U1` | Written as `version`. Decoding rejects any other value. |
| 1 | - | reserved | Written as zero, ignored on decode. |
| 2 | `sensor_id` | `uint16_t` | ID of the sensor to test (IDs listed in UBX-MON-SPT, e.g. 3 = Bosch BMI160). |
| 4 | - | reserved[8] | Written as zero, ignored on decode. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool cfg_spt::matches(const message_view& raw)` | `bool` | True for class 0x06 / ID 0x64. |
| `static bool cfg_spt::decode(const message_view& raw, cfg_spt& out)` | `bool` | Decodes an exactly 12-byte payload with version 0. On failure returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the 12-byte payload, used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/cfg_spt.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

castle::status start_self_test(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    cfg_spt test{};
    test.sensor_id = 3U;
    return encode_message(test, frame, capacity, written);
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/cfg_spt.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/cfg_spt.cpp).

## Constraints & Notes

- Fixed-size value type: no heap, exceptions, RTTI or virtual functions.
- `decode()` accepts only a payload of exactly 12 bytes whose version byte is 0.
- Thread-safety: none. Instances are plain values and are not thread-safe.
