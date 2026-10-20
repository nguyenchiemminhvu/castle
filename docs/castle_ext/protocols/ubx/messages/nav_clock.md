# UBX NAV-CLOCK

## Overview

Model of the periodic/polled UBX-NAV-CLOCK output (0x01 0x22): the receiver clock bias and drift with their accuracy estimates.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_clock.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md) (supplies `matches()`, `decode()`, `encode()`; payloads are read/written through [`payload_reader`](../payload_reader.md) / [`payload_writer`](../payload_writer.md))

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_clock` | `struct nav_clock : detail::fixed_message<nav_clock, UBX_CLASS_NAV, UBX_ID_NAV_CLOCK, 20U>`. |

### Constants

| Name | Type | Value |
| --- | --- | --- |
| `nav_clock::msg_class` | `uint8_t` | `UBX_CLASS_NAV` |
| `nav_clock::msg_id` | `uint8_t` | `UBX_ID_NAV_CLOCK` |
| `nav_clock::payload_length` | `castle::size_type` | `20U` |

### Fields

| Field | Offset | Type | Unit | Description |
| --- | --- | --- | --- | --- |
| `i_tow` | 0 | `uint32_t` | ms | `iTOW`: GPS time of week of the navigation epoch. |
| `clk_b` | 4 | `int32_t` | ns | `clkB`: clock bias. |
| `clk_d` | 8 | `int32_t` | ns/s | `clkD`: clock drift. |
| `t_acc` | 12 | `uint32_t` | ns | `tAcc`: time accuracy estimate. |
| `f_acc` | 16 | `uint32_t` | ps/s | `fAcc`: frequency accuracy estimate. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static matches(const message_view& raw)` | `bool` | `true` when `raw` carries this class/ID. |
| `static decode(const message_view& raw, nav_clock& out)` | `bool` | Requires class/ID match and payload length exactly 20; otherwise returns `false` with `out` value-initialised. |
| `encode(payload_writer& writer) const` | `void` | Writes the 20-byte payload; used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_clock.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_clock;

void handle(const message_view& raw)
{
    nav_clock time{};
    if (nav_clock::decode(raw, time))
    {
        const int32_t bias_ns = time.clk_b;
        const int32_t drift_ns_per_s = time.clk_d;
        (void)bias_ns;
        (void)drift_ns_per_s;
    }
}
```

Full walk-through: [`samples/castle_ext/protocols/ubx/messages/nav_clock.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_clock.cpp).

## Constraints & Notes

- Fixed 20-byte payload; `decode()` rejects shorter and longer payloads (the UBX length field is authoritative) and leaves `out` value-initialised on any failure.
- No heap allocation, exceptions, RTTI or virtual dispatch; the model is a plain value type that owns its data (it does not alias the parser buffer).
- All fields are stored as the exact wire integers in their spec units.
- Not thread-safe; synchronise externally if shared.
