# UBX MON-HW3

## Overview

Model of UBX-MON-HW3 (class `0x0A`, ID `0x37`), the I/O pin status: a 22-byte header (flags, `CH[10]` hardware version) followed by one 6-byte block per pin (`22 + nPins·6`). Antenna and RF status are reported by UBX-MON-RF ([mon_rf.md](mon_rf.md)).

## Header

`#include "castle_ext/protocols/ubx/messages/mon_hw3.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/types.hpp](../../../../core/types.md)
- [castle/container/vector.hpp](../../../../container/vector.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/repeated_group.hpp](detail/repeated_group.md)
- [castle/container/string.hpp](../../../../container/string.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_hw3_pin` | One 6-byte pin block. |
| `mon_hw3<MaxPins = 32U>` | Message with up to `MaxPins` stored pins. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `mon_hw3::msg_class` / `msg_id` | `0x0A` / `0x37` | Class/ID. |
| `mon_hw3::header_length` | `22U` | Header bytes. |
| `mon_hw3::block_length` | `6U` | Bytes per pin block. |
| `mon_hw3::HW_VERSION_LENGTH` | `10U` | Width of `hwVersion` (`CH[10]`). |
| `mon_hw3::FLAGS_RTC_CALIB` | `0x01U` | `flags` bit 0. |
| `mon_hw3::FLAGS_SAFE_BOOT` | `0x02U` | `flags` bit 1. |
| `mon_hw3::FLAGS_XTAL_ABSENT` | `0x04U` | `flags` bit 2. |
| `mon_hw3_pin::PIN_MASK_PERIPH_PIO` | `0x0001U` | 0 = peripheral, 1 = PIO. |
| `mon_hw3_pin::PIN_MASK_PIN_BANK` | `0x000EU` | Bits 3..1: bank 0..7 = A..H. |
| `mon_hw3_pin::PIN_MASK_DIRECTION` | `0x0010U` | 0 = input, 1 = output. |
| `mon_hw3_pin::PIN_MASK_VALUE` | `0x0020U` | 0 = low, 1 = high. |
| `mon_hw3_pin::PIN_MASK_VP_MANAGER` | `0x0040U` | Used by the virtual pin manager. |
| `mon_hw3_pin::PIN_MASK_PIO_IRQ` | `0x0080U` | Interrupt enabled. |
| `mon_hw3_pin::PIN_MASK_PIO_PULL_HIGH` | `0x0100U` | Pull-high resistor in use. |
| `mon_hw3_pin::PIN_MASK_PIO_PULL_LOW` | `0x0200U` | Pull-low resistor in use. |

### `mon_hw3` fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | 0 | Message version (`0x00`). |
| `n_pins` | `uint8_t` | 1 | On-wire pin count; may exceed `pins.size()`. |
| `flags` | `uint8_t` (X1) | 2 | See `FLAGS_*`. |
| `hw_version` | `castle::container::string<10>` | 3 | Hardware version (same as UBX-MON-VER), read with `stream.text`. |
| — | reserved | 13..21 | |
| `pins` | `castle::container::vector<mon_hw3_pin, MaxPins>` | 22 + n·6 | Stored pin blocks. |

### `mon_hw3_pin` fields (offset within block)

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `pin_id` | `uint16_t` | 0 | Pin identifier (external and internal pins). |
| `pin_mask` | `uint16_t` (X2) | 2 | See `PIN_MASK_*`. |
| `vp` | `uint8_t` | 4 | Virtual pin mapping. |
| — | reserved | 5 | |

### Functions

| Function | Description |
| --- | --- |
| `static bool mon_hw3::matches(const message_view&)` | Class/ID check. |
| `static bool mon_hw3::decode(const message_view&, mon_hw3&)` | Validates `22 + nPins·6` exactly, then decodes. |
| `void mon_hw3::encode(payload_writer&) const` | Writes header (`nPins = pins.size()`) and every pin. |
| `bool rtc_calib() const` / `safe_boot()` / `xtal_absent()` | Header `flags` bits. |
| `bool mon_hw3_pin::periph_pio() const` | Bit 0. |
| `uint8_t mon_hw3_pin::pin_bank() const` | Bits 3..1, shifted (0 = A .. 7 = H). |
| `bool direction()` / `value()` / `vp_manager()` / `pio_irq()` / `pio_pull_high()` / `pio_pull_low()` | Bits 4..9 of `pin_mask`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mon_hw3.hpp"

using namespace castle::protocols::ubx::messages;

mon_hw3<> hw{};
if (mon_hw3<>::decode(raw, hw))
{
    const char* version = hw.hw_version.c_str();
    for (const mon_hw3_pin& pin : hw.pins)
    {
        if (pin.periph_pio() && pin.direction()) { /* PIO output */ }
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/mon_hw3.cpp`.

## Constraints & Notes

- Storage: `castle::container::vector<..., MaxPins>` (default `MaxPins = 32U`) lives inside the object; no heap allocation, no exceptions.
- `decode()` requires the payload length to be exactly `22 + nPins·6`; any mismatch (or a different class/ID, or fewer than 22 bytes) returns `false` with `out` value-initialised.
- Capacity overflow: when `nPins` exceeds `MaxPins`, the first `MaxPins` blocks are stored, the rest are skipped, `decode()` still succeeds and `n_pins` reports the on-wire count.
- `encode()` writes the count from the vector's `size()`; the stored count field is ignored.
- Reserved bytes are skipped on decode and written as zero on encode.
- `hw_version` holds the characters up to the first NUL (all 10 when unterminated); encode pads with NULs to 10 bytes.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Thread-safety: none; not thread-safe. Value type; copy freely.
