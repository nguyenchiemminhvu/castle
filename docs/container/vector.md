# vector

## Overview
`castle::container::vector` is a fixed-capacity contiguous sequence that stores its elements inside the object itself. Use it when you need vector-like append/pop-back behavior without heap allocation or capacity growth.

## Header
`#include "castle/container/vector.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/error_handler.md`](../core/error_handler.md)
- [`../core/traits.md`](../core/traits.md)
- [`../core/types.md`](../core/types.md)
- [`initializer_list.md`](initializer_list.md)
- [`../error/status.md`](../error/status.md)
- [`../iterator/reverse_iterator.md`](../iterator/reverse_iterator.md)
- [`../memory/object.md`](../memory/object.md)
- [`../memory/construct.md`](../memory/construct.md)
- [`../memory/destroy.md`](../memory/destroy.md)
- [`../memory/static_storage.md`](../memory/static_storage.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`../utility/move.md`](../utility/move.md)

## Public API
| API | Description |
|---|---|
| `vector<T, N>` | Fixed-capacity vector with room for at most `N` live elements. |
| Constructors / assignment | Default, copy, move, and `initializer_list` forms; copy and move are O(size). |
| `size()`, `capacity()`, `empty()`, `full()` | O(1) capacity/state queries. |
| `begin()/end()`, `cbegin()/cend()`, `rbegin()/rend()`, `crbegin()/crend()` | Iterators over the contiguous live range. |
| `operator[]`, `front()`, `back()`, `data()` | O(1) element/storage access; unchecked for bounds/emptiness. |
| `emplace_back(args...)` | O(1) append-in-place; returns `status::ok` or `status::full`. |
| `push_back(const T&)`, `push_back(T&&)` | O(1) append by copy or move; returns `status::ok` or `status::full`. |
| `pop_back()` | O(1) remove-last; returns `status::ok` or `status::empty`. |
| `clear()` | O(size) destroy all live elements. |
| `static_capacity` | Compile-time capacity constant equal to `N`. |

## Usage Example
See [`samples/sample_vector.cpp`](../../samples/sample_vector.cpp).

```cpp
castle::container::vector<int, 4U> values{1, 2};
values.emplace_back(3);
values.push_back(4);
CASTLE_SAMPLE_CHECK(values.full());
CASTLE_SAMPLE_CHECK(values.back() == 4);
```

## Constraints & Notes
- No dynamic allocation; storage for `N` elements is embedded in the object.
- Appends and removals at the back are O(1); copy, move, clear, and initializer-list construction are O(size).
- References and iterators remain valid until the referenced element is removed or the vector is cleared; no reallocation occurs.
- `operator[]`, `front()`, and `back()` are unchecked.
- A full append returns `status::full`; popping an empty vector returns `status::empty`.
