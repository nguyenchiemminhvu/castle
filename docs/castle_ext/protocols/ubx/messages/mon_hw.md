# UBX MON-HW

## Overview

Model of UBX-MON-HW (class `0x0A`, ID `0x09`), the 60-byte hardware status: PIO pin masks, noise level, AGC, antenna supervisor, jamming state and virtual pin mapping. Deprecated in this protocol version in favour of UBX-MON-HW3 and UBX-MON-RF, but still output. Built on `detail::fixed_message`; the antenna and jamming enumerations are shared with `mon_rf.hpp`.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_hw.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/fixed_message.hpp](detail/fixed_message.md)
- [castle/container/array.hpp](../../../../container/array.md)
- [mon_rf.hpp](mon_rf.md) (`mon_jamming_state`, `mon_antenna_status`, `mon_antenna_power`)

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_hw` | UBX-MON-HW payload (`fixed_message<mon_hw, UBX_CLASS_MON, UBX_ID_MON_HW, 60U>`). |
| `mon_antenna_status` | (from `mon_rf.hpp`) `init = 0`, `dont_know = 1`, `ok = 2`, `short_circuit = 3` (`SHORT`), `open = 4`. |
| `mon_antenna_power` | (from `mon_rf.hpp`) `off = 0`, `on = 1`, `dont_know = 2`. |
| `mon_jamming_state` | (from `mon_rf.hpp`) `unknown = 0`, `ok = 1`, `warning = 2`, `critical = 3`. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `msg_class` / `msg_id` | `0x0A` / `0x09` | Inherited from `fixed_message`. |
| `payload_length` | `60U` | Exact payload length. |
| `FLAGS_RTC_CALIB` | `0x01U` | `flags` bit 0: RTC calibrated. |
| `FLAGS_SAFE_BOOT` | `0x02U` | `flags` bit 1: safeboot mode active. |
| `FLAGS_JAMMING_STATE` | `0x0CU` | `flags` bits 3..2: jamming state. |
| `FLAGS_XTAL_ABSENT` | `0x10U` | `flags` bit 4: RTC crystal absent. |
| `VP_COUNT` | `17U` | Entries of `vp`. |

### Fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `pin_sel` | `uint32_t` (X4) | 0 | Pins set as peripheral/PIO. |
| `pin_bank` | `uint32_t` (X4) | 4 | Pins set as bank A/B. |
| `pin_dir` | `uint32_t` (X4) | 8 | Pins set as input/output. |
| `pin_val` | `uint32_t` (X4) | 12 | Pin values low/high. |
| `noise_per_ms` | `uint16_t` | 16 | Noise level measured by the GPS core. |
| `agc_cnt` | `uint16_t` | 18 | AGC monitor (0..8191). |
| `a_status` | `mon_antenna_status` | 20 | Antenna supervisor state. |
| `a_power` | `mon_antenna_power` | 21 | Antenna power state. |
| `flags` | `uint8_t` (X1) | 22 | See `FLAGS_*`. |
| — | reserved | 23 | |
| `used_mask` | `uint32_t` (X4) | 24 | Pins used by the virtual pin manager. |
| `vp` | `castle::container::array<uint8_t, 17>` | 28 | Pin mapping per physical pin. |
| `jam_ind` | `uint8_t` | 45 | CW jamming indicator (0 none .. 255 strong). |
| — | reserved | 46..47 | |
| `pin_irq` | `uint32_t` (X4) | 48 | Pins using the PIO IRQ. |
| `pull_h` | `uint32_t` (X4) | 52 | Pins using the pull-high resistor. |
| `pull_l` | `uint32_t` (X4) | 56 | Pins using the pull-low resistor. |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | Class/ID check. |
| `static bool decode(const message_view&, mon_hw&)` | Exact-length decode; `out` reset on failure. |
| `void encode(payload_writer&) const` | Writes the payload. |
| `bool rtc_calib() const` | `flags` bit 0. |
| `bool safe_boot() const` | `flags` bit 1. |
| `mon_jamming_state jamming_state() const` | `flags` bits 3..2, shifted down. |
| `bool xtal_absent() const` | `flags` bit 4 (not supported for protocol < 18.00). |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mon_hw.hpp"

using namespace castle::protocols::ubx::messages;

mon_hw hw{};
if (mon_hw::decode(raw, hw) && (hw.jamming_state() == mon_jamming_state::critical))
{
    // interference visible and no fix
}
```

See `samples/castle_ext/protocols/ubx/messages/mon_hw.cpp`.

## Constraints & Notes

- Value type: every field is stored in the object itself; no heap allocation, no exceptions.
- `decode()` rejects a different class/ID or any payload length other than the exact 60 bytes and leaves `out` value-initialised on failure.
- Reserved bytes are skipped on decode and written as zero on encode.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Thread-safety: none; not thread-safe. Use one instance per thread or synchronise externally.
- Enumerated fields are stored as received; out-of-range codes are kept verbatim in the enum.
