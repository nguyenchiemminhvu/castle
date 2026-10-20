# UBX MON-HW2

## Overview

Model of UBX-MON-HW2 (class `0x0A`, ID `0x0B`), the 28-byte extended hardware status: RF front-end I/Q imbalance and magnitude, low-level configuration source and POST status. Deprecated in this protocol version in favour of UBX-MON-HW3 and UBX-MON-RF, but still output. Built on `detail::fixed_message`.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_hw2.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/fixed_message.hpp](detail/fixed_message.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_hw2_cfg_source` | `enum class : uint8_t` for `cfgSource`: `flash_image = 102`, `otp = 111`, `config_pins = 112`, `rom = 114`. |
| `mon_hw2` | UBX-MON-HW2 payload (`fixed_message<mon_hw2, UBX_CLASS_MON, UBX_ID_MON_HW2, 28U>`). |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `msg_class` / `msg_id` | `0x0A` / `0x0B` | Inherited from `fixed_message`. |
| `payload_length` | `28U` | Exact payload length. |

### Fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `ofs_i` | `int8_t` | 0 | I-part imbalance (-128 max negative .. 127 max positive). |
| `mag_i` | `uint8_t` | 1 | I-part magnitude (0 no signal .. 255 max). |
| `ofs_q` | `int8_t` | 2 | Q-part imbalance. |
| `mag_q` | `uint8_t` | 3 | Q-part magnitude. |
| `cfg_source` | `mon_hw2_cfg_source` | 4 | Source of low-level configuration. |
| — | reserved | 5..7 | |
| `low_lev_cfg` | `uint32_t` | 8 | Low-level configuration (obsolete for protocol > 15.00). |
| — | reserved | 12..19 | |
| `post_status` | `uint32_t` | 20 | POST status word. |
| — | reserved | 24..27 | |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | Class/ID check. |
| `static bool decode(const message_view&, mon_hw2&)` | Exact-length decode; `out` reset on failure. |
| `void encode(payload_writer&) const` | Writes the payload. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mon_hw2.hpp"

using namespace castle::protocols::ubx::messages;

mon_hw2 hw{};
if (mon_hw2::decode(raw, hw) && (hw.cfg_source == mon_hw2_cfg_source::otp))
{
    int imbalance = hw.ofs_i; // smaller magnitude is better
}
```

See `samples/castle_ext/protocols/ubx/messages/mon_hw2.cpp`.

## Constraints & Notes

- Value type: every field is stored in the object itself; no heap allocation, no exceptions.
- `decode()` rejects a different class/ID or any payload length other than the exact 28 bytes and leaves `out` value-initialised on failure.
- Reserved bytes are skipped on decode and written as zero on encode.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Thread-safety: none; not thread-safe. Use one instance per thread or synchronise externally.
- `cfg_source` is stored as received; values outside the four spec-listed codes are kept verbatim in the enum.
- Rules of thumb from the spec: smaller `|ofs_i|` / `|ofs_q|` is better; `mag_i` and `mag_q` should ideally be equal.
