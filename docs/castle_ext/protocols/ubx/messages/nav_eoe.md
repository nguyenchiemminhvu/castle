# UBX NAV-EOE

## Overview

Typed model of UBX-NAV-EOE (class 0x01, ID 0x61), the periodic end-of-epoch marker. The receiver sends it after all enabled NAV class messages (and NMEA messages) of a navigation epoch, so a host can treat everything received since the previous marker as one epoch.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_eoe.hpp"`

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
| `nav_eoe` | Value type for the payload; derives from `detail::fixed_message<nav_eoe, UBX_CLASS_NAV, UBX_ID_NAV_EOE, 4U>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav_eoe::msg_class` | `uint8_t` | `UBX_CLASS_NAV` (0x01) | UBX class. |
| `nav_eoe::msg_id` | `uint8_t` | `UBX_ID_NAV_EOE` | UBX message ID. |
| `nav_eoe::payload_length` | `castle::size_type` | `4U` | Exact payload length. |

### Fields

| Offset | Field | Type | Unit / scale | Description |
| --- | --- | --- | --- | --- |
| 0 | `i_tow` | `uint32_t` | ms | GPS time of week of the epoch that just ended. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool nav_eoe::matches(const message_view& raw)` | `bool` | True when `raw` carries class 0x01 / ID `UBX_ID_NAV_EOE`. |
| `static bool nav_eoe::decode(const message_view& raw, nav_eoe& out)` | `bool` | Decodes an exactly 4-byte payload. On a wrong class/ID or length returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the payload (reserved bytes as zero); used by `encode_message()` for loop-back/simulation. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_eoe.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_eoe;

void on_message(const message_view& raw)
{
    nav_eoe value{};
    if (nav_eoe::matches(raw) && nav_eoe::decode(raw, value))
    {
        // All messages for epoch value.i_tow have been received; publish them.
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav_eoe.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_eoe.cpp).

## Constraints & Notes

- The message carries no flags; matching `i_tow` against the collected NAV messages is up to the host.
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); copying it copies the whole decoded payload.
- `decode()` accepts only a payload of exactly 4 bytes; shorter or longer payloads are rejected rather than partially decoded.
- Raw wire integers are stored unchanged; the `*_m()` / `*_deg()` style helpers convert to `double` on demand, so decoding performs no floating-point math.
- `message_view` is non-owning: its payload must stay valid while `decode()` runs; the decoded struct owns its data afterwards.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
