# UBX CFG-RST

## Overview

Typed model of UBX-CFG-RST (class 0x06, ID 0x04), the command the host sends to reset the receiver and optionally clear backup (BBR) data structures. Do not wait for an acknowledgement: newer firmware never sends one, and older firmware may reset before the ACK has been sent completely.

## Header

`#include "castle_ext/protocols/ubx/messages/cfg_rst.hpp"`

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
| `cfg_rst` | Payload model; derives from `detail::fixed_message<cfg_rst, UBX_CLASS_CFG, UBX_ID_CFG_RST, 4U>`. |
| `enum class cfg_rst_mode : uint8_t` | `hardware_reset` (0x00, watchdog, immediately), `software_reset` (0x01, controlled), `software_reset_gnss_only` (0x02), `hardware_reset_after_shutdown` (0x04), `gnss_stop` (0x08), `gnss_start` (0x09). |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `cfg_rst::msg_class` / `msg_id` | `uint8_t` | `UBX_CLASS_CFG` / `UBX_ID_CFG_RST` (0x04) | UBX class/ID. |
| `cfg_rst::payload_length` | `castle::size_type` | `4U` | Exact payload length. |
| `BBR_EPH` ... `BBR_RTC` | `uint16_t` | `0x0001` ... `0x0100` | `navBbrMask` bits 0..8: ephemeris, almanac, health, Klobuchar, position, clock drift, oscillator, UTC/leap seconds, RTC. |
| `BBR_AOP` | `uint16_t` | `0x8000` | Bit 15: autonomous orbit parameters. |
| `BBR_HOT_START` / `BBR_WARM_START` / `BBR_COLD_START` | `uint16_t` | `0x0000` / `0x0001` / `0xFFFF` | Spec-defined preset masks. |

### Fields

| Offset | Field | Type | Default | Description |
| --- | --- | --- | --- | --- |
| 0 | `nav_bbr_mask` | `uint16_t` | `BBR_HOT_START` | BBR sections to clear. |
| 2 | `reset_mode` | `cfg_rst_mode` | `software_reset` | Reset type. |
| 3 | - | reserved | - | Written as zero, ignored on decode. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static constexpr cfg_rst hot_start(cfg_rst_mode = software_reset)` | `cfg_rst` | Keep all BBR data. |
| `static constexpr cfg_rst warm_start(cfg_rst_mode = software_reset)` | `cfg_rst` | Clear ephemeris. |
| `static constexpr cfg_rst cold_start(cfg_rst_mode = software_reset)` | `cfg_rst` | Clear all BBR sections. |
| `static bool matches(const message_view& raw)` | `bool` | True for class 0x06 / ID 0x04. |
| `static bool decode(const message_view& raw, cfg_rst& out)` | `bool` | Loop-back decode of an exactly 4-byte payload. On failure returns `false` and leaves `out` value-initialised. |
| `void encode(payload_writer& writer) const` | - | Writes the 4-byte payload, used by `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/cfg_rst.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

castle::status cold_reset(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    return encode_message(cfg_rst::cold_start(), frame, capacity, written);
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/cfg_rst.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/cfg_rst.cpp).

## Constraints & Notes

- Fixed-size, `constexpr`-constructible value type: no heap, exceptions, RTTI or virtual functions.
- `reset_mode` is not range-checked on encode. Values outside the enumeration are sent as they are.
- Thread-safety: none. Instances are plain values and are not thread-safe.
