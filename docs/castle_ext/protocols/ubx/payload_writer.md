# UBX payload writer

## Overview

`payload_writer.hpp` is the encoding mirror of [`payload_reader`](payload_reader.md). `payload_writer` is a bounds-checked, sequential little-endian cursor over a caller-provided `castle::container::array_view<uint8_t>`: every `write()` stores one field in UBX byte order and advances.

It uses the same sticky-error model as the reader. The first overflow latches `castle::status::full` (or the first status passed to `fail()`), every later write becomes a no-op, and the encoder checks `status()` once at the end.

The header also provides two frame-level helpers:

- `encode_message(message, ...)` encodes any message model directly into the payload slot of the caller's frame buffer, then fills in the UBX header and Fletcher-8 checksum around it. No scratch payload copy is made.
- `encode_poll<Message>(...)` encodes the zero-payload poll request for a message (e.g. UBX-MON-VER, UBX-NAV-PVT).

## Header

`#include "castle_ext/protocols/ubx/payload_writer.hpp"`

## Dependencies

* [`core/compiler.hpp`](../../../core/compiler.md)
* [`core/traits.hpp`](../../../core/traits.md)
* [`core/types.hpp`](../../../core/types.md)
* [`container/array.hpp`](../../../container/array.md)
* [`container/array_view.hpp`](../../../container/array_view.md)
* [`container/string.hpp`](../../../container/string.md)
* [`error/status.hpp`](../../../error/status.md)
* [`utility/bit_cast.hpp`](../../../utility/bit_cast.md)
* [`utility/bytes.hpp`](../../../utility/bytes.md)
* [`ubx.hpp`](ubx.md)

## Public API

### `payload_writer`

| API | Description |
| --- | --- |
| `explicit payload_writer(array_view<uint8_t> payload)` | Writer over caller storage. A view whose `data()` is `nullptr` has zero capacity, so the first write latches `full`. |
| `ok()` | `true` while every write fitted and no `fail()` was recorded. |
| `status()` | Latched outcome: `ok`, `full` after an overflow, or the first `fail()` reason. |
| `size()` | Bytes written so far. |
| `remaining()` | Writable bytes left. |
| `fail(castle::status reason)` | Latches an encoder-specific failure (only the first is kept), e.g. `invalid_argument` for a CFG key with an unknown size. |
| `write_zeros(count)` | Writes `count` zero bytes. |
| `write_bytes(data, count)` | Copies `count` raw bytes. |
| `write_uint(value, width)` | Writes the low `width` bytes of `value` little-endian (runtime width, e.g. CFG values). |
| `write(T value)` | Overloads for `char`, `uint8_t` ... `int64_t`, `float` (`R4`), `double` (`R8`): exactly `sizeof(T)` bytes. |
| `write(Enum value)` | Writes an `enum` through its underlying type. |
| `write(const array<T, N>& values)` | Writes `N` consecutive elements. |

### Layout verbs (shared with `payload_reader`)

| API | Description |
| --- | --- |
| `field(const T& value)` | Same as `write(value)`. |
| `reserved(count)` | Writes `count` zero bytes. |
| `constant(value)` | Writes a fixed protocol byte (message type or version). |
| `text(const castle::container::string<W>& value)` | Writes a `CH[W]` field: the characters, then NUL padding up to exactly `W` bytes. |

### Frame helpers

| API | Description |
| --- | --- |
| `encode_message(const Message& m, uint8_t* output, size_type capacity, size_type& written)` | Requires `Message::msg_class`, `Message::msg_id` and `void encode(payload_writer&) const`. Returns `invalid_argument` for a null `output`, `full` when the frame does not fit, the encoder's latched failure, or `ok`. `written` is the frame size on success and `0` otherwise. |
| `encode_poll<Message>(uint8_t* output, size_type capacity, size_type& written)` | Encodes the empty-payload poll frame; forwards `encode_frame()`'s status. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/payload_writer.hpp"
#include "castle_ext/protocols/ubx/messages/cfg_rst.hpp"
#include "castle_ext/protocols/ubx/messages/mon_ver.hpp"

using namespace castle::protocols::ubx;

uint8_t frame[64U];
castle::size_type written = 0U;

// Command: cold start.
castle::status result = encode_message(messages::cfg_rst::cold_start(), frame, sizeof(frame), written);

// Poll request: UBX-MON-VER with an empty payload.
result = encode_poll<messages::mon_ver<>>(frame, sizeof(frame), written);

// Raw writer over caller storage.
uint8_t payload[4U];
payload_writer writer(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
writer.field(static_cast<uint16_t>(0x1234U));
writer.reserved(2U);
// writer.status() == castle::status::ok, writer.size() == 4
```

See [`samples/castle_ext/protocols/ubx/payload_writer.cpp`](../../../../samples/castle_ext/protocols/ubx/payload_writer.cpp).

## Constraints & Notes

- No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers.
- The writer is **non-owning** and never writes outside its view.
- `encode_message` limits the payload to `UBX_MAX_PAYLOAD_LEN` (65535) even when the output buffer is larger.
- On any failure `encode_message` leaves `written == 0`. Bytes already written into `output` are undefined; never transmit them.
- `float` / `double` writes assume IEEE-754, which UBX requires.
- Not thread-safe; use one writer per encoding context.
- Complexity: O(payload size) for `encode_message`; O(1) per scalar write.
