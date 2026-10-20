# UBX payload reader

## Overview

`payload_reader.hpp` provides a bounds-checked sequential cursor over a validated UBX payload. Message decoders need to walk a payload field-by-field without manually computing byte offsets (which is error-prone) or `reinterpret_cast`-ing the buffer into a packed struct (which is unsafe due to alignment and endianness portability issues). `payload_reader` handles the sequential little-endian decoding and cursor advancement automatically.

It uses a sticky-error model: if a read runs past the end of the payload (or a decoder calls `fail()`), it starts returning zero/nullptr and every subsequent read continues to fail. This allows decoders to perform a single `ok()` check at the end of the sequence rather than guarding every individual field access.

The reader shares its *layout verbs* (`field`, `reserved`, `constant`, `text`) with [`payload_writer`](payload_writer.md). A message describes its wire format once in a `layout(Self&, Stream&)` function, and the same function both decodes (with a reader) and encodes (with a writer); see [`detail/fixed_message.hpp`](messages/detail/fixed_message.md).

## Header

`#include "castle_ext/protocols/ubx/payload_reader.hpp"`

## Dependencies

* [`core/compiler.hpp`](../../../core/compiler.md)
* [`core/traits.hpp`](../../../core/traits.md)
* [`core/types.hpp`](../../../core/types.md)
* [`container/array.hpp`](../../../container/array.md)
* [`container/array_view.hpp`](../../../container/array_view.md)
* [`container/string.hpp`](../../../container/string.md)
* [`utility/bit_cast.hpp`](../../../utility/bit_cast.md)
* [`ubx.hpp`](ubx.md)

## Public API

### Cursor and state

| API | Description |
| --- | --- |
| `payload_reader(payload)` | Constructs a reader over an `array_view<const uint8_t>`. A view whose `data()` is `nullptr` starts in the failed state (`ok() == false`). |
| `ok()` | Reports whether every read so far stayed within the payload and no `fail()` was recorded (sticky). |
| `fail()` | Latches a decoder-specific failure, e.g. an unexpected constant byte. |
| `position()` | Current cursor offset in bytes from the start of the payload. |
| `remaining()` | Number of unread bytes. |
| `skip(count)` | Advances the cursor by `count` bytes. Returns `true` on success. |
| `read_bytes(count)` | Returns a `const uint8_t*` to `count` raw bytes and advances, or `nullptr` on failure. |

### Width-named reads

| API | UBX type | Bytes |
| --- | --- | --- |
| `read_u8()`, `read_i8()` | `U1`/`X1`, `I1` | 1 |
| `read_u16()`, `read_i16()` | `U2`/`X2`, `I2` | 2 |
| `read_u32()`, `read_i32()` | `U4`/`X4`, `I4` | 4 |
| `read_u64()`, `read_i64()` | `U8`, `I8` | 8 |
| `read_r4()` | `R4` (IEEE-754 `float`, via `castle::bit_cast`) | 4 |
| `read_r8()` | `R8` (IEEE-754 `double`, via `castle::bit_cast`) | 8 |

### Type-deduced reads

| API | Description |
| --- | --- |
| `read(T& value)` | Overloads for `char`, `uint8_t` ... `int64_t`, `float`, `double`: reads exactly `sizeof(T)` bytes into `value`. |
| `read(Enum& value)` | Reads an `enum` through its underlying integer type. |
| `read(castle::container::array<T, N>& values)` | Reads `N` consecutive elements (UBX `U2[6]`, `I1[4]`, ...). |

### Layout verbs (shared with `payload_writer`)

| API | Description |
| --- | --- |
| `field(T& value)` | Same as `read(value)`; width follows the declared field type. |
| `reserved(count)` | Skips `count` reserved bytes. |
| `constant(expected)` | Consumes one byte and calls `fail()` unless it equals `expected` (message type/version bytes). |
| `text(castle::container::string<W>& value)` | Consumes exactly `W` bytes of a NUL-padded `CH[W]` field and stores the characters before the first NUL. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/payload_reader.hpp"

// `message` is a validated message_view obtained from decode_frame().
castle::protocols::ubx::payload_reader reader(message.payload);

uint32_t i_tow = reader.read_u32();
uint16_t year = 0U;
reader.field(year);          // width follows the type
reader.reserved(2U);         // skip reserved bytes
reader.constant(0x00U);      // required version byte

if (!reader.ok())
{
    return false;            // payload too short or version mismatch
}
```

See [`samples/castle_ext/protocols/ubx/payload_reader.cpp`](../../../../samples/castle_ext/protocols/ubx/payload_reader.cpp).

## Constraints & Notes

- No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers.
- The reader is **non-owning**. The payload buffer must outlive the `payload_reader`.
- **Sticky error state:** a failed read or `fail()` latches `ok()` to `false`. Later reads return `0` / `nullptr` without moving the cursor or touching memory out of bounds.
- `read_bytes(count)` returns a pointer into the underlying buffer; do not modify the data through it.
- `read_r4()` / `read_r8()` assume IEEE-754 `float` / `double`, which UBX requires.
- Not thread-safe; use one reader per decoding context.
- Complexity: every operation is O(1) except array and `text` reads, which are O(N).
