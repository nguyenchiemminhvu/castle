# initializer_list

## Overview
Castle-facing spelling of the compiler-recognized brace-init-list view type. Use it when an API should accept `{...}` syntax without allocating or copying into a dynamic container.

## Header
`#include "castle/container/initializer_list.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [config](../core/config.md)
- [types](../core/types.md)
- [traits](../core/traits.md)
- [traits](../iterator/traits.md)
- [reverse_iterator](../iterator/reverse_iterator.md)
- [forward](../utility/forward.md)

## Public API
| API | Description |
| --- | --- |
| `initializer_list<T>` | Alias of `std::initializer_list<T>` or a compatible freestanding fallback. |
| `const_reverse_initializer_iterator<T>` | Reverse-iterator alias for initializer-list ranges. |
| `begin(list)`, `end(list)` | Returns raw pointer iterators. O(1). |
| `size(list)`, `empty(list)` | Range size helpers. O(1). |
| `front(list)`, `back(list)` | First/last element access. O(1); list must be non-empty. |
| `rbegin(list)`, `rend(list)` | Reverse iteration helpers. O(1). |

## Usage Example
See `samples/sample_initializer_list.cpp`.

```cpp
castle::container::initializer_list<int> values = {1, 2, 3};
int first = castle::container::front(values);
int count = static_cast<int>(castle::container::size(values));
```

## Constraints & Notes
- The backing array is a compiler-managed temporary; do not store the view past the full expression that created it unless a named initializer-list object keeps it alive.
- No allocation, exceptions, RTTI, or virtual dispatch.
- Iterator invalidation follows the temporary lifetime of the backing array.
- Not thread-safe beyond the immutability guarantees of the underlying temporary storage.
