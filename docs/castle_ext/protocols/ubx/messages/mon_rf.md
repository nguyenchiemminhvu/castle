# UBX MON-RF

## Overview

Model of UBX-MON-RF (class `0x0A`, ID `0x38`): a 4-byte header followed by one 24-byte RF block per supported band (`4 + nBlocks·24`), carrying jamming state, antenna supervisor status, noise, AGC and I/Q statistics. Also defines the jamming-state and antenna enumerations reused by `mon_hw.hpp`.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_rf.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/types.hpp](../../../../core/types.md)
- [castle/container/vector.hpp](../../../../container/vector.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/repeated_group.hpp](detail/repeated_group.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_jamming_state` | `enum class : uint8_t`: `unknown = 0` (or disabled), `ok = 1`, `warning = 2` (interference, fix OK), `critical = 3` (interference, no fix). |
| `mon_antenna_status` | `enum class : uint8_t`: `init = 0`, `dont_know = 1`, `ok = 2`, `short_circuit = 3` (`SHORT`), `open = 4`. |
| `mon_antenna_power` | `enum class : uint8_t`: `off = 0`, `on = 1`, `dont_know = 2`. |
| `mon_rf_block` | One 24-byte RF block. |
| `mon_rf<MaxBlocks = 2U>` | Message with up to `MaxBlocks` stored blocks. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `mon_rf::msg_class` / `msg_id` | `0x0A` / `0x38` | Class/ID. |
| `mon_rf::header_length` | `4U` | Header bytes. |
| `mon_rf::block_length` | `24U` | Bytes per block (`mon_rf_block::wire_size`). |
| `mon_rf_block::BLOCK_ID_L1` | `0U` | L1 band. |
| `mon_rf_block::BLOCK_ID_L2_L5` | `1U` | L2 or L5 band (product dependent). |
| `mon_rf_block::FLAGS_JAMMING_STATE` | `0x03U` | `flags` bits 1..0. |

### `mon_rf` fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | 0 | Message version (`0x00`). |
| `n_blocks` | `uint8_t` | 1 | On-wire block count; may exceed `blocks.size()`. |
| — | reserved | 2..3 | |
| `blocks` | `castle::container::vector<mon_rf_block, MaxBlocks>` | 4 + n·24 | Stored blocks. |

### `mon_rf_block` fields (offset within block)

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `block_id` | `uint8_t` | 0 | RF block ID (`BLOCK_ID_*`). |
| `flags` | `uint8_t` (X1) | 1 | Bits 1..0 jamming state. |
| `ant_status` | `mon_antenna_status` | 2 | Antenna supervisor state. |
| `ant_power` | `mon_antenna_power` | 3 | Antenna power state. |
| `post_status` | `uint32_t` | 4 | POST status word. |
| — | reserved | 8..11 | |
| `noise_per_ms` | `uint16_t` | 12 | Noise level measured by the GPS core. |
| `agc_cnt` | `uint16_t` | 14 | AGC monitor (0..8191). |
| `jam_ind` | `uint8_t` | 16 | CW jamming indicator (0 none .. 255 strong). |
| `ofs_i` | `int8_t` | 17 | I-part imbalance (-128..127). |
| `mag_i` | `uint8_t` | 18 | I-part magnitude (0..255). |
| `ofs_q` | `int8_t` | 19 | Q-part imbalance (-128..127). |
| `mag_q` | `uint8_t` | 20 | Q-part magnitude (0..255). |
| — | reserved | 21..23 | |

### Functions

| Function | Description |
| --- | --- |
| `static bool mon_rf::matches(const message_view&)` | Class/ID check. |
| `static bool mon_rf::decode(const message_view&, mon_rf&)` | Validates `4 + nBlocks·24` exactly, then decodes. |
| `void mon_rf::encode(payload_writer&) const` | Writes header (`nBlocks = blocks.size()`) and every block. |
| `mon_jamming_state mon_rf_block::jamming_state() const` | `flags` bits 1..0. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mon_rf.hpp"

using namespace castle::protocols::ubx::messages;

mon_rf<> rf{};
if (mon_rf<>::decode(raw, rf))
{
    for (const mon_rf_block& block : rf.blocks)
    {
        if (block.ant_status == mon_antenna_status::short_circuit) { /* antenna short */ }
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/mon_rf.cpp`.

## Constraints & Notes

- Storage: `castle::container::vector<..., MaxBlocks>` (default `MaxBlocks = 2U`) lives inside the object; no heap allocation, no exceptions.
- `decode()` requires the payload length to be exactly `4 + nBlocks·24`; any mismatch (or a different class/ID, or fewer than 4 bytes) returns `false` with `out` value-initialised.
- Capacity overflow: when `nBlocks` exceeds `MaxBlocks`, the first `MaxBlocks` blocks are stored, the rest are skipped, `decode()` still succeeds and `n_blocks` reports the on-wire count.
- `encode()` writes the count from the vector's `size()`; the stored count field is ignored.
- Reserved bytes are skipped on decode and written as zero on encode.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Thread-safety: none; not thread-safe. Value type; copy freely.
