# sample_window

## Overview
Fixed-capacity, drop-oldest window over the newest `Capacity` samples. It is the streaming storage shared by the sliding statistics in `castle::statistic`. When the window is full, a push evicts the oldest sample and can report it, so incremental estimators can retract it. Samples are addressed oldest to newest.

## Header
`#include "castle_ext/statistic/sample_window.hpp"`

## Dependencies
- [compiler](../../castle/core/compiler.md)
- [status](../../castle/core/status.md)
- [types](../../castle/core/types.md)
- [ring_buffer](../../castle/dsa/ring_buffer.md)
- [tags](../../castle/iterator/tags.md)

## Public API
`sample_window<T, Capacity>`: `T` must be trivially copyable, trivially destructible, and default constructible; `Capacity > 0`. Logical index 0 is the oldest sample. Stack use is O(1) for every operation below.

| API | Description | Time |
| --- | --- | --- |
| `capacity()` (static) | Compile-time capacity. | O(1) |
| `size()`, `empty()`, `full()`, `available()` | State queries; `available() == capacity() - size()`. | O(1) |
| `push(value)` | Appends; returns `true` when the oldest sample was evicted. | O(1) |
| `push(value, evicted)` | Appends; returns `true` and writes the evicted sample to `evicted`. `evicted` is unchanged when nothing is evicted. | O(1) |
| `clear()` | Removes every sample. | O(1) |
| `operator[](index)` | Copy of the sample, or a value-initialized `T` when `index >= size()`. | O(1) |
| `at(index, out)` | `status::ok`, or `status::out_of_range` (`out` unchanged). | O(1) |
| `oldest(out)`, `newest(out)` | `status::ok`, or `status::empty` (`out` unchanged). | O(1) |
| `begin()`, `end()` | `const_iterator` (forward, oldest to newest). Dereferencing returns a copy. | O(1) |
| `latest(count)` | `view` over the newest `min(count, size())` samples; has `begin()`, `end()`, `size()`, `empty()`. | O(1) |
| `copy_to(destination, destination_capacity)` | Copies oldest to newest; returns `min(size(), destination_capacity)`, or 0 for `nullptr`. | O(size()) |

## Usage Example
See [`samples/castle_ext/statistic/sample_window.cpp`](../../../samples/castle_ext/statistic/sample_window.cpp).

```cpp
castle::statistic::sample_window<float, 4U> window;
window.push(1.0f);
float evicted = 0.0f;
if (window.push(2.0f, evicted))
{
    // `evicted` holds the sample that left the window.
}
for (const float sample : window.latest(2U))
{
    (void)sample;
}
```

## Constraints & Notes
- Inline storage only (a `castle::dsa::ring_buffer`); no heap allocation, no exceptions.
- Non-copyable and non-movable, like the underlying ring buffer.
- Iterators and views are invalidated by `push()` and `clear()`.
- Because the ring has no contiguous storage, iterator dereference returns by value and `operator[]` is O(1) but not a reference.
- Not thread-safe; protect shared instances externally.
