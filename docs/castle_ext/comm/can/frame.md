# CAN Frame

## Overview
`castle::comm::can::frame` is the platform-neutral value type for one classic CAN 2.0A/2.0B frame: identifier, identifier format, frame type (data or remote request), data length code (DLC) and an inline 8-byte payload. Every CAN backend converts it to and from the native structure (`struct can_frame` on both Linux SocketCAN and Zephyr), so application code never touches platform headers. The header also provides the identifier limits, the `id_format` and `frame_type` enumerations, `max_id()` and `validate_frame()`.

CAN FD is out of scope: the payload is fixed at 8 bytes (classic CAN only).

## Header
`#include "castle_ext/comm/can/frame.hpp"`

## Dependencies
- [`../../../container/array.md`](../../../container/array.md)
- [`../../../container/array_view.md`](../../../container/array_view.md)
- [`../../../container/initializer_list.md`](../../../container/initializer_list.md)
- [`../../../core/compiler.md`](../../../core/compiler.md)
- [`../../../core/types.md`](../../../core/types.md)
- [`../../../error/status.md`](../../../error/status.md)

## Public API
All names live in `castle::comm::can`.

### Constants and enumerations
| API | Description |
|---|---|
| `max_data_length` | `size_type` constant, `8U`: payload capacity of a classic CAN frame. |
| `max_standard_id` | `uint32_t` constant, `0x7FFU`: highest 11-bit identifier. |
| `max_extended_id` | `uint32_t` constant, `0x1FFFFFFFU`: highest 29-bit identifier. |
| `id_format` | `uint8_t` enum: `standard` (11-bit, CAN 2.0A, value `0`) and `extended` (29-bit, CAN 2.0B). |
| `frame_type` | `uint8_t` enum: `data` (carries `dlc` payload bytes, value `0`) and `remote` (RTR request; `dlc` is the requested length). |
| `max_id(id_format)` | `constexpr uint32_t`. Returns `max_extended_id` for `extended`, `max_standard_id` for every other value. `noexcept`, O(1). |

### `struct frame`
| Member | Description |
|---|---|
| `uint32_t id` | Identifier, to be within `max_id(format)`. |
| `id_format format` | Identifier format. |
| `frame_type type` | Data or remote frame. |
| `uint8_t dlc` | Data length code. Must be at most 8 to pass `validate_frame()`. |
| `container::array<uint8_t, 8> data` | Inline payload. Bytes beyond `data_length()` are kept at zero by the mutators. |
| `frame()` | `constexpr`. Standard-format data frame, identifier `0`, `dlc` `0`, zeroed payload. |
| `explicit frame(uint32_t identifier, id_format id_kind = id_format::standard)` | `constexpr`. Empty data frame with the given identifier and format. The identifier is not range-checked here; see `validate_frame()`. |
| `is_extended()` | `constexpr bool`. True when `format == id_format::extended`. |
| `is_remote()` | `constexpr bool`. True when `type == frame_type::remote`. |
| `data_length()` | `constexpr size_type`. `0` for remote frames, otherwise `dlc` clamped to 8. |
| `payload()` | `array_view<const uint8_t>` over the first `data_length()` bytes of `data`. Non-owning; valid while the frame lives. |
| `set_data(const void* bytes, size_type length)` | Makes the frame a data frame holding a copy of `bytes`; the rest of the payload is zeroed. Returns `ok`; `out_of_range` when `length > 8` (checked first); `invalid_argument` when `bytes == nullptr` and `length != 0`. The frame is unchanged on failure. O(8). |
| `set_data(initializer_list<uint8_t>)` | Same as above for a brace-enclosed list, e.g. `set_data({0x01U, 0x02U})`. Returns `ok` or `out_of_range` (more than 8 elements). |
| `set_remote(size_type requested_length)` | Makes the frame a remote request: type `remote`, `dlc = requested_length`, payload zeroed. Returns `ok` or `out_of_range` when `requested_length > 8`; unchanged on failure. O(8). |

### Free functions
| API | Description |
|---|---|
| `operator==(frame const&, frame const&)` | True when identifier, format, type and DLC are equal and the first `data_length()` payload bytes are equal. Bytes beyond the valid length are ignored. `noexcept`, O(8). |
| `operator!=(frame const&, frame const&)` | Negation of `operator==`. |
| `validate_frame(frame const&)` | Returns `ok`, or `invalid_argument` for an out-of-range `format` or `type` enumerator, `dlc > 8`, or `id > max_id(format)`. `basic_bus::send()` calls it before touching the backend. |

## Usage Example
See `samples/castle_ext/comm/can/frame.cpp`.

```cpp
#include "castle_ext/comm/can/frame.hpp"

#include <assert.h>

using namespace castle::comm::can;

int main()
{
    frame request(0x123U);
    assert(request.set_data({0x01U, 0x02U, 0x03U}) == castle::status::ok);
    assert(request.data_length() == 3U);
    assert(request.payload()[2] == 0x03U);

    frame poll(0x18FF0001U, id_format::extended);
    assert(poll.set_remote(4U) == castle::status::ok);
    assert(poll.is_remote() && poll.is_extended());
    assert(poll.data_length() == 0U);          // RTR frames carry no payload

    assert(request.set_data(nullptr, 9U) == castle::status::out_of_range);
    assert(request.data_length() == 3U);       // unchanged on failure

    frame wide(0x800U);                        // 12 bits do not fit a standard identifier
    assert(validate_frame(wide) == castle::status::invalid_argument);
    wide.format = id_format::extended;
    assert(validate_frame(wide) == castle::status::ok);
    return 0;
}
```

## Constraints & Notes
- `frame` is a plain value type of a few words plus 8 payload bytes: trivially copyable, no heap, no exceptions, no RTTI, no virtual functions, `constexpr` constructors.
- Public fields can be assigned directly. Nothing stops an inconsistent frame (for example `dlc = 12`), which is why `basic_bus::send()` runs `validate_frame()`. Frames received from a backend are always valid.
- `set_data()` and `set_remote()` zero the payload beyond the valid length, so frames copy and compare deterministically.
- `data_length()` clamps `dlc` to 8, so `payload()` can never overrun `data` even for an unvalidated frame.
- A remote frame is a request, not a payload carrier: `data_length()` is zero and `payload()` is empty regardless of `dlc`.
- Filters do not distinguish data from remote frames; check `is_remote()` after reception (see [`filter.md`](filter.md)).
- Related: [`filter.md`](filter.md), [`types.md`](types.md), [`bus.md`](bus.md), [`can.md`](can.md).
