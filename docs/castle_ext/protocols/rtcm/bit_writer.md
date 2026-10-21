# RTCM bit writer

## Overview

`bit_writer.hpp` provides `bit_writer`, a bounds-checked, sequential, **big-endian bit** writer over a caller-provided `castle::container::array_view<uint8_t>`, and `encode_message()`, which turns a message model into a complete RTCM 3 frame. The module is **encode-only**: it produces the frames sent to a GNSS receiver; there is no decoder.

RTCM payloads are bit streams: data fields (DF) are 1..64 bits wide and are packed most-significant bit first with no alignment between fields. The unused bits of the last byte are zero, which is the padding RTCM requires.

It uses the same sticky-error model as the UBX [`payload_writer`](../ubx/payload_writer.md): the first overflow, out-of-range value or explicit `fail()` latches a `castle::status`; every later write becomes a no-op and the encoder checks `status()` once at the end.

`encode_message()` writes the payload directly into the payload slot of the caller's frame buffer and then fills in the header and CRC-24Q around it (see [`rtcm.md`](rtcm.md)), so no scratch buffer or copy is needed.

## Header

`#include "castle_ext/protocols/rtcm/bit_writer.hpp"`

## Dependencies

* [`core/compiler.hpp`](../../../core/compiler.md)
* [`core/traits.hpp`](../../../core/traits.md)
* [`core/types.hpp`](../../../core/types.md)
* [`container/array_view.hpp`](../../../container/array_view.md)
* [`container/string.hpp`](../../../container/string.md)
* [`error/status.hpp`](../../../error/status.md)
* [`rtcm.hpp`](rtcm.md)

## Public API

### `bit_writer`

| API | Description |
| --- | --- |
| `explicit bit_writer(array_view<uint8_t> payload)` | Writer over caller storage. A view whose `data()` is `nullptr` has zero capacity, so the first write latches `full`. |
| `ok()` | `true` while every write fitted and no `fail()` was recorded. |
| `status()` | Latched outcome: `ok`, `full` after an overflow, `out_of_range` after an invalid value or width, or the first `fail()` reason. |
| `bit_size()` | Bits written so far. |
| `size()` | Payload size in bytes: the bits written rounded up to a whole byte. |
| `remaining_bits()` | Bits that can still be written. |
| `fail(castle::status reason)` | Latches an encoder-specific failure (only the first is kept), e.g. `invalid_argument` for MSM masks that do not match the satellite and cell counts. |
| `write_bits(uint64_t value, size_type width)` | Appends the low `width` bits of `value`, MSB first. `width` may be `0..64`; a wider width, or a `value` with bits above `width`, latches `out_of_range`; not enough room latches `full`. A failing write stores nothing. |

### Data fields

| API | Description |
| --- | --- |
| `field(T value, size_type bits)` for unsigned integral `T` (not `bool`) | RTCM `uintN`. Latches `out_of_range` if `value` does not fit `bits` (or `bits > 64`). |
| `field(T value, size_type bits)` for signed integral `T` | RTCM `intN`, two's complement. Latches `out_of_range` when `bits` is `0` or above `64`, or `value` is outside `-2^(bits-1) .. 2^(bits-1)-1`. |
| `field(bool value)` | One-bit flag. |
| `reserved(size_type bits)` | Writes `bits` zero bits; runs longer than 64 bits are split internally. |
| `text(const castle::container::string<N>& value)` | Writes an 8-bit character count followed by the 8-bit characters (DF029/DF030 and siblings). `N` must be at most 255 (`static_assert`). |

### Frame helper

| API | Description |
| --- | --- |
| `encode_message(const Message& message, uint8_t* output, size_type output_capacity, size_type& bytes_written)` | `Message` must provide `void encode(bit_writer&) const`. Returns `invalid_argument` for a null `output`, `full` when the capacity is below `RTCM_FRAME_OVERHEAD` or the frame does not fit, the first status latched by `Message::encode()`, or `ok`. `bytes_written` is the frame size on success and `0` otherwise. |

## Usage Example

```cpp
#include "castle_ext/protocols/rtcm/bit_writer.hpp"
#include "castle_ext/protocols/rtcm/messages/station_arp.hpp"

using namespace castle::protocols::rtcm;

// Raw bit writer over caller storage.
uint8_t payload[8U];
bit_writer writer(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
writer.field(static_cast<uint16_t>(1005U), 12U);
writer.field(static_cast<int32_t>(-2), 12U);
writer.field(true);
writer.reserved(3U);
// writer.ok(), writer.bit_size() == 28, writer.size() == 4

// A message model encodes straight into a frame.
messages::station_arp station{};
station.station_id = 1U;
station.set_position_m(4000000.0, 3000000.0, 5000000.0);

uint8_t frame[32U];
castle::size_type written = 0U;
castle::status result = encode_message(station, frame, sizeof(frame), written);
// result == castle::status::ok, written == 25
```

See [`samples/castle_ext/protocols/rtcm/bit_writer.cpp`](../../../../samples/castle_ext/protocols/rtcm/bit_writer.cpp).

## Constraints & Notes

- Encode only: Castle has no RTCM decoder.
- No heap allocation, exceptions, RTTI, virtual dispatch or STL containers. The writer is **non-owning** and never writes outside its view.
- `encode_message()` limits the payload to `RTCM_MAX_PAYLOAD_LEN` (1023 bytes) even when `output` is larger; a message that needs more latches `full`.
- Fields that only partly fit are not stored: a failing write leaves the position unchanged, but earlier fields stay in the buffer.
- On any failure `encode_message()` leaves `bytes_written == 0`; bytes already written into `output` are undefined and must not be transmitted.
- A failing signed `field()` is reported through `fail()`, so an already latched `full` is kept as the first reason.
- Not thread-safe; use one writer per encoding context.
- Complexity: O(payload size) for `encode_message()`; O(width / 8) per field.
