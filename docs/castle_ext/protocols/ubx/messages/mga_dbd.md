# UBX MGA-DBD

## Overview

Models UBX-MGA-DBD (0x13 0x80) navigation database dump entries: 12 reserved bytes followed by a free-length, firmware-specific `data` section. The receiver sends them in response to the empty-payload poll request (`encode_poll<mga_dbd<>>`) and ends the dump with a UBX-MGA-ACK carrying the entry count. Entries are meant to be stored by the host and sent back to the same receiver later.

## Header

`#include "castle_ext/protocols/ubx/messages/mga_dbd.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/vector.hpp`](../../../../container/vector.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/repeated_group.hpp`](detail/repeated_group.md)
- `stdint.h`

## Public API

| Type / constant | Value | Description |
| --- | --- | --- |
| `template <castle::size_type MaxData = 164U> struct mga_dbd` | - | Database entry (spec 3.13.3.2). 164 bytes is the spec maximum payload, so the default never truncates. |
| `mga_dbd::msg_class` / `msg_id` | `UBX_CLASS_MGA`, `UBX_ID_MGA_DBD` | Wire constants. |
| `mga_dbd::header_length` | `12U` | Reserved prefix before `data`. |

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | True for class 0x13, ID 0x80 with at least 12 payload bytes; the empty poll request does not match. |
| `static bool decode(const message_view&, mga_dbd&)` | Skips the 12 reserved bytes and stores up to `MaxData` data bytes (`detail::read_tail`); the rest is skipped. `false` (and `out` reset) for another class/ID or a payload under 12 bytes. |
| `void encode(payload_writer&) const` | Writes 12 zero bytes then every stored byte (`detail::write_tail`); payload length is `12 + data.size()`. |
| `encode_poll<mga_dbd<>>(buf, cap, written)` | Poll request (spec 3.13.3.1): empty payload, no struct. |

### `mga_dbd<MaxData>` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `data` | `castle::container::vector<uint8_t, MaxData>` | `{}` | Firmware-specific entry bytes (at most `MaxData`; the rest is skipped). |

### `mga_dbd<MaxData>` constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `msg_class` | `uint8_t` | `UBX_CLASS_MGA` | UBX class byte. |
| `msg_id` | `uint8_t` | `UBX_ID_MGA_DBD` | UBX message ID byte. |
| `header_length` | `castle::size_type` | `12U` | Reserved bytes before `data`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mga_dbd.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

uint8_t poll[UBX_FRAME_OVERHEAD];
castle::size_type written = 0U;
castle::status result = encode_poll<mga_dbd<>>(poll, sizeof(poll), written);

void on_frame(const message_view& raw) {
    mga_dbd<> entry{};
    if (mga_dbd<>::decode(raw, entry)) {
        // store entry.data (entry.data.size() bytes) for later upload
    }
}
// Full walkthrough: samples/castle_ext/protocols/ubx/messages/mga_dbd.cpp
```

## Constraints & Notes

- Value types: every field lives inline in the struct; no heap allocation, no exceptions, no RTTI.
- Not thread-safe: a model is a plain value; synchronise externally if one instance is shared.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Capacity: `data` is a `castle::container::vector<uint8_t, MaxData>`; bytes beyond `MaxData` are dropped (decode still succeeds), so pick `MaxData >= 152` to round-trip every entry losslessly.
- The data length is implied by the payload length; there is no count field.
- `mga_dbd` itself is large (`MaxData` bytes inline); avoid placing many instances on small stacks.
