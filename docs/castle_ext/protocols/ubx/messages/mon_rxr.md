# UBX MON-RXR

## Overview

Model of UBX-MON-RXR (class `0x0A`, ID `0x21`), the 1-byte output the receiver sends when it enters or leaves backup mode. Built on `detail::fixed_message`, so `matches()`, `decode()` and `encode()` all derive from one `layout()`.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_rxr.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/fixed_message.hpp](detail/fixed_message.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_rxr` | UBX-MON-RXR payload (`fixed_message<mon_rxr, UBX_CLASS_MON, UBX_ID_MON_RXR, 1U>`). |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `msg_class` / `msg_id` | `0x0A` / `0x21` | Inherited from `fixed_message`. |
| `payload_length` | `1U` | Exact payload length. |
| `FLAGS_AWAKE` | `0x01U` | `flags` bit 0 `awake`. |

### Fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `flags` | `uint8_t` (X1) | 0 | Receiver status flags. |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | Class/ID check. |
| `static bool decode(const message_view&, mon_rxr&)` | Exact-length decode; `out` reset on failure. |
| `void encode(payload_writer&) const` | Writes the payload (used by `encode_message()`). |
| `bool awake() const` | `true` when the receiver is awake (not in backup mode). |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mon_rxr.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::mon_rxr;

void on_message(const message_view& raw)
{
    mon_rxr status{};
    if (mon_rxr::decode(raw, status) && status.awake())
    {
        // receiver left backup mode
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/mon_rxr.cpp` for the full round trip.

## Constraints & Notes

- Value type: every field is stored in the object itself; no heap allocation, no exceptions.
- `decode()` rejects a different class/ID or any payload length other than the exact 1 bytes and leaves `out` value-initialised on failure.
- Reserved bytes are skipped on decode and written as zero on encode.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs.
- Thread-safety: none; not thread-safe. Use one instance per thread or synchronise externally.
- Output only: the receiver sends it spontaneously; there is no poll request.
