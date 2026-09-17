# circular_iterator

## Overview
`castle::circular_iterator` wraps an existing iterator or pointer and makes traversal repeat over a fixed half-open range. It is useful for ring-style scans, round-robin tables, and other deterministic embedded loops that must wrap without modulus arithmetic or dynamic allocation.

## Header
`#include "castle/iterator/circular_iterator.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`traits.md`](traits.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename Iterator> class circular_iterator` | Stores `first`, `last`, and `current` iterators for a circular `[first, last)` range. |
| `circular_iterator()` | Value-initializes the stored iterators. |
| `circular_iterator(Iterator first, Iterator last, Iterator current)` | Configures the circular range and current position. |
| `Iterator base() const` | Returns the wrapped iterator currently stored by the adaptor. |
| `reference operator*() const` | Dereferences the current element. Constant time. |
| `pointer operator->() const` | Returns the current element pointer. Constant time. |
| `circular_iterator& operator++()` / `circular_iterator operator++(int)` | Advances and wraps from `last` back to `first`. Constant time. |
| `circular_iterator& operator--()` / `circular_iterator operator--(int)` | Moves backward and wraps from `first` to the element before `last`. Constant time; requires decrement support in the underlying iterator. |
| `bool operator==(const circular_iterator& other) const` | Compares only the current wrapped iterator position. |
| `bool operator!=(const circular_iterator& other) const` | Negation of equality. |

## Usage Example
See `samples/sample_circular_iterator.cpp` for a complete standalone example.

```cpp
#include "castle/iterator/circular_iterator.hpp"

uint8_t values[3U] = {10U, 20U, 30U};
castle::circular_iterator<uint8_t*> it(values, values + 3U, values + 2U);

++it; // wraps to values[0]
```

## Constraints & Notes
- The stored iterators must describe the same valid half-open range.
- `first == last` represents an empty range; increment and decrement become no-ops, but dereference is invalid.
- Iterator invalidation, lifetime, and thread-safety are exactly those of the wrapped iterator and underlying storage.
- Equality and inequality compare only the current iterator position; two adaptors with different bounds can compare equal if `base()` matches.
- No heap allocation, exceptions, RTTI, virtual dispatch, or STL iterator utilities are used.
