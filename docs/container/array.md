# array

## Overview
Fixed-size contiguous container whose storage lives directly inside the object. Use it when the element count is known at compile time and every slot should always exist.

## Header
`#include "castle/container/array.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [types](../core/types.md)
- [traits](../core/traits.md)
- [error_handler](../core/error_handler.md)
- [reverse_iterator](../iterator/reverse_iterator.md)
- [initializer_list](initializer_list.md)
- [forward](../utility/forward.md)

## Public API
| API | Description |
| --- | --- |
| `array<T, N>` | Fixed-size container with exactly `N` inline elements. |
| `array()` | Default-constructs all elements. |
| `array(args...)` | Variadic constructor that participates only when exactly `N` arguments are supplied. |
| `array(initializer_list<T>)` | Initializes from exactly `N` list elements; asserts on size mismatch. |
| `static_size`, `size()`, `capacity()` | Compile-time/runtime size information. `size()` and `capacity()` both equal `N`. |
| `empty()`, `full()` | `empty()` is `true` only when `N == 0`; `full()` is always `true`. |
| `begin()/end()/cbegin()/cend()` | Pointer iterators. O(1). |
| `rbegin()/rend()/crbegin()/crend()` | Reverse iterators. O(1). |
| `operator[]`, `front()`, `back()`, `data()` | Element/pointer access. O(1). |
| `array<T, 0U>` | Zero-sized specialization with size/capacity/iterator/data access only; no element accessors. |

## Usage Example
See `samples/sample_array.cpp`.

```cpp
castle::container::array<int, 3U> values{1, 2, 3};
values[1] = 20;
int last = values.back();
```

## Constraints & Notes
- No dynamic allocation; all elements are part of the object itself.
- Iterators, pointers, and references remain valid until the array object is destroyed.
- Bounds are unchecked; callers must satisfy index and non-empty preconditions.
- Not thread-safe; concurrent mutation requires external synchronization.
