# UBX fixed_message

## Overview

`detail::fixed_message` is the CRTP base behind every fixed-length UBX message model. A message describes its payload **once**, in a static `layout(Self&, Stream&)` function, and gets `matches()`, `decode()` and `encode()` from the base. The same layout runs through a [`payload_reader`](../../payload_reader.md) when decoding and a [`payload_writer`](../../payload_writer.md) when encoding, so the field order and widths cannot drift between the two directions.

## Header

`#include "castle_ext/protocols/ubx/messages/detail/fixed_message.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../../core/types.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../../payload_writer.md)

## Public API

`template <typename Derived, uint8_t MsgClass, uint8_t MsgId, castle::size_type PayloadLength> struct fixed_message`

| Member | Description |
| --- | --- |
| `msg_class`, `msg_id` | `static constexpr uint8_t` copies of `MsgClass` / `MsgId`. |
| `payload_length` | `static constexpr castle::size_type`, the exact payload length. |
| `static bool matches(const message_view& raw)` | `true` when `raw` has this class/ID. |
| `static bool decode(const message_view& raw, Derived& out)` | Value-initialises `out`, then requires a class/ID match and `payload_length() == PayloadLength`, runs `Derived::layout(out, reader)`, and checks `reader.ok()`. On any failure `out` is value-initialised again and `false` is returned. |
| `void encode(payload_writer& writer) const` | Runs `Derived::layout(*this, writer)`; used by [`encode_message()`](../../payload_writer.md). |

### Requirements on `Derived`

```cpp
template <typename Self, typename Stream>
static void layout(Self& self, Stream& stream) noexcept;
```

`Self` is `Derived` (decode) or `const Derived` (encode). The layout may only use the shared stream verbs: `field()`, `reserved()`, `constant()` and `text()`. The bytes it consumes must add up to exactly `PayloadLength`.

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/detail/fixed_message.hpp"

namespace ubx = castle::protocols::ubx;

struct nav_eoe : ubx::messages::detail::fixed_message<nav_eoe, ubx::UBX_CLASS_NAV, ubx::UBX_ID_NAV_EOE, 4U>
{
    uint32_t i_tow = 0U;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) noexcept
    {
        stream.field(self.i_tow);
    }
};

// nav_eoe::decode(raw, value);                         // decode a received frame
// ubx::encode_message(value, frame, sizeof(frame), n); // or encode one
```

Real messages built on this base: [`nav_posllh`](../nav_posllh.md), [`cfg_rst`](../cfg_rst.md).

## Constraints & Notes

- Header-only, zero runtime overhead: `encode()` is a non-virtual member and the CRTP cast resolves at compile time.
- No heap allocation, exceptions, RTTI or virtual dispatch.
- `decode()` rejects shorter **and longer** payloads; the UBX length field is authoritative.
- A failed `constant()` check (wrong message type or version byte) makes `decode()` return `false`.
- Decoded models own their data; they do not alias the parser's payload buffer.
- Not thread-safe; models are plain value types.
