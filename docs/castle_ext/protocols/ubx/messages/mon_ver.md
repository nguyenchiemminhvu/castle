# UBX MON-VER

## Overview

Model of UBX-MON-VER (0x0a 0x04), the receiver and software version report (polled). The payload is `swVersion` CH[30] and `hwVersion` CH[10], followed by `[0..n]` CH[30] extension strings (`40 + [0..n]·30`). The extension count is implied by the payload length. Strings are stored as fixed-capacity `castle::container::string` with the NUL padding stripped. Request the message with `encode_poll<mon_ver<>>()`.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_ver.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/types.hpp](../../../../core/types.md)
- [castle/container/string.hpp](../../../../container/string.md)
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
| `mon_ver<MaxExtensions = 10>` | Message model; stores up to `MaxExtensions` extension strings. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `msg_class` / `msg_id` | `0x0A` / `0x04` | UBX class / ID. |
| `sw_version_len` / `hw_version_len` / `extension_len` | `30` / `10` / `30` | Wire widths of the CH[] fields. |
| `header_length` | `40` | `swVersion` + `hwVersion`. |
| `block_length` | `30` | Extension size. |

### Fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `sw_version` | `castle::container::string<30>` | 0 | Software version, up to the first NUL. |
| `hw_version` | `castle::container::string<10>` | 30 | Hardware version, up to the first NUL. |
| `extensions` | `castle::container::vector<castle::container::string<30>, MaxExtensions>` | 40 + n·30 | Extension strings in wire order; `extensions.size()` is the number stored. |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | `true` for class `0x0A` / ID `0x04`. |
| `static bool decode(const message_view&, mon_ver&)` | `false` for wrong class/ID or a length that is not `40 + n·30`. |
| `void encode(payload_writer&) const` | Writes all strings, NUL-padded to their wire widths. |


## Usage Example

```cpp
using namespace castle::protocols::ubx;
using castle::protocols::ubx::messages::mon_ver;
using castle::container::string_view;

void on_message(const message_view& raw) {
    mon_ver<> version{};
    if (mon_ver<>::decode(raw, version)) {
        for (const auto& extension : version.extensions) {
            if (extension.view().starts_with(string_view("PROTVER="))) { /* ... */ }
        }
    }
}
```

The full, runnable example is `samples/castle_ext/protocols/ubx/messages/mon_ver.cpp`.

## Constraints & Notes

- Payload length must be exactly `40 + n·30` (`implied_group_count`).
- Extensions beyond `MaxExtensions` are skipped. A CH[] field that uses its full width without a NUL is kept whole (`size() == width`).
- API change from the previous version: `sw_version` / `hw_version` were `char[30]` / `char[10]` and `extensions` was `array<array<char,30>, N>` with a separate `extension_count`. They are now `string<30>` / `string<10>` and a `vector<string<30>, N>`; use `extensions.size()` instead of `extension_count`.
- No heap allocation: every member lives in the value itself (fixed-capacity Castle containers only); no exceptions, no RTTI.
- `decode()` sets `out` to a value-initialised object first and resets it again on any failure, so callers never observe partial state.
- `message_view` is non-owning; its payload must stay valid during `decode()`. Decoded values own their data and may outlive the frame.
- Thread-safety: none. Values are plain value types; concurrent access to one instance needs external synchronisation.
