# DataSlot

## Overview

`castle_ext::data::data_slot<T, MaxReaders>` is a lock-free cell that holds the latest value of `T`. One writer publishes; any number of readers take zero-copy snapshots. It is the heap-free counterpart of an atomically swapped `std::shared_ptr<const T>`:

| `shared_ptr` design | `data_slot` |
|---|---|
| `make_shared<T>(value)` on each publish | Copy into one of `MaxReaders + 2` preallocated buffers |
| `atomic_store` / `atomic_load` | One atomic exchange (writer) / one atomic increment (reader) |
| `shared_ptr<const T>` held by a reader | Move-only `snapshot<T>` |
| Unbounded memory | Fixed memory; the writer reports `status::full` instead of allocating |

A held snapshot stays valid and unchanged while newer values are published. Readers access the value in place, so the only copy of `T` is the one the writer makes. Each slot of a [`data_pool`](data_pool.md) is a `data_slot`, and a slot can also be used alone as a single-type channel.

## Header

`#include "castle_ext/data/data_slot.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../include/castle/core/compiler.hpp)
- [`castle/core/error_handler.hpp`](../../../include/castle/core/error_handler.hpp)
- [`castle/core/traits.hpp`](../../../include/castle/core/traits.hpp)
- [`castle/core/types.hpp`](../../../include/castle/core/types.hpp)
- [`castle/core/status.hpp`](../../../include/castle/core/status.hpp)
- [`castle/sync/atomic.hpp`](../../sync/atomic.md)

## Public API

### `data_slot<T, MaxReaders = 1>`

| API | Description |
|---|---|
| `value_type` | `T`. |
| `data_slot()` | The current value is a default-constructed `T` with generation 0. |
| `publish(const T& value)` | Copies `value` into a free buffer and makes it current. Returns `status::ok`, `status::full` (every spare buffer is held by readers), or `status::data_loss` (another `publish()` on this slot is in progress). On failure the current value is unchanged. Never blocks. |
| `get_snapshot() const` | Pins the current value and returns a non-empty `snapshot<T>`. Wait-free. |

### `snapshot<T>`

| API | Description |
|---|---|
| `snapshot()` | Constructs an empty snapshot. |
| move constructor / move assignment | Transfer ownership; the source becomes empty. Copying is deleted. |
| `~snapshot()`, `reset()` | Release the buffer back to the writer. |
| `explicit operator bool` | Returns whether a value is held. |
| `get()` | Returns `const T*`, or `nullptr` when empty. |
| `operator*`, `operator->` | Access the value. Calling them on an empty snapshot is a contract violation (`CASTLE_ASSERT`). |
| `generation()` | 0 for the initial default value or an empty snapshot; 1, 2, 3, … for each successful publish. After 2^32 − 1 publishes the count wraps to 1, never to 0. |

## Usage Example

See [`samples/castle_ext/data/data_slot.cpp`](../../../samples/castle_ext/data/data_slot.cpp).

```cpp
castle_ext::data::data_slot<lidar_scan, 2U> slot;

// Driver thread
if (slot.publish(scan) != castle::status::ok)
{
    // Readers still hold every spare buffer: drop this scan or retry on the next one.
}

// Reader thread: no copy of lidar_scan is made
const castle_ext::data::snapshot<lidar_scan> latest = slot.get_snapshot();
if (latest.generation() != 0U)
{
    process(latest->range_mm);
}
```

## Constraints & Notes

- **Type requirements:** `T` must be a non-array, non-cv object type that is default-constructible and copy-assignable. It does not have to be trivially copyable, so Castle containers and strings may be members.
- **Memory:** `MaxReaders + 2` objects of `T`, plus three 32-bit words and a flag. `MaxReaders` can be 1 to 254.
- **Sizing `MaxReaders`:** set it to the number of snapshots of *older* values that may be held at the same moment, usually one per reader thread that keeps a snapshot across a publish. Snapshots of the current value never block the writer. `publish()` returns `status::full` only when more than `MaxReaders` older buffers are held.
- **Writer:** one at a time. A `publish()` that overlaps another one on the same slot (another thread, an ISR, or re-entry from `T::operator=`) is rejected with `status::data_loss`; slot state is never corrupted. Cost: one copy of `T` plus O(`MaxReaders`) atomic loads.
- **Readers:** any number of threads. Acquiring is one `fetch_add`. Releasing is one CAS while the buffer is still current (retried only when the control word changes concurrently), otherwise one `fetch_sub`.
- **Memory ordering:** the writer's exchange is release and the reader's increment is acquire, so a snapshot always sees a fully written value. A reader's release is ordered before the writer reuses that buffer.
- **Implementation:** the control word holds the current buffer index (high 8 bits) and the number of readers that pinned it (low 24 bits). When a buffer is retired, the writer moves that count into the buffer's own holder counter.
- **Lifetime:** a snapshot must not outlive its slot. The slot cannot be copied or moved.
