# UBX NAV2-DOP

## Overview

Typed model of UBX-NAV2-DOP (class 0x29, ID 0x04), the secondary-output dilution-of-precision values. Its payload is identical to UBX-NAV-DOP, so `nav2_dop` is an alias of the shared template `basic_nav_dop` with the NAV2 class/ID; fields and helpers are documented in [`nav_dop.md`](nav_dop.md).

## Header

`#include "castle_ext/protocols/ubx/messages/nav2_dop.hpp"`

## Dependencies

- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/messages/nav_dop.hpp`](nav_dop.md) (`basic_nav_dop`)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav2_dop` | `basic_nav_dop<UBX_CLASS_NAV2, UBX_ID_NAV2_DOP>`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `nav2_dop::msg_class` | `uint8_t` | `UBX_CLASS_NAV2` (0x29) | UBX class. |
| `nav2_dop::msg_id` | `uint8_t` | `UBX_ID_NAV2_DOP` (0x04) | UBX message ID. |
| `nav2_dop::payload_length` | `castle::size_type` | `18U` | Exact payload length. |

### Fields

Identical to [`nav_dop`](nav_dop.md#fields): `i_tow` (ms), `g_dop`, `p_dop`, `t_dop`, `v_dop`, `h_dop`, `n_dop`, `e_dop` (all `uint16_t`, scale 0.01).

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool matches(const message_view& raw)` | `bool` | True only for class 0x29 / ID 0x04 (a NAV-DOP frame does not match). |
| `static bool decode(const message_view& raw, nav2_dop& out)` | `bool` | Decodes an exactly 18-byte payload; `out` is value-initialised on failure. |
| `void encode(payload_writer& writer) const` | - | Writes the payload; used by `encode_message()`. |
| `gdop_value()` ... `edop_value()` | `double` | Raw value × 0.01. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav2_dop.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav2_dop;

double secondary_hdop(const message_view& raw)
{
    nav2_dop dop{};
    return nav2_dop::decode(raw, dop) ? dop.hdop_value() : 99.99;
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/nav2_dop.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav2_dop.cpp).

## Constraints & Notes

- `nav2_dop` and `nav_dop` are distinct types sharing one layout definition. `nav2_dop` no longer derives from `nav_dop`, so a `nav2_dop` cannot be passed where a `nav_dop&` is expected.
- Fixed-size value type (no heap, no exceptions, no RTTI, no virtual); `decode()` rejects any payload that is not exactly 18 bytes.
- Thread-safety: none. Instances are plain values and are not thread-safe; synchronise externally when shared.
