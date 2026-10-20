# UBX SEC-UNIQID

## Overview

`sec_uniqid` models UBX-SEC-UNIQID (0x27 0x03), the output carrying the receiver's unique 40-bit (5-byte) chip identifier. Request it with an empty poll; the receiver answers with this 9-byte payload. It is a [`fixed_message`](detail/fixed_message.md) and implements UBX-19056845 exactly.

## Header

`#include "castle_ext/protocols/ubx/messages/sec_uniqid.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

All names live in `castle::protocols::ubx::messages`.

### Types

| Type | Description |
| --- | --- |
| `sec_uniqid` | The message; `fixed_message<sec_uniqid, UBX_CLASS_SEC, UBX_ID_SEC_UNIQID, 9U>`. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `sec_uniqid::msg_class` / `msg_id` | `UBX_CLASS_SEC` (`0x27`) / `UBX_ID_SEC_UNIQID` (`0x03`) | Class/ID, inherited from `fixed_message`. |
| `sec_uniqid::payload_length` | `9U` | Exact payload length. |

### Fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | - | Message version (0x01 for this version). Offset 0, followed by 3 reserved bytes. |
| `unique_id` | `castle::container::array<uint8_t, 5U>` | - | Unique chip ID, in wire order. Offset 4. |

### Functions

| Function | Description |
| --- | --- |
| `static bool sec_uniqid::matches(const message_view& raw)` | Class/ID check (inherited). |
| `static bool sec_uniqid::decode(const message_view& raw, sec_uniqid& out)` | Inherited: requires class/ID and exactly 9 payload bytes; on failure `out` is value-initialised and `false` is returned. |
| `void sec_uniqid::encode(payload_writer& writer) const` | Inherited; writes zero reserved bytes. Used by `encode_message()` (loop-back). |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/sec_uniqid.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

// Request the chip ID (empty payload).
uint8_t poll[UBX_FRAME_OVERHEAD];
castle::size_type written = 0U;
const castle::status result = encode_poll<sec_uniqid>(poll, sizeof(poll), written);

void handle(const message_view& raw)
{
    sec_uniqid id{};
    if (sec_uniqid::decode(raw, id))
    {
        // id.unique_id[0] .. id.unique_id[4]
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/sec_uniqid.cpp` for encode, decode and field access.

## Constraints & Notes

- Implements UBX-19056845 exactly (9-byte payload). Length validation: `decode()` requires exactly 9 payload bytes; shorter or longer payloads (or a different class/ID) return `false` with `out` value-initialised. Firmware that appends bytes to this message produces frames this model rejects.
- `version` is stored, not checked: the decoder accepts any version byte.
- `encode_message()` returns `castle::status::full` when the output buffer is smaller than the 17-byte frame.
- No heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs. The decoded model owns its data.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.
