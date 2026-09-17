# iterator

## Overview
`castle/iterator/iterator.hpp` is the convenience umbrella header for Castle's iterator subsystem. It exists so a translation unit can include iterator algorithms, adaptors, and traits together through one locked public include.

## Header
`#include "castle/iterator/iterator.hpp"`

## Dependencies
- [`operations.md`](operations.md)
- [`reverse_iterator.md`](reverse_iterator.md)
- [`circular_iterator.md`](circular_iterator.md)
- [`fixed_iterator.md`](fixed_iterator.md)
- [`traits.md`](traits.md)

## Public API
| API | Description |
| --- | --- |
| `#include "castle/iterator/iterator.hpp"` | Re-exports Castle iterator operations, `reverse_iterator`, `circular_iterator`, `fixed_iterator`, and `iterator_traits` through one header. |

## Usage Example
See `samples/sample_iterator.cpp` for a complete standalone example.

```cpp
#include "castle/iterator/iterator.hpp"

uint8_t values[3U] = {4U, 5U, 6U};
auto second = castle::next(values, 1);
castle::reverse_iterator<uint8_t*> rit(values + 3U);
```

## Constraints & Notes
- This header adds no runtime behavior of its own; it only aggregates other iterator headers.
- Prefer including narrower headers when compile-time dependency minimization matters.
- Behavior, complexity, invalidation, and category requirements come from the re-exported components.
- No heap allocation, exceptions, RTTI, virtual dispatch, or STL iterator utilities are introduced by this umbrella include.
