# UBX configuration

## Overview
`ubx_config.hpp` is the shared vocabulary for u-blox configuration: storage layers, strongly typed configuration values and key/value entries. It contains no wire encoding. The CFG message models own the payload formats and consume these types:

- [`cfg_valset`](messages/cfg_valset.md): UBX-CFG-VALSET (write values, with or without a transaction);
- [`cfg_valget`](messages/cfg_valget.md): UBX-CFG-VALGET (`cfg_valget_poll` request, `cfg_valget` response);
- [`cfg_valdel`](messages/cfg_valdel.md): UBX-CFG-VALDEL (delete saved values).

A value's wire width is never stored with the value; it always follows the key's size code (`castle::protocols::ubx::value_byte_size()` in [`ubx.hpp`](ubx.md)).

## Header
`#include "castle_ext/protocols/ubx/ubx_config.hpp"`

## Dependencies
- [`core/compiler.hpp`](../../../core/compiler.md)
- [`core/types.hpp`](../../../core/types.md)
- [`ubx.hpp`](ubx.md)

## Public API

All names live in `castle::protocols::ubx::config`.

| API | Description |
| --- | --- |
| `config_layer` | `enum class : uint8_t` storage-layer bit flags: `ram = 0x01`, `bbr = 0x02`, `flash = 0x04`. |
| `operator|(config_layer, config_layer)` | Combines two layers into a raw `uint8_t` layer bitmask for VALSET / VALDEL. |
| `UBX_CFG_VALGET_LAYER_RAM` / `_BBR` / `_FLASH` / `_DEFAULT` | VALGET source-layer **selector** values `0` / `1` / `2` / `7`. These are not bit flags. |
| `config_value` | `constexpr` value type holding one configuration value as its raw 64-bit pattern. |
| `config_entry` | `{ uint32_t key_id; config_value value; }`. |

### `config_value`

| Member | Description |
| --- | --- |
| `config_value()` | Zero value. |
| `explicit config_value(T)` | For `bool`, `uint8_t` ... `uint64_t`, `int8_t` ... `int64_t`. Signed values are stored as their two's-complement bit pattern at their own width (no sign extension into the upper bits). |
| `as_bool()`, `as_u8()` ... `as_u64()`, `as_i8()` ... `as_i64()` | Reinterprets the low bits at the requested width. |
| `operator==`, `operator!=` | Compares the raw bit patterns. |
| `raw` | Public `uint64_t` bit pattern. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/cfg_valset.hpp"

namespace ubx = castle::protocols::ubx;

ubx::messages::cfg_valset<> set{};
set.layers = ubx::config::config_layer::ram | ubx::config::config_layer::flash;
static_cast<void>(set.entries.push_back(
    ubx::config::config_entry{0x30210001U, ubx::config::config_value(static_cast<uint16_t>(1000U))})); // CFG-RATE-MEAS

uint8_t frame[64U];
castle::size_type written = 0U;
if (castle::succeeded(ubx::encode_message(set, frame, sizeof(frame), written)))
{
    // Send frame[0..written), then wait for UBX-ACK-ACK / UBX-ACK-NAK.
}
```

See [`samples/castle_ext/protocols/ubx/ubx_config.cpp`](../../../../samples/castle_ext/protocols/ubx/ubx_config.cpp).

## Constraints & Notes
- Header-only value types; no heap allocation, exceptions, RTTI, virtual dispatch, or STL containers.
- All types are trivially copyable and `constexpr`-constructible.
- `config_value` does not know its wire width. Encoding a key with an unknown size code is rejected by the message models (`castle::status::invalid_argument`).
- VALSET/VALDEL `layers` is a bitmask built with `operator|`. VALGET `layer` is a single selector: use the `UBX_CFG_VALGET_LAYER_*` constants, never `config_layer`.
- Removed in this release: `build_valset_frame()`, `build_valget_frame()`, `parse_valget_response()`, `write_config_value()` and `read_config_value()`. Use `encode_message(messages::cfg_valset<>{...})`, `encode_message(messages::cfg_valget_poll<>{...})` and `messages::cfg_valget<>::decode()` instead.
