# array_view

## Overview
Lightweight non-owning view over contiguous elements. Use it to pass pointer-plus-length data through APIs without copying and without transferring ownership.

## Header
`#include "castle/container/array_view.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [types](../core/types.md)
- [initializer_list](initializer_list.md)

## Public API
| API | Description |
| --- | --- |
| `array_view<T>` | Pointer/length view. `T` may be `const` for read-only access. |
| `array_view()` | Constructs an empty view. O(1). |
| `array_view(pointer, size)` | Views a raw contiguous range. O(1). |
| `array_view(Container&)`, `array_view(const Container&)` | Views a compatible container exposing `data()` and `size()`. O(1). |
| `size()`, `empty()`, `data()` | View state queries. O(1). |
| `operator[]`, `front()`, `back()` | Element access. O(1); caller must respect bounds/non-empty preconditions. |
| `begin()`, `end()` | Pointer iterators over the viewed range. O(1). |
| `subview(offset, count)` | Returns a clamped child view or an empty default view when `offset > size()`. O(1). |
| `make_array_view(...)` | Factory overloads for mutable pointers, const pointers, and initializer lists. O(1). |

## Usage Example
See `samples/sample_array_view.cpp`.

```cpp
uint16_t samples[4] = {1U, 2U, 3U, 4U};
castle::container::array_view<uint16_t> view(samples, 4U);
castle::container::array_view<uint16_t> tail = view.subview(2U, 2U);
```

## Constraints & Notes
- No allocation; the view stores only a pointer and a length.
- The referenced storage must outlive the view.
- Iterator invalidation follows the underlying storage: destroying or relocating the source buffer invalidates the view.
- `make_array_view(initializer_list)` is only safe for immediate use within the same full expression because the backing array is temporary.
- Not thread-safe; coordinate concurrent mutation of the referenced storage externally.
