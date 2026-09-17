# spsc_buffer_desc

## Overview
`castle::container::spsc_buffer_desc` is a fixed-capacity descriptor ring that hands caller-owned buffers from one producer to one consumer. Use it when payload storage must live outside the container, such as DMA-capable RX/TX memory, but descriptor ownership still needs deterministic FIFO hand-off.

## Header
`#include "castle/container/spsc_buffer_desc.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/error_handler.md`](../core/error_handler.md)
- [`../core/traits.md`](../core/traits.md)
- [`../core/types.md`](../core/types.md)
- [`../error/status.md`](../error/status.md)
- [`../atomic/atomic.md`](../atomic/atomic.md)
- [`array.md`](array.md)
- [`array_view.md`](array_view.md)

## Public API
| API | Description |
|---|---|
| `spsc_buffer_desc<TBuffer, BUFFER_SIZE, N_BUFFERS, TState>` | Descriptor ring over `N_BUFFERS` caller-owned buffers, each holding `BUFFER_SIZE` elements. |
| `buffer_handle` | Non-owning per-slot handle returned by acquire operations. |
| `is_valid()` | Returns whether the descriptor was constructed with non-null payload storage. |
| `capacity()` / `buffer_capacity()` | Compile-time descriptor count and per-buffer payload size. |
| `writable()` / `readable()` / `full()` / `empty()` | O(1) state queries for the next producer/consumer slot. |
| `acquire_write(out)` / `acquire_write()` | Acquire the next free slot for writing; return `status::invalid_config` or `status::full` on failure. |
| `acquire_read(out)` / `acquire_read()` | Acquire the next ready slot for reading; return `status::invalid_config` or `status::empty` on failure. |
| `clear()` | O(N_BUFFERS) quiescent reset; invalidates outstanding handles by advancing slot generations. |
| `buffer_handle::is_valid()`, `is_write()`, `is_read()` | Check handle ownership and mode. |
| `buffer_handle::capacity()`, `max_size()`, `size()` | Capacity and committed-size queries. |
| `buffer_handle::data()`, `write_view()`, `read_view()` | Access the payload pointer or typed array views. |
| `buffer_handle::commit(count)` | Publish a written buffer; returns `status::invalid_argument` or `status::out_of_range` on failure. |
| `buffer_handle::release()` | Return a read buffer to the free pool; returns `status::invalid_argument` on failure. |
| `buffer_handle::cancel()` | Return an acquired write buffer to the free pool without publishing it. |

## Usage Example
See [`samples/sample_spsc_buffer_desc.cpp`](../../samples/sample_spsc_buffer_desc.cpp).

```cpp
uint8_t storage[2U][16U] = {};
castle::container::spsc_buffer_desc<uint8_t, 16U, 2U> desc(&storage[0U][0U]);

auto write = desc.acquire_write();
auto view = write.write_view();
view[0U] = 0x55U;
CASTLE_SAMPLE_CHECK(write.commit(1U) == castle::status::ok);
```

## Constraints & Notes
- No dynamic allocation; only descriptor metadata is owned by the container.
- Payload storage is supplied by the caller and must remain valid for the descriptor object's entire lifetime.
- Slot lifecycle is `free -> writing -> ready -> reading -> free`; `cancel()` returns `writing -> free`.
- With the default `castle::atomic<uint8_t>` state type, `commit()`, `release()`, and `cancel()` publish with release stores and `acquire_write()`, `acquire_read()`, and `is_valid()` observe with acquire loads.
- A non-atomic `TState` is only safe when external synchronization already guarantees ordering and visibility.
- This is strictly a single-producer/single-consumer structure; multiple writers or readers require external synchronization.
- `clear()` is quiescent and must not race with DMA, interrupts, or foreground producer/consumer code.
