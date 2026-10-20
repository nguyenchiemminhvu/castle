# UBX cfg_data

## Overview

UBX-CFG-VALSET, -VALGET and -VALDEL share one wire shape: a 4-byte header whose byte 0 is the message version (which also picks the layout), then either a list of `U4` key IDs or `cfgData` (a key ID followed by a value whose width comes from the key's size code). `cfg_data.hpp` defines that shape once:

- `cfg_message<Derived, MsgId, Version>`: a CRTP base that derives `matches()`, `decode()` and `encode()` from one `layout()`, like [`fixed_message`](fixed_message.md), but for a variable-length payload identified by its version byte;
- `cfg_keys()` / `cfg_data()`: layout verbs for the two kinds of body;
- `cfg_transaction_action` / `cfg_transaction_field`: the `transaction` byte used by the version 1 VALSET and VALDEL layouts.

## Header

`#include "castle_ext/protocols/ubx/messages/detail/cfg_data.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../../core/types.md)
- [`castle/container/vector.hpp`](../../../../../container/vector.md)
- [`castle/error/status.hpp`](../../../../../error/status.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../../ubx.md) (`value_byte_size()`)
- [`castle_ext/protocols/ubx/ubx_config.hpp`](../../ubx_config.md) (`config::config_value`)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../../payload_writer.md)

## Public API

### Types and constants (namespace `castle::protocols::ubx::messages`)

| Name | Description |
| --- | --- |
| `CFG_MAX_KEYS` | `64U`: the spec's limit on keys or key/value pairs in one message. Every `vector` capacity passed to the helpers is checked against it with a `static_assert`. |
| `enum class cfg_transaction_action : uint8_t` | `transactionless = 0`, `start = 1`, `ongoing = 2`, `apply = 3`. |
| `cfg_transaction_field` | Mixin holding `uint8_t transaction`, with `TRANSACTION_ACTION = 0x03` (bits 1..0), `action()` and `set_action(cfg_transaction_action)` (keeps the other bits). |

### `detail::cfg_message<Derived, MsgId, Version>`

| Member | Description |
| --- | --- |
| `msg_class` / `msg_id` / `version` / `header_length` | `UBX_CLASS_CFG`, `MsgId`, `Version`, `4U`. |
| `static bool matches(const message_view&)` | True for class CFG / `MsgId`, payload of at least 4 bytes, and byte 0 equal to `Version`. Two layouts on one ID therefore never both match, and versions the spec does not define match neither. |
| `static bool decode(const message_view&, Derived&)` | Checks `matches()`, reads the version byte and then `Derived::layout`. On any failure (wrong ID or version, partial key, unknown value size, truncated value) returns `false` and value-initialises `out`. |
| `void encode(payload_writer&) const` | Writes `Version` and then `Derived::layout`. |

`Derived::layout()` describes payload bytes 1 and up. The base handles byte 0.

### Layout verbs (namespace `detail`)

| Function | Description |
| --- | --- |
| `cfg_keys(payload_reader&, vector<uint32_t, N>&)` | Clears the vector and reads every remaining `U4` key. Keys past `N` are read but not stored. A trailing partial key latches a reader failure. |
| `cfg_keys(payload_writer&, const vector<uint32_t, N>&)` | Writes every stored key. |
| `cfg_data(payload_reader&, vector<Entry, N>&)` | Clears the vector and reads every remaining key/value pair, each value `value_byte_size(key)` bytes wide. An unknown size code or a truncated pair latches a failure. Pairs past `N` are read but not stored. `Entry` needs `key_id` and a `value` constructible from `uint64_t` (`config::config_entry`, `cfg_valget_entry`). |
| `cfg_data(payload_writer&, const vector<Entry, N>&)` | Writes every pair. A key with an unknown size code latches `castle::status::invalid_argument`. |
| `uint64_t read_cfg_value(payload_reader&, uint8_t size)` | Reads a 1-, 2-, 4- or 8-byte little-endian value. |
| `uint64_t cfg_raw_value(uint64_t)` / `cfg_raw_value(const config::config_value&)` | Returns the raw bit pattern of either value representation. |

## Usage Example

```cpp
template <castle::size_type MaxKeys = CFG_MAX_KEYS>
struct cfg_valdel : detail::cfg_message<cfg_valdel<MaxKeys>, UBX_ID_CFG_VALDEL, 0x00U>, cfg_valdel_layers
{
    castle::container::vector<uint32_t, MaxKeys> keys{};

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) noexcept
    {
        stream.field(self.layers);
        stream.reserved(2U);
        detail::cfg_keys(stream, self.keys);
    }
};
```

See [`cfg_valset`](../cfg_valset.md), [`cfg_valget`](../cfg_valget.md) and [`cfg_valdel`](../cfg_valdel.md) for the concrete messages.

## Constraints & Notes

- No heap allocation. Storage is the message's fixed-capacity `vector`.
- `cfgData` cannot be walked past a key whose size code is unknown, so such a payload is rejected instead of being returned partly decoded.
- `decode()` handles a payload shorter than 4 bytes (including a null view) through `matches()`, before any reader is created.
- Complexity: O(payload length) to decode, O(stored entries) to encode.
- Thread-safety: none. These are stateless helpers over caller-owned values.
