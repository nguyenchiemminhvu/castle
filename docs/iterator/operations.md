# operations

## Overview
`castle::distance`, `castle::advance`, `castle::next`, and `castle::prev` provide small iterator utilities without depending on `<iterator>`. They exist so embedded code can move iterators and measure ranges using Castle iterator categories only.

## Header
`#include "castle/iterator/operations.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/types.md`](../core/types.md)
- [`traits.md`](traits.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename Iterator> difference_type distance(Iterator first, Iterator last)` | Returns the signed distance from `first` to `last`. Constant time for random-access iterators; linear for input/forward/bidirectional iterators. |
| `template <typename Iterator> void advance(Iterator& iterator, difference_type offset)` | Moves `iterator` in place by `offset`. Constant time for random-access iterators; linear otherwise. |
| `template <typename Iterator> Iterator next(Iterator iterator, difference_type offset = 1)` | Returns a moved copy of `iterator` after applying `advance`. |
| `template <typename Iterator> Iterator prev(Iterator iterator, difference_type offset = 1)` | Returns a copy moved backward by `offset`. Requires bidirectional or random-access capability for actual backward motion. |

## Usage Example
See `samples/sample_operations.cpp` for a complete standalone example.

```cpp
#include "castle/iterator/operations.hpp"

uint8_t values[5U] = {1U, 2U, 3U, 4U, 5U};
auto third = castle::next(values, 2);
auto count = castle::distance(values, values + 5U);
```

## Constraints & Notes
- Category dispatch comes from `castle::iterator_traits<Iterator>::iterator_category`.
- `distance` is linear for input, forward, and bidirectional iterators; it does not attempt to optimize beyond category guarantees.
- `advance` and `next` ignore negative offsets for input and forward iterators because only forward movement is available through that dispatch path.
- `prev` on input or forward iterators leaves the iterator unchanged for the same reason.
- Iterator invalidation and thread-safety are exactly those of the wrapped iterator and underlying storage.
- No heap allocation, exceptions, RTTI, virtual dispatch, or STL iterator utilities are used.
