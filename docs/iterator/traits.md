# iterator_traits

## Overview
`castle::iterator_traits` extracts the associated types needed by Castle iterator algorithms and adaptors. It exists so generic code can work with custom iterators and raw pointers without depending on `<iterator>`.

## Header
`#include "castle/iterator/traits.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/types.md`](../core/types.md)
- [`tags.md`](tags.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename Iterator> struct iterator_traits` | Reads `difference_type`, `value_type`, `pointer`, `reference`, and `iterator_category` from a custom iterator's nested aliases. |
| `template <typename T> struct iterator_traits<T*>` | Models mutable raw pointers as random-access iterators using Castle scalar aliases. |
| `template <typename T> struct iterator_traits<const T*>` | Models pointers to constant elements as random-access iterators while preserving const access. |

## Usage Example
See `samples/sample_iterator_traits.cpp` for a complete standalone example.

```cpp
#include "castle/iterator/traits.hpp"

using traits = castle::iterator_traits<uint8_t*>;
traits::iterator_category category{};
```

## Constraints & Notes
- The primary template requires a custom iterator type to expose all expected nested aliases.
- Pointer specializations use `castle::difference_type` and `random_access_iterator_tag`.
- The trait reports associated types only; it does not validate runtime iterator state or change invalidation behavior.
- Thread-safety and lifetime are exactly those of the iterators and storage that user code passes around.
- No heap allocation, exceptions, RTTI, virtual dispatch, or STL iterator utilities are used.
