# UBX CFG-VALGET

## Overview

Typed models of UBX-CFG-VALGET (class 0x06, ID 0x8b), which reads configuration items. Two layouts share the class/ID and are told apart by the version byte:

- `cfg_valget_poll` (version 0) is the poll request the host sends. It names a source layer, a skip `position`, and up to 64 key IDs. A key can be a complete key, a group wild card (item part `0xffff`), or the all-groups wild card.
- `cfg_valget` (version 1) is the polled response from the receiver. It carries up to 64 key/value pairs (`cfgData`), and each value is as wide as its key's size code. A response with exactly 64 pairs may have more to read: poll again with `position` increased.

## Header

`#include "castle_ext/protocols/ubx/messages/cfg_valget.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/vector.hpp`](../../../../container/vector.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/ubx_config.hpp`](../ubx_config.md) (`UBX_CFG_VALGET_LAYER_*`)
- [`detail/cfg_data.hpp`](detail/cfg_data.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `cfg_valget_entry` | One response pair: `uint32_t key_id`, `uint64_t value` (zero-extended raw value). |
| `cfg_valget_poll<MaxKeys = 64>` | Poll request. Derives from `detail::cfg_message<..., UBX_ID_CFG_VALGET, 0x00>`. |
| `cfg_valget<MaxEntries = 64>` | Polled response. Derives from `detail::cfg_message<..., UBX_ID_CFG_VALGET, 0x01>`. |

### Constants (both models)

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `msg_class` | `uint8_t` | `UBX_CLASS_CFG` (0x06) | UBX class. |
| `msg_id` | `uint8_t` | `UBX_ID_CFG_VALGET` (0x8b) | UBX message ID. |
| `version` | `uint8_t` | poll `0x00`, response `0x01` | Payload byte 0. |
| `header_length` | `castle::size_type` | `4U` | Bytes before the keys or `cfgData`. |

### Fields

| Offset | Field | Type | Description |
| --- | --- | --- | --- |
| 0 | (version) | `U1` | Written and checked by the base. |
| 1 | `layer` | `uint8_t` | Layer: `config::UBX_CFG_VALGET_LAYER_RAM` (0), `_BBR` (1), `_FLASH` (2), `_DEFAULT` (7). |
| 2 | `position` | `uint16_t` | Poll: key values to skip. Response: number skipped, copied from the request. |
| 4 + n·4 | `keys` (poll) | `vector<uint32_t, MaxKeys>` | Key IDs to retrieve. |
| 4 + n | `entries` (response) | `vector<cfg_valget_entry, MaxEntries>` | Key ID (4 bytes) followed by a value of `value_byte_size(key_id)` bytes. |

### Functions (both models)

| Function | Returns | Description |
| --- | --- | --- |
| `static bool matches(const message_view& raw)` | `bool` | Class/ID match, at least 4 bytes, and this layout's version byte. A poll and a response never both match. |
| `static bool decode(const message_view& raw, T& out)` | `bool` | Decodes the header and body. Returns `false` and value-initialises `out` for a wrong version, a partial key, a key with an unknown size code, or a truncated value. |
| `void encode(payload_writer& writer) const` | - | Writes the payload from `keys.size()` / `entries.size()`. For a response key with an unknown size code it latches `castle::status::invalid_argument`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/cfg_valget.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

castle::status poll_rate(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    cfg_valget_poll<1U> poll{};
    poll.layer = config::UBX_CFG_VALGET_LAYER_RAM;
    static_cast<void>(poll.keys.push_back(0x30210001U)); // CFG-RATE-MEAS
    return encode_message(poll, frame, capacity, written);
}

void on_message(const message_view& raw)
{
    cfg_valget<> response{};
    if (cfg_valget<>::decode(raw, response))
    {
        for (const cfg_valget_entry& entry : response.entries) { static_cast<void>(entry); }
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/cfg_valget.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/cfg_valget.cpp).

## Constraints & Notes

- Capacity: at most 64 keys or pairs (spec limit, enforced by `static_assert`). Keys or pairs past `MaxKeys` / `MaxEntries` are consumed but not stored. Use `.size()` for the number stored.
- `cfgData` cannot be walked past a key with an unknown size code, so such a response is rejected instead of being returned partly decoded.
- No heap allocation. All storage is inline in the value type.
- `message_view` is non-owning. Its payload must stay valid while `decode()` runs.
- Thread-safety: none. Instances are plain values and are not thread-safe.
