# UBX payload reader

## Overview

`payload_reader.hpp` provides a bounds-checked sequential cursor over a validated UBX payload. Message decoders need to walk a payload field-by-field without manually computing byte offsets (which is error-prone) or `reinterpret_cast`-ing the buffer into a packed struct (which is unsafe due to alignment and endianness portability issues). `payload_reader` handles the sequential little-endian decoding and cursor advancement automatically.

It uses a sticky-error model: if a read runs past the end of the payload, it starts returning zero/nullptr and every subsequent read continues to fail. This allows decoders to perform a single `ok()` check at the end of the sequence rather than guarding every individual field access.

## Header

`#include "castle_ext/protocols/ubx/payload_reader.hpp"`

## Dependencies

* [`core/compiler.hpp`](../../../core/compiler.md)
* [`core/types.hpp`](../../../core/types.md)
* [`container/array_view.hpp`](../../../container/array_view.md)
* [`ubx.hpp`](ubx.md)

## Public API

| API | Description |
| --- | --- |
| `payload_reader(payload)` | Constructs a reader over an `array_view<const uint8_t>` representing the message payload. |
| `ok()` | Reports whether every read so far has stayed within the payload bounds (sticky error state). |
| `position()` | Returns the current cursor offset in bytes from the start of the payload. |
| `remaining()` | Returns the number of unread bytes remaining in the payload. |
| `skip(count)` | Advances the cursor by `count` bytes without inspecting them. Returns `true` if successful. |
| `read_bytes(count)` | Returns a pointer (`const uint8_t*`) to `count` raw bytes (e.g., for fixed-size ASCII fields) and advances the cursor. |
| `read_u8()`, `read_i8()` | Reads an unsigned/signed 8-bit integer and advances the cursor by 1 byte. |
| `read_u16()`, `read_i16()` | Reads a little-endian unsigned/signed 16-bit integer and advances the cursor by 2 bytes. |
| `read_u32()`, `read_i32()` | Reads a little-endian unsigned/signed 32-bit integer and advances the cursor by 4 bytes. |
| `read_u64()`, `read_i64()` | Reads a little-endian unsigned/signed 64-bit integer and advances the cursor by 8 bytes. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"

// Assuming `message` is a validated message_view obtained from decode_frame()
castle::protocols::ubx::payload_reader reader(message.payload);

// Read fields sequentially
uint32_t i_tow = reader.read_u32(); // GPS time of week
uint16_t year  = reader.read_u16();
uint8_t  month = reader.read_u8();
uint8_t  day   = reader.read_u8();

// Perform a single validation check at the end
if (!reader.ok()) 
{ 
    // The payload was shorter than expected; parsed values will be 0.
    return false; 
}

// Proceed with valid decoded fields...
```

## Constraints & Notes

- No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers.
- The reader is **non-owning**. The underlying payload buffer must outlive the `payload_reader` instance.
- **Sticky error state:** A failed read immediately latches `ok()` to `false`. All subsequent reads will return `0` or `nullptr` without advancing the cursor or accessing out-of-bounds memory.
- `read_bytes(count)` returns a direct pointer to the underlying buffer. The caller must not attempt to modify the referenced data.
