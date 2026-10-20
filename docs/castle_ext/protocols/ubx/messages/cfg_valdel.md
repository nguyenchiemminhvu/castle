# UBX CFG-VALDEL

## Overview

Typed models of UBX-CFG-VALDEL (class 0x06, ID 0x8c), the set message the host sends to delete saved configuration values from the BBR and/or Flash layer. The deleted items revert to their defaults once those layers are loaded into RAM. The receiver answers with UBX-ACK-ACK or UBX-ACK-NAK. Two layouts share the class/ID and are told apart by the version byte:

- `cfg_valdel` (version 0) is applied immediately.
- `cfg_valdel_transaction` (version 1) adds a `transaction` byte. A message with no keys only changes the transaction state.

Both carry up to 64 `U4` key IDs.

## Header

`#include "castle_ext/protocols/ubx/messages/cfg_valdel.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/vector.hpp`](../../../../container/vector.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/ubx_config.hpp`](../ubx_config.md) (`config::config_layer`)
- [`detail/cfg_data.hpp`](detail/cfg_data.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `cfg_valdel_layers` | Mixin holding `uint8_t layers` (default `LAYERS_BBR \| LAYERS_FLASH`) and its bit helpers. |
| `cfg_valdel<MaxKeys = 64>` | Version 0. Derives from `detail::cfg_message<..., UBX_ID_CFG_VALDEL, 0x00>` and `cfg_valdel_layers`. |
| `cfg_valdel_transaction<MaxKeys = 64>` | Version 1. Also derives from `cfg_transaction_field`. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `msg_class` / `msg_id` | `uint8_t` | `UBX_CLASS_CFG` / `UBX_ID_CFG_VALDEL` (0x8c) | UBX class/ID. |
| `version` | `uint8_t` | `0x00` / `0x01` | Payload byte 0. |
| `header_length` | `castle::size_type` | `4U` | Bytes before the keys. |
| `LAYERS_BBR` / `LAYERS_FLASH` | `uint8_t` | `0x02` / `0x04` | `layers` bits 1 and 2. RAM is not a VALDEL layer. |
| `TRANSACTION_ACTION` | `uint8_t` | `0x03` | `transaction` bits 1..0 (version 1 only). |

### Fields

| Offset | Field | Type | Description |
| --- | --- | --- | --- |
| 0 | (version) | `U1` | Written and checked by the base. |
| 1 | `layers` | `uint8_t` | Layers to delete from. |
| 2 | `transaction` (v1 only) | `uint8_t` | Transaction action in bits 1..0. In version 0 these two bytes are reserved. |
| 3 | - | reserved | Written as zero, ignored on decode. |
| 4 + n·4 | `keys` | `vector<uint32_t, MaxKeys>` | Key IDs to delete. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `bool bbr() const` / `flash() const` | `bool` | Layer bit tests. |
| `cfg_transaction_action action() const` / `void set_action(cfg_transaction_action)` | - | Version 1 transaction action. |
| `static bool matches(const message_view& raw)` | `bool` | Class/ID match, at least 4 bytes, and this layout's version byte. |
| `static bool decode(const message_view& raw, T& out)` | `bool` | Loop-back decode. Returns `false` and value-initialises `out` for a wrong version or a key list that is not a whole number of `U4` values. |
| `void encode(payload_writer& writer) const` | - | Writes the payload from `keys.size()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/cfg_valdel.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

castle::status forget_rate(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    cfg_valdel<1U> del{};
    del.layers = cfg_valdel<1U>::LAYERS_FLASH;
    static_cast<void>(del.keys.push_back(0x30210001U)); // CFG-RATE-MEAS
    return encode_message(del, frame, capacity, written);
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/cfg_valdel.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/cfg_valdel.cpp).

## Constraints & Notes

- Capacity: at most 64 keys per message (spec limit, enforced by `static_assert`). Keys past `MaxKeys` are consumed but not stored on decode.
- No heap allocation. All storage is inline in the value type.
- Thread-safety: none. Instances are plain values and are not thread-safe.
