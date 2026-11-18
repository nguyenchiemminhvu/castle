# fixed_iterator

## Overview
`castle::fixed_iterator` wraps an iterator or pointer together with explicit first/last bounds and prevents movement outside that half-open range. It exists for fixed-capacity embedded data structures that need iterator-style traversal with deterministic boundary clamping.

## Header
`#include "castle/iterator/fixed_iterator.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/types.md`](../core/types.md)
- [`traits.md`](traits.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename Iterator> class fixed_iterator` | Stores `first`, `current`, and `last` iterators for a bounded `[first, last)` range. |
| `fixed_iterator()` | Value-initializes the stored iterators. |
| `fixed_iterator(Iterator first, Iterator current, Iterator last)` | Configures the bounded range and current position. |
| `Iterator base() const` | Returns the wrapped iterator currently stored by the adaptor. |
| `bool valid() const` | Returns `true` when `current != last`, which means dereference is valid. Constant time. |
| `reference operator*() const` | Dereferences the current element. Constant time. |
| `pointer operator->() const` | Returns the current element pointer. Constant time. |
| `fixed_iterator& operator++()` / `fixed_iterator operator++(int)` | Advances toward `last` and stops at the stored end sentinel. Constant time. |
| `fixed_iterator& operator--()` / `fixed_iterator operator--(int)` | Moves toward `first` and stops at the stored first element. Constant time; requires decrement support in the underlying iterator. |
| `bool operator==(const fixed_iterator& other) const` | Compares only the current wrapped iterator position. |
| `bool operator!=(const fixed_iterator& other) const` | Negation of equality. |

## Usage Example
See `samples/sample_fixed_iterator.cpp` for a complete standalone example.

```cpp
#include "castle/iterator/fixed_iterator.hpp"

uint8_t values[4U] = {1U, 2U, 3U, 4U};
castle::fixed_iterator<uint8_t*> it(values, values, values + 4U);

while (it.valid())
{
    ++it;
}
```

## Constraints & Notes
- Dereference is valid only while `valid()` returns `true`.
- Increment never moves past `last`; decrement never moves before `first`.
- A `current == last` end sentinel can be decremented back into the valid range when the underlying iterator supports `--`.
- Iterator invalidation, lifetime, and thread-safety are exactly those of the wrapped iterator and underlying storage.
- Equality and inequality compare only the current iterator position; two adaptors with different bounds can compare equal if `base()` matches.
- No heap allocation, exceptions, RTTI, virtual dispatch, or STL iterator utilities are used.
