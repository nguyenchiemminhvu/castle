# UBX MON-PATCH

## Overview

Model of UBX-MON-PATCH (0x0a 0x27), the installed patches report (polled). A 4-byte header is followed by `nEntries` 16-byte patch blocks (`4 + nEntries·16`). Only installed and enabled patches are reported.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_patch.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/types.hpp](../../../../core/types.md)
- [castle/container/vector.hpp](../../../../container/vector.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/repeated_group.hpp](detail/repeated_group.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_patch_location` | `enum class : uint8_t` for `patchInfo.location`: `efuse = 0`, `rom = 1`, `bbr = 2`, `file_system = 3`. |
| `mon_patch_entry` | One 16-byte patch block (`wire_size = 16`, `layout()`). |
| `mon_patch<MaxEntries = 8>` | Message model; stores up to `MaxEntries` entries. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `mon_patch::msg_class` / `msg_id` | `0x0A` / `0x27` | UBX class / ID. |
| `mon_patch::header_length` / `block_length` | `4` / `16` | Header and patch block sizes. |
| `mon_patch_entry::PATCH_INFO_ACTIVATED` | `0x1` | `patchInfo` bit 0: patch is active. |
| `mon_patch_entry::PATCH_INFO_LOCATION` | `0x6` | `patchInfo` bits 2..1: storage location. |
| `mon_patch_entry::PATCH_INFO_LOCATION_SHIFT` | `1` | Shift for `PATCH_INFO_LOCATION`. |

### `mon_patch` fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `version` | `uint16_t` | 0 | Message version (`0x0001`). |
| `n_entries` | `uint16_t` | 2 | On-wire patch count; may exceed `entries.size()`. Encode writes `entries.size()`. |
| `entries` | `castle::container::vector<mon_patch_entry, MaxEntries>` | 4 | Patch blocks in wire order. |

### `mon_patch_entry` fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `patch_info` | `uint32_t` | 4 + n·16 | Status bitfield (`PATCH_INFO_*`). |
| `comparator` | `uint32_t` | 8 + n·16 | Number of the comparator. |
| `patch_address` | `uint32_t` | 12 + n·16 | Address targeted by the patch. |
| `patch_data` | `uint32_t` | 16 + n·16 | Data inserted at `patch_address`. |

### Functions

| Function | Description |
| --- | --- |
| `static bool mon_patch::matches(const message_view&)` | `true` for class `0x0A` / ID `0x27`. |
| `static bool mon_patch::decode(const message_view&, mon_patch&)` | `false` for wrong class/ID or a length other than `4 + nEntries·16`. |
| `void mon_patch::encode(payload_writer&) const` | Writes the header (count from `entries.size()`) and every entry. |
| `bool mon_patch_entry::activated() const` | `PATCH_INFO_ACTIVATED` set. |
| `mon_patch_location mon_patch_entry::location() const` | Bits 2..1 of `patch_info`. |


## Usage Example

```cpp
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void on_message(const message_view& raw) {
    mon_patch<> patches{};
    if (mon_patch<>::decode(raw, patches)) {
        for (const mon_patch_entry& entry : patches.entries) {
            bool active_rom_patch = entry.activated() && (entry.location() == mon_patch_location::rom);
            (void)active_rom_patch;
        }
    }
}

// Poll: encode_poll<mon_patch<>>(frame, sizeof(frame), written);
```

The full, runnable example is `samples/castle_ext/protocols/ubx/messages/mon_patch.cpp`.

## Constraints & Notes

- Payload length must be exactly `4 + nEntries·16` (`has_group_length`).
- Entries beyond `MaxEntries` are skipped; `n_entries` still reports the on-wire count.
- No heap allocation: every member lives in the value itself (fixed-capacity Castle containers only); no exceptions, no RTTI.
- `decode()` sets `out` to a value-initialised object first and resets it again on any failure, so callers never observe partial state.
- `message_view` is non-owning; its payload must stay valid during `decode()`. Decoded values own their data and may outlive the frame.
- Thread-safety: none. Values are plain value types; concurrent access to one instance needs external synchronisation.
