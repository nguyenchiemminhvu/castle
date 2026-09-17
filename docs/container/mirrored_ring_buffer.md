# Mirrored ring buffer

## Overview

A fixed-capacity single-producer/single-consumer (SPSC) ring buffer with physically mirrored storage.

The buffer owns `2 * N` element slots. The second half mirrors the first half, so a logical sequence that crosses the ring-buffer wrap point can still be accessed as one contiguous memory range.

This is useful for embedded DMA/I/O pipelines and other bounded producer/consumer designs where contiguous reservations are preferable to handling two wrapped regions.

There is no heap allocation, exceptions, RTTI, virtual dispatch, or STL dependency.

## Header

```cpp
#include "castle/container/mirrored_ring_buffer.hpp"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `mirrored_ring_buffer<T, N>` | Fixed-capacity SPSC ring buffer with `N` logical elements and `2 * N` physical storage slots. |
| `size()` / `capacity()` / `available()` / `empty()` / `full()` | Current occupancy and capacity information. |
| `push()` / `pop()` | Single-element FIFO operations; return `status::full` / `status::empty` when the buffer cannot satisfy the request. |
| `push_bulk()` / `pop_bulk()` | Bounded bulk transfer without heap allocation; return the number of elements actually transferred. |
| `peek()` | Read an element without removing it; returns `status::out_of_range` if `index` is not currently valid. |
| `write_reserve()` / `write_commit()` | Reserve and publish a contiguous writable region; `write_commit()` returns `status::invalid_argument` for a reservation it does not own, or `status::full` if it is larger than the free region. |
| `read_reserve()` / `read_commit()` | Reserve and release a contiguous readable region; `read_commit()` returns `status::invalid_argument` for a reservation it does not own, or `status::empty` if it is larger than the readable region. |
| `write_reserve_optimal()` | Obtain the complete currently available contiguous writable region when it satisfies the requested minimum size. |
| Iterators | `begin/end` and `cbegin/cend`; expose the logical contents as one contiguous random-access range. |
| Element access | `operator[]`, `front()`, `back()`, `data()`. |
| `clear()` | Discard all logical contents. Intended for quiescent use. |
| `static_capacity` | Compile-time logical element capacity. |

## Example

```cpp
castle::container::mirrored_ring_buffer<int, 8> buffer;

buffer.push(10);
buffer.push(20);
buffer.push(30);

for (auto it = buffer.begin(); it != buffer.end(); ++it)
{
    // The logical contents are a contiguous range.
}
```

For direct I/O or DMA-style access, use reservations:

```cpp
castle::container::mirrored_ring_buffer<uint8_t, 256> buffer;

auto write = buffer.write_reserve(64);

for (castle::size_type i = 0; i < write.size(); ++i)
{
    write[i] = static_cast<uint8_t>(i);
}

buffer.write_commit(write);

auto read = buffer.read_reserve(64);

// Consume read.data() through read.data() + read.size().

buffer.read_commit(read);
```

## Embedded notes

- The logical capacity is `N`; the physical storage requirement is `2 * N` elements.
- The second physical half mirrors the first half after a write reservation is committed.
- This allows a wrapped logical sequence to be represented by one contiguous range starting at the current read position.
- The container is designed for **single-producer/single-consumer** use.
- Each side stores only to its own index (`write_index_` for the producer, `read_index_` for the consumer), so that store/self-load pair only needs `memory_order_relaxed`. The *other* side's index is loaded with `memory_order_acquire` to observe what it published (new data for the consumer, reclaimed space for the producer) and is released with `memory_order_release` when advanced. A stale acquire read can only make a side see less room/data than truly exists, never more, so this asymmetric ordering is both correct and cheaper than acquiring both indices on every call.
- `front()`, `back()`, `operator[]`, and `data()` do not perform this acquire load themselves (matching the zero-overhead-accessor style used elsewhere in castle); call `size()`, `empty()`, `end()`, or `read_reserve()` first to establish that at least one element is available before using them, the same precondition `std::vector::front()` has for a non-empty container.
- Reservation APIs are intended for zero-copy-style access, DMA, and I/O workflows.
- A reservation should be committed by the same side that created it before making another reservation on that side.
- `push()`/`pop()`/`peek()`/`write_commit()`/`read_commit()` return `castle::status` rather than `bool`, so each failure mode is distinguishable: `status::full`/`status::empty` for a saturated/exhausted buffer, `status::out_of_range` for an invalid `peek()` index, and `status::invalid_argument` for a reservation passed to `write_commit()`/`read_commit()` that the buffer did not hand out. A reservation whose size no longer fits (e.g. because it was resized to something larger than the original) returns `status::full` from `write_commit()` or `status::empty` from `read_commit()`.
- A reader may freely modify the contents of a reservation returned by `read_reserve()`; the modified slots are only ever reused by a subsequent write, which fully re-establishes the mirror from what it writes regardless of prior contents.
- `clear()` is a quiescent operation and must not race with producer or consumer activity.
- `T` must be trivially copyable and trivially destructible.
- `N` must be greater than zero.
- No dynamic allocation is performed.
- For power-of-two capacities, the internal logical-to-physical index calculation uses masking; other capacities use modulo arithmetic.
- Publishing a reservation (`mirror_range`) copies at most `N` elements split into at most two straight-line loops (one per side of the physical wrap point), rather than branching on every element.
