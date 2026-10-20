# UBX typed_message

## Overview

Some UBX class/ID pairs carry several payload layouts and tell them apart by the first payload byte (`type`). Examples: UBX-MGA-GPS-EPH / -ALM / -HEALTH / -UTC / -IONO, the UBX-MGA-INI variants, and UBX-RXM-RLM short/long. `detail::typed_message` extends [`fixed_message`](fixed_message.md) for that case. Each layout is its own model, and `matches()` also checks the exact length and the type byte, so a decoder registry hands a frame only to the one model that can decode it.

## Header

`#include "castle_ext/protocols/ubx/messages/detail/typed_message.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../../core/types.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../../ubx.md)
- [`detail/fixed_message.hpp`](fixed_message.md)

## Public API

`template <typename Derived, uint8_t MsgClass, uint8_t MsgId, uint8_t Type, castle::size_type PayloadLength> struct typed_message : fixed_message<Derived, MsgClass, MsgId, PayloadLength>`

| Member | Description |
| --- | --- |
| `message_type` | `static constexpr uint8_t`, the discriminating payload byte 0 (`Type`). |
| `static bool matches(const message_view& raw)` | `true` only for this class/ID, `payload_length() == PayloadLength`, and `payload[0] == Type`. |
| `decode()`, `encode()`, `msg_class`, `msg_id`, `payload_length` | Inherited from `fixed_message`. |

`Derived::layout()` must start with `stream.constant(Type)`, so encoding writes the discriminator and decoding re-validates it. When the spec fixes the version byte too, follow with `stream.constant(version)`.

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/mga_gps.hpp"

using namespace castle::protocols::ubx::messages;

// One frame on 0x13 0x00 matches exactly one of these:
//   mga_gps_eph::matches(raw), mga_gps_alm::matches(raw), mga_gps_utc::matches(raw), ...
mga_gps_utc utc{};
if (mga_gps_utc::decode(raw, utc))
{
    // utc.utc_dt_ls ...
}
```

See [`mga_gps`](../mga_gps.md) for a complete multi-layout message.

## Constraints & Notes

- `matches()` reads `payload[0]` only after the length check succeeds, so it is safe on empty payloads.
- No heap allocation, exceptions, RTTI or virtual dispatch.
- Layouts that differ only by length (no type byte, e.g. UBX-RXM-PMREQ v0/v1) do not use this base; they narrow `matches()` on length alone.
