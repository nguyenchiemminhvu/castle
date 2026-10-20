# UBX NAV-ODO

## Overview

Typed model of UBX-NAV-ODO (class 0x01, ID 0x09), the periodic/polled odometer solution: ground distance since the last odometer reset, total cumulative distance and the distance accuracy.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_odo.hpp"`

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
| `nav_odo` | Value type for the payload; derives from `detail::fixed_message<nav_odo, UBX_CLASS_NAV, UBX_ID_NAV_ODO, 20U>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_odo::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_odo::msg_id` | `uint8_t` | `UBX_ID_NAV_ODO` (0x09) | UBX message ID. |
| `nav_odo::payload_length` | `castle::size_type` | `20U` | Exact payload length. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `version` | `uint8_t` | - | Message version (0x00 for this version). |
| 1 | - | `U1[3]` | - | Reserved; skipped on decode, zero on encode. |
| 4 | `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| 8 | `distance` | `uint32_t` | m | Ground distance since the last odometer reset. |
| 12 | `total_distance` | `uint32_t` | m | Total cumulative ground distance. |
| 16 | `distance_std` | `uint32_t` | m | Ground distance accuracy (1-sigma). |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID 0x09. |
| `static bool decode(const message_view& raw, nav_odo& out)` | `bool` | Decodes an exactly 20-byte payload; `out` is value-initialised on failure. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_odo.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_odo;

uint32_t trip_metres(const message_view& raw)
{
    nav_odo odo{};
    return nav_odo::decode(raw, odo) ? odo.distance : 0U;
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_odo.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_odo.cpp).

## Constraints & Notes

- UBX-NAV-ODO is not listed in the ZED-F9R interface description (UBX-19056845 R01); the layout follows the generic u-blox protocol definition.
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual).
- `decode()` accepts only a payload of exactly 20 bytes; shorter or longer payloads are rejected rather than partially decoded.
- All fields are integer metres; no scaled helpers are needed.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
