# UBX SEC-CRC

## Overview

`sec_crc` models UBX-SEC-CRC (0x27 0x01), the output reporting the CRC-32 of a configuration or firmware region. UBX-SEC-CRC is **not part of UBX-19056845** (ZED-F9R interface description); this model follows the generic u-blox layout of its 8-byte prefix. It is a [`fixed_message`](detail/fixed_message.md) with a custom `decode()` that tolerates trailing bytes appended by other firmware.

## Header

`#include "castle_ext/protocols/ubx/messages/sec_crc.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

All names live in `castle::protocols::ubx::messages`.

### Types

| Type | Description |
| --- | --- |
| `sec_crc` | The message; `fixed_message<sec_crc, UBX_CLASS_SEC, UBX_ID_SEC_CRC, 8U>`. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `sec_crc::msg_class` / `msg_id` | `UBX_CLASS_SEC` (`0x27`) / `UBX_ID_SEC_CRC` (`0x01`) | Class/ID, inherited from `fixed_message`. |
| `sec_crc::payload_length` | `8U` | Length of the decoded prefix and of the encoded payload. |

### Fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | - | Message version. Offset 0. |
| `crc_type` | `uint8_t` | - | CRC type selector. Offset 1. |
| `reserved` | `uint16_t` | - | Reserved bytes 2..3, kept verbatim (decoded and re-encoded as-is, not zeroed). Offset 2. |
| `crc32` | `uint32_t` | - | CRC-32 value. Offset 4. |

### Functions

| Function | Description |
| --- | --- |
| `static bool sec_crc::matches(const message_view& raw)` | Class/ID check (inherited); any length. |
| `static bool sec_crc::decode(const message_view& raw, sec_crc& out)` | Requires class/ID and at least 8 payload bytes; decodes the first 8 bytes and ignores the rest. On failure `out` is value-initialised and `false` is returned. |
| `void sec_crc::encode(payload_writer& writer) const` | Inherited; writes exactly 8 bytes. Used by `encode_message()` (loop-back). |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/sec_crc.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    sec_crc crc{};
    if (!sec_crc::decode(raw, crc))
    {
        return;
    }
    // crc.crc_type selects the region, crc.crc32 is its CRC-32
}
```

See `samples/castle_ext/protocols/ubx/messages/sec_crc.cpp` for encode, decode and the trailing-byte tolerance.

## Constraints & Notes

- Not part of UBX-19056845 (ZED-F9R): the layout is the generic u-blox 8-byte prefix; `version` and `crc_type` values are stored, not checked.
- Length validation: `decode()` accepts any payload of length >= 8, decodes the 8-byte prefix and ignores extra bytes; shorter payloads (or a different class/ID) return `false` with `out` value-initialised.
- `encode()` always writes exactly 8 bytes, so a decoded frame with a tail re-encodes without it. `encode_message()` returns `castle::status::full` when the output buffer is smaller than the 16-byte frame.
- Field names are unchanged from the previous version (`version`, `crc_type`, `reserved`, `crc32`).
- No heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs. The decoded model owns its data.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.
