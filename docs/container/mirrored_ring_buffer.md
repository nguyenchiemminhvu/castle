# mirrored_ring_buffer

## Overview
`castle::container::mirrored_ring_buffer` is a fixed-capacity SPSC FIFO that keeps its logical sequence contiguous by mirroring writes into a second physical storage half. Use it for DMA or streaming I/O paths that benefit from contiguous reservations even when the logical ring wraps.

## Header
`#include "castle/container/mirrored_ring_buffer.hpp"`

## Dependencies
- [`../atomic/atomic.md`](../atomic/atomic.md)
- [`array_view.md`](array_view.md)
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/traits.md`](../core/traits.md)
- [`../core/types.md`](../core/types.md)
- [`../error/status.md`](../error/status.md)
- [`../utility/move.md`](../utility/move.md)

## Public API
| API | Description |
|---|---|
| `mirrored_ring_buffer<T, N>` | Fixed-capacity SPSC ring buffer with logical capacity `N` and physical storage `2 * N`. |
| `capacity()`, `max_size()`, `size()`, `available()`, `empty()`, `full()` | O(1) capacity/state queries. |
| `push()` / `pop()` / `pop(out)` | O(1) single-element FIFO operations; return `status::full` or `status::empty` on failure. |
| `peek(index, out)` | O(1) random access relative to the current front; returns `status::out_of_range` for an invalid index. |
| `push_bulk(src, max_count)` / `pop_bulk(dst, max_count)` | O(count) bulk copy helpers that return the number of transferred elements. |
| `write_reserve()` / `write_reserve_optimal()` / `write_commit()` | Writable contiguous reservation API for the producer. `write_commit()` returns `status::invalid_argument` or `status::full` on failure. |
| `read_reserve()` / `read_commit()` | Readable contiguous reservation API for the consumer. `read_commit()` returns `status::invalid_argument` or `status::empty` on failure. |
| `begin()/end()`, `cbegin()/cend()`, `data()`, `operator[]`, `front()`, `back()` | Access the readable logical sequence as one contiguous range. |
| `clear()` | O(1) quiescent reset that discards all logical contents. |
| `static_capacity` | Compile-time logical capacity constant equal to `N`. |

## Usage Example
See [`samples/sample_mirrored_ring_buffer.cpp`](../../samples/sample_mirrored_ring_buffer.cpp).

```cpp
castle::container::mirrored_ring_buffer<uint8_t, 8U> buffer;
auto write = buffer.write_reserve(4U);
for (castle::size_type i = 0U; i < write.size(); ++i)
{
    write[i] = static_cast<uint8_t>(i);
}
CASTLE_SAMPLE_CHECK(buffer.write_commit(write) == castle::status::ok);
```

## Constraints & Notes
- No dynamic allocation; the object owns `2 * N` elements so mirrored ranges stay contiguous.
- `T` must be trivially copyable and trivially destructible.
- The implementation does not rely on page alignment or virtual-memory aliasing; it mirrors writes explicitly.
- Producer operations use `write_index_` as their self-owned index and acquire-load `read_index_`; consumer operations use the opposite pattern.
- `write_commit()` and `read_commit()` publish progress with release stores, and the opposite side observes it with acquire loads.
- `front()`, `back()`, `operator[]`, and `data()` are unchecked and should only be used after readable data has been established.
- `clear()` must not race with producer or consumer activity.
