# UBX CFG-VALSET

## Overview

Typed models of UBX-CFG-VALSET (class 0x06, ID 0x8a), the set message the host sends to change configuration item values. The receiver answers with UBX-ACK-ACK or UBX-ACK-NAK. Two layouts share the class/ID and are told apart by the version byte:

- `cfg_valset` (version 0) is applied immediately.
- `cfg_valset_transaction` (version 1) adds a `transaction` byte so that several messages are applied atomically. A message with no entries only changes the transaction state.

Both carry up to 64 key/value pairs (`cfgData`). Each value is written with the width given by its key's size code.

## Header

`#include "castle_ext/protocols/ubx/messages/cfg_valset.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/vector.hpp`](../../../../container/vector.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md) (`value_byte_size()`)
- [`castle_ext/protocols/ubx/ubx_config.hpp`](../ubx_config.md) (`config::config_entry`, `config::config_value`, `config::config_layer`)
- [`detail/cfg_data.hpp`](detail/cfg_data.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `cfg_valset_layers` | Mixin holding `uint8_t layers` (default `LAYERS_RAM`) and its bit helpers. |
| `cfg_valset<MaxEntries = 64>` | Version 0. Derives from `detail::cfg_message<..., UBX_ID_CFG_VALSET, 0x00>` and `cfg_valset_layers`. |
| `cfg_valset_transaction<MaxEntries = 64>` | Version 1. Also derives from `cfg_transaction_field`. |
| `cfg_transaction_action` | `transactionless`, `start`, `ongoing`, `apply` (see [`cfg_data`](detail/cfg_data.md)). |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `msg_class` / `msg_id` | `uint8_t` | `UBX_CLASS_CFG` / `UBX_ID_CFG_VALSET` (0x8a) | UBX class/ID. |
| `version` | `uint8_t` | `0x00` / `0x01` | Payload byte 0. |
| `header_length` | `castle::size_type` | `4U` | Bytes before `cfgData`. |
| `LAYERS_RAM` / `LAYERS_BBR` / `LAYERS_FLASH` | `uint8_t` | `0x01` / `0x02` / `0x04` | `layers` bits 0..2, equal to `config::config_layer` values. |
| `TRANSACTION_ACTION` | `uint8_t` | `0x03` | `transaction` bits 1..0 (version 1 only). |

### Fields

| Offset | Field | Type | Description |
| --- | --- | --- | --- |
| 0 | (version) | `U1` | Written and checked by the base. |
| 1 | `layers` | `uint8_t` | Target layers bitmask. `config_layer::ram \| config_layer::bbr` produces this value. |
| 2 | `transaction` (v1 only) | `uint8_t` | Transaction action in bits 1..0. In version 0 these two bytes are reserved. |
| 3 | - | reserved | Written as zero, ignored on decode. |
| 4 + n | `entries` | `vector<config::config_entry, MaxEntries>` | Key ID (4 bytes) followed by a value of `value_byte_size(key_id)` bytes. |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `bool ram() const` / `bbr() const` / `flash() const` | `bool` | Layer bit tests. |
| `cfg_transaction_action action() const` / `void set_action(cfg_transaction_action)` | - | Version 1 transaction action. |
| `static bool matches(const message_view& raw)` | `bool` | Class/ID match, at least 4 bytes, and this layout's version byte. |
| `static bool decode(const message_view& raw, T& out)` | `bool` | Loop-back decode. Returns `false` and value-initialises `out` for a wrong version, a key with an unknown size code, a truncated value, or a partial key. |
| `void encode(payload_writer& writer) const` | - | Writes the payload. A key with an unknown size code latches `castle::status::invalid_argument`, which `encode_message()` returns with `written == 0`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/cfg_valset.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

castle::status set_rate(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    cfg_valset<1U> set{};
    set.layers = config::config_layer::ram | config::config_layer::flash;
    config::config_entry rate{};
    rate.key_id = 0x30210001U; // CFG-RATE-MEAS (U2, ms)
    rate.value = config::config_value(static_cast<uint16_t>(100U));
    static_cast<void>(set.entries.push_back(rate));
    return encode_message(set, frame, capacity, written);
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/cfg_valset.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/cfg_valset.cpp).

## Constraints & Notes

- Capacity: at most 64 pairs per message (spec limit, enforced by `static_assert`). A transaction can span several messages.
- The encoder does not validate key IDs beyond their size code. The receiver NAKs unknown keys.
- No heap allocation. The payload is written straight into the caller's frame buffer by `encode_message()`, with no scratch copy.
- Thread-safety: none. Instances are plain values and are not thread-safe.
