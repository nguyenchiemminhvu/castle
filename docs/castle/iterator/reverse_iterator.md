# reverse_iterator

## Overview
`castle::reverse_iterator` adapts an existing iterator or pointer so traversal runs from back to front. It exists for deterministic reverse scans in containers and fixed buffers without pulling in the standard library iterator adaptor.

## Header
`#include "castle/iterator/reverse_iterator.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/types.md`](../core/types.md)
- [`traits.md`](traits.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename Iterator> class reverse_iterator` | Stores a base iterator whose predecessor is the current reverse element. |
| `reverse_iterator()` | Value-initializes the stored base iterator. |
| `explicit reverse_iterator(Iterator current)` | Stores `current` as the base iterator. Dereference reads the element immediately before it. |
| `template <typename OtherIterator> reverse_iterator(const reverse_iterator<OtherIterator>& other)` | Converts from another compatible reverse iterator type. |
| `Iterator base() const` | Returns the stored base iterator. Constant time. |
| `reference operator*() const` / `pointer operator->() const` | Accesses the element preceding `base()`. Constant time; requires safe decrement of the base iterator. |
| `reverse_iterator& operator++()` / `reverse_iterator operator++(int)` | Advances in reverse order by decrementing the base iterator. Constant time; requires decrement support. |
| `reverse_iterator& operator--()` / `reverse_iterator operator--(int)` | Moves in the opposite direction by incrementing the base iterator. Constant time; requires increment support. |
| `operator+(difference_type)` / `operator-(difference_type)` / `operator+=` / `operator-=` / `operator[]` | Random-access reverse navigation helpers. Constant time; require random-access underlying iterators. |
| `==`, `!=`, `<`, `>`, `<=`, `>=`, `offset + iterator`, `lhs - rhs` | Non-member comparisons and arithmetic defined in terms of the stored base iterators. |

## Usage Example
See `samples/sample_reverse_iterator.cpp` for a complete standalone example.

```cpp
#include "castle/iterator/reverse_iterator.hpp"

uint8_t values[3U] = {1U, 2U, 3U};
castle::reverse_iterator<uint8_t*> rit(values + 3U);
uint8_t newest = *rit;
```

## Constraints & Notes
- `base()` follows the conventional reverse-iterator rule: it points one element past the element returned by `operator*()`.
- Dereference is only valid when the stored base iterator can be decremented safely.
- Random-access arithmetic and ordering comparisons require the corresponding operations on the underlying iterator type.
- Iterator invalidation, lifetime, and thread-safety are exactly those of the wrapped iterator and underlying storage.
- No heap allocation, exceptions, RTTI, virtual dispatch, or STL iterator utilities are used.
