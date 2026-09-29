# UBX configuration

## Overview
`ubx_config.hpp` provides deterministic, header-only helpers for building and decoding u-blox UBX configuration transactions based on the UBX-CFG-VALSET and UBX-CFG-VALGET messages. It depends only on ../ubx/ubx.md and reuses its frame encoding, little-endian accessors, checksum handling, and UBX CFG key-size decoding helpers.

The module provides:
- Strongly typed configuration values (`config_value`);
- Key/value configuration entries (`config_entry`);
- UBX-CFG-VALSET frame generation for writing configuration values;
- UBX-CFG-VALGET frame generation for polling configuration values;
- UBX-CFG-VALGET response parsing into caller-provided storage;
- Configuration layer constants and helpers.

No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers are used.

## Header
`#include "castle_ext/protocols/ubx/ubx_config.hpp"`

## Dependencies
- ../../../core/compiler.md
- ../../../core/types.md
- ../../../error/status.md
- [`/../../container/array.md
- ../../../container/array_view.md
- ../ubx/ubx.md

## Public API

| API | Description |
| --- | --- |
| `config_layer` | UBX configuration storage layers (`ram`, `bbr`, `flash`). |
| `operator|(config_layer, config_layer)` | Combines VALSET layer flags into a raw layer bitmask. |
| `UBX_CFG_VALGET_LAYER_RAM` | Query the RAM layer in VALGET requests. |
| `UBX_CFG_VALGET_LAYER_BBR` | Query the battery-backed RAM layer in VALGET requests. |
| `UBX_CFG_VALGET_LAYER_FLASH` | Query the Flash layer in VALGET requests. |
| `UBX_CFG_VALGET_LAYER_DEFAULT` | Query the default configuration source layer. |
| `config_value` | Strongly typed storage for one UBX configuration value. |
| `config_entry` | One UBX key/value configuration entry. |
| `write_config_value(value, size, output)` | Encodes a configuration value into little-endian binary form. |
| `read_config_value(data, size)` | Decodes a configuration value from little-endian binary form. |
| `build_valset_frame(entries, layers, output, capacity, written)` | Encodes a complete UBX-CFG-VALSET frame. |
| `build_valget_frame(keys, layer, position, output, capacity, written)` | Encodes a complete UBX-CFG-VALGET poll frame. |
| `parse_valget_response(payload, entries, capacity)` | Decodes key/value pairs from a UBX-CFG-VALGET response payload. |

## Configuration Layers

### `config_layer`

Represents UBX configuration storage layers for VALSET operations.

```cpp
enum class config_layer : uint8_t
{
    ram   = 0x01U,
    bbr   = 0x02U,
    flash = 0x04U
};
```

Multiple layers may be combined:

```cpp
uint8_t layers =
    config_layer::ram
    | config_layer::flash;
```

### VALGET Source Layers

Unlike VALSET, UBX-CFG-VALGET uses a source-layer selector rather than a bitmask.

| Constant | Meaning |
| --- | --- |
| `UBX_CFG_VALGET_LAYER_RAM` | Query RAM configuration. |
| `UBX_CFG_VALGET_LAYER_BBR` | Query battery-backed RAM. |
| `UBX_CFG_VALGET_LAYER_FLASH` | Query Flash configuration. |
| `UBX_CFG_VALGET_LAYER_DEFAULT` | Query the receiver's default configuration source. |

## Configuration Values

### `config_value`

`config_value` is a compact, strongly typed wrapper that stores a configuration value as a raw 64-bit bit pattern.

Supported constructors:

```cpp
config_value(bool);
config_value(uint8_t);
config_value(uint16_t);
config_value(uint32_t);
config_value(uint64_t);

config_value(int8_t);
config_value(int16_t);
config_value(int32_t);
config_value(int64_t);
```

Supported accessors:

```cpp
as_bool();
as_u8();
as_u16();
as_u32();
as_u64();

as_i8();
as_i16();
as_i32();
as_i64();
```

Signed integers are stored using their native two's-complement bit pattern.

### `config_entry`

Represents one configuration key/value pair.

```cpp
struct config_entry
{
    uint32_t key_id;
    config_value value;
};
```

The encoded wire size of `value` is determined from the key's UBX size code using `ubx::value_byte_size()` rather than from the `config_value` type itself.

## Usage Example

See `samples/castle_ext/protocols/ubx/ubx_config.cpp`.

```cpp
#include "castle_ext/protocols/ubx/ubx_config.hpp"

using namespace castle_ext::protocols::ubx::config;

config_entry entries[1] =
{
    {0x30210001U, config_value(static_cast<uint16_t>(1000U))}
};

uint8_t frame[64U];
castle::size_type written = 0U;

build_valset_frame(
    castle::container::array_view<const config_entry>(entries, 1U),
    static_cast<uint8_t>(config_layer::ram),
    frame,
    sizeof(frame),
    written);
```

## Building a VALSET Request

`build_valset_frame()` creates a complete UBX-CFG-VALSET write request.

```cpp
using namespace castle_ext::protocols::ubx::config;

config_entry entries[2] =
{
    {0x30210001U, config_value(static_cast<uint16_t>(1000U))},
    {0x1031001FU, config_value(true)}
};

uint8_t frame[128U];
castle::size_type written = 0U;

build_valset_frame(
    castle::container::array_view<const config_entry>(entries, 2U),
    static_cast<uint8_t>(config_layer::ram | config_layer::flash),
    frame,
    sizeof(frame),
    written);
```

The generated payload layout is:

```text
[VERSION]
[LAYERS]
[RESERVED0]
[RESERVED1]
[KEY0][VALUE0]
[KEY1][VALUE1]
...
```

The complete UBX frame is then produced using `ubx::encode_frame()`.

## Building a VALGET Poll Request

`build_valget_frame()` creates a UBX-CFG-VALGET poll request.

```cpp
using namespace castle_ext::protocols::ubx::config;

uint32_t keys[2] =
{
    0x30210001U,
    0x1031001FU
};

uint8_t frame[64U];
castle::size_type written = 0U;

build_valget_frame(
    castle::container::array_view<const uint32_t>(keys, 2U),
    UBX_CFG_VALGET_LAYER_RAM,
    0U,
    frame,
    sizeof(frame),
    written);
```

The generated payload layout is:

```text
[VERSION]
[LAYER]
[POSITION_LO]
[POSITION_HI]
[KEY0]
[KEY1]
...
```

The `position` field allows callers to request large responses in chunks.

## Parsing a VALGET Response

`parse_valget_response()` decodes a UBX-CFG-VALGET response payload into caller-provided storage.

```cpp
using namespace castle_ext::protocols::ubx;
using namespace castle_ext::protocols::ubx::config;

message_view view;

if (decode_frame(frame_view, view)
    && view.is(UBX_CLASS_CFG, UBX_ID_CFG_VALGET))
{
    config_entry entries[16U];

    castle::size_type count =
        parse_valget_response(
            view.payload,
            entries,
            16U);

    for (castle::size_type i = 0U; i < count; ++i)
    {
        uint32_t key = entries[i].key_id;
        config_value value = entries[i].value;
    }
}
```

The response payload format is:

```text
[VERSION]
[LAYER]
[POSITION_LO]
[POSITION_HI]
[KEY0][VALUE0]
[KEY1][VALUE1]
...
```

Entry decoding stops safely at:
- End of payload;
- Full caller storage;
- Unrecognized key size code;
- Malformed or truncated key/value pair.

## Value Encoding Helpers

### `write_config_value()`

Writes a value using the UBX key's encoded width.

Supported widths:

| Value Size | Encoded Type |
| --- | --- |
| 1 | `uint8_t` |
| 2 | `uint16_t` |
| 4 | `uint32_t` |
| 8 | `uint64_t` |

Returns `false` for unsupported widths.

### `read_config_value()`

Performs the inverse operation and returns a decoded `config_value`.

Unsupported widths return a default-constructed (zeroed) value.

## Status Codes

Both `build_valset_frame()` and `build_valget_frame()` return `castle::status`.

| Status | Meaning |
| --- | --- |
| `castle::status::ok` | Frame generated successfully. |
| `castle::status::invalid_argument` | Invalid arguments, empty key/entry list, null output pointer, or invalid key size code. |
| `castle::status::full` | Scratch payload or output buffer capacity is insufficient. |

## Constraints & Notes

- No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers.
- All APIs operate on caller-owned buffers and storage.
- Frame generation ultimately reuses `ubx::encode_frame()`.
- Configuration values are stored as raw bit patterns in a `uint64_t`.
- Actual wire width is determined from the key ID's size code via `ubx::value_byte_size()`.
- `build_valset_frame()` rejects keys with unknown or unsupported size codes.
- `build_valget_frame()` requires at least one key ID.
- `parse_valget_response()` never allocates memory and never writes beyond the provided entry array.
- Parsed entries reference copied key/value data and do not alias the original payload.
- Template parameter `MaxPayloadLen` controls the local scratch payload capacity and defaults to `UBX_SAFE_MAX_PAYLOAD_LEN`.
- This module builds and parses generic UBX configuration transactions only; it does not define symbolic configuration keys. Applications may define project-specific key constants and use them with `config_entry`.
- All functionality is fully header-only and suitable for deterministic embedded systems.
