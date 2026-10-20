# UBX RXM-RLM

## Overview

`rxm_rlm.hpp` models UBX-RXM-RLM (0x02 0x59), the output reporting the contents of any Galileo Search and Rescue (SAR) Return Link Message detected by the receiver. Two layouts share the class/ID and are told apart by payload byte 1 (`type`):

- `rxm_rlm_short`: short-RLM report, `type` 0x01, 16 bytes, 16-bit parameters.
- `rxm_rlm_long`: long-RLM report, `type` 0x02, 28 bytes, 96-bit parameters.

Both layouts are identical apart from the size of `params`, so they are aliases of one template, `basic_rxm_rlm<Type, ParamBytes, PayloadLength>`, built on [`fixed_message`](detail/fixed_message.md). Payload byte 0 is the fixed message version (0x00), so the discriminator is byte 1 and [`typed_message`](detail/typed_message.md) (which checks byte 0) does not apply; `basic_rxm_rlm` narrows `matches()` itself.

## Header

`#include "castle_ext/protocols/ubx/messages/rxm_rlm.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

All names live in `castle::protocols::ubx::messages`.

### Types

| Type | Description |
| --- | --- |
| `template <uint8_t Type, castle::size_type ParamBytes, castle::size_type PayloadLength> struct basic_rxm_rlm` | Shared layout; `fixed_message<basic_rxm_rlm<...>, UBX_CLASS_RXM, UBX_ID_RXM_RLM, PayloadLength>`. `Type` is the value of payload byte 1, `ParamBytes` the size of `params`, `PayloadLength` the exact payload length. `static_assert(PayloadLength > 13 + ParamBytes)` guarantees at least one trailing reserved byte. |
| `rxm_rlm_short` | `basic_rxm_rlm<RXM_RLM_TYPE_SHORT, 2U, 16U>`: short-RLM report. |
| `rxm_rlm_long` | `basic_rxm_rlm<RXM_RLM_TYPE_LONG, 12U, 28U>`: long-RLM report. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `RXM_RLM_TYPE_SHORT` | `0x01U` | `type` byte of the short-RLM report. |
| `RXM_RLM_TYPE_LONG` | `0x02U` | `type` byte of the long-RLM report. |
| `RXM_RLM_VERSION` | `0x00U` | Message version byte of both layouts. |
| `msg_class` / `msg_id` | `UBX_CLASS_RXM` (`0x02`) / `UBX_ID_RXM_RLM` (`0x59`) | Class/ID, inherited from `fixed_message`. |
| `payload_length` | `16U` (short) / `28U` (long) | Exact payload length, inherited from `fixed_message`. |
| `message_type` | `Type` (`0x01` short / `0x02` long) | Discriminating payload byte. |
| `type_offset` | `1U` | Payload offset of the `type` byte. |
| `BEACON_ID_MASK` | `0x0FFFFFFFFFFFFFFFULL` | Mask of the 60 significant bits returned by `beacon_id()`. |

### Fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| (version) | `uint8_t` | 0 | Not a member: written as `RXM_RLM_VERSION`, re-validated on decode. |
| (type) | `uint8_t` | 1 | Not a member: written as `Type`, re-validated on decode. |
| `sv_id` | `uint8_t` | 2 | Transmitting satellite (see Satellite Numbering). Byte 3 is reserved. |
| `beacon` | `castle::container::array<uint8_t, 8U>` | 4..11 | Beacon identifier (60 bits), most significant byte first; the top four bits of the first byte are zero. |
| `message` | `uint8_t` | 12 | Message code (4 bits). |
| `params` | `castle::container::array<uint8_t, ParamBytes>` | 13.. | Parameters, most significant byte first: 2 bytes (16 bits) short, 12 bytes (96 bits) long. |

Trailing reserved bytes: 15 (short), 25..27 (long).

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view& raw)` | `true` only for class/ID 0x02 0x59, `payload_length() == PayloadLength`, and `payload[1] == Type`. The byte is read only after the length check succeeds. |
| `static bool decode(const message_view& raw, basic_rxm_rlm& out)` | Inherited from `fixed_message`: requires class/ID and the exact length, then the layout re-validates the version byte (0x00) and the type byte. On failure `out` is value-initialised and `false` is returned. |
| `void encode(payload_writer& writer) const` | Inherited from `fixed_message`; writes the version, the type byte, the fields and zero reserved bytes. Used by `encode_message()` (loop-back). |
| `uint64_t beacon_id() const` | The 60-bit beacon identifier assembled big-endian from `beacon`, masked with `BEACON_ID_MASK`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/rxm_rlm.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    if (rxm_rlm_short::matches(raw))
    {
        rxm_rlm_short rlm{};
        if (rxm_rlm_short::decode(raw, rlm))
        {
            const uint64_t beacon = rlm.beacon_id();
            // rlm.message, rlm.params[0], rlm.params[1] ...
        }
    }
    else if (rxm_rlm_long::matches(raw))
    {
        rxm_rlm_long rlm{};
        if (rxm_rlm_long::decode(raw, rlm))
        {
            // rlm.params holds 12 bytes
        }
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/rxm_rlm.cpp` for both layouts, encode, decode and `beacon_id()`.

## Constraints & Notes

- Length validation: each layout is exact-length (16 or 28 bytes); `decode()` rejects any other length, so a frame on 0x02 0x59 decodes as at most one of the two.
- Discriminator: payload byte 1, not byte 0 (byte 0 is the version 0x00). That is why `basic_rxm_rlm` narrows `matches()` itself instead of deriving from `typed_message`. `decode()` re-validates both the version and the type byte through the layout's `constant()` checks; a mismatch returns `false`.
- `beacon` and `params` are kept as raw big-endian byte arrays (the spec's transmission order); only `beacon_id()` assembles an integer.
- No heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs. The decoded model owns its data.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.
