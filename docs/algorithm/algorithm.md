# Algorithm

## Overview
Deterministic, STL-free generic algorithms for Castle iterators and raw pointers. Use this header when you need common search, copy, transform, remove, sort, and ordered-lookup operations without heap allocation or recursion-heavy implementations.

## Header
`#include "castle/algorithm/algorithm.hpp"`

## Dependencies
- [`iterator/iterator.hpp`](../iterator/iterator.md)
- `castle/utility/move.hpp`
- `castle/utility/pair.hpp`
- `castle/utility/swap.hpp`

## Public API
| API | Description |
| --- | --- |
| `find`, `find_if`, `find_if_not` | Linear search over an input range; return the first matching iterator or `last`. |
| `count`, `count_if` | Count matching elements in an input range; `O(N)`. |
| `all_of`, `any_of`, `none_of` | Predicate checks over an input range with short-circuit behavior. |
| `equal` | Sequence equality checks for 3-iterator and 4-iterator forms, with optional predicate overloads. |
| `mismatch` | Returns `castle::pair` of the first mismatching iterators, with optional predicate overloads. |
| `copy`, `copy_n`, `copy_if`, `move` | Copy or move ranges into an output iterator; no allocation. |
| `fill`, `fill_n`, `transform`, `generate` | Overwrite or generate values in-place or into an output range. |
| `replace`, `replace_if` | In-place replacement on forward ranges. |
| `remove`, `remove_if`, `unique` | Non-erasing algorithms that compact the logical range and return the new end iterator. |
| `sort` | In-place heapsort for random-access iterators; deterministic `O(N log N)` worst case and `O(1)` extra storage. |
| `lower_bound`, `upper_bound`, `binary_search` | Ordered-range lookup helpers for forward iterators. |
| `min`, `max`, `min3`, `max3` | Value comparisons without STL dependency. |
| `min_element`, `max_element` | Find the smallest or largest element in a forward range. |

## Usage Example
```cpp
// See: samples/sample_algorithm.cpp
#include "castle/algorithm/algorithm.hpp"

int values[6] = {4, 1, 7, 1, 9, 2};
castle::sort(values, values + 6);

int* found = castle::find(values, values + 6, 7);
bool has_nine = castle::binary_search(values, values + 6, 9);

(void)found;
(void)has_nine;
```

## Constraints & Notes
- No heap allocation, exceptions, RTTI, or STL algorithms.
- Iterator categories are checked with `static_assert`; unsupported iterator types fail at compile time.
- `sort()` uses iterative heapsort, not quicksort or mergesort.
- `remove`, `remove_if`, and `unique` do not resize containers; callers must treat the returned iterator as the new logical end.
- Ordered lookup assumes the range is already sorted with the same comparator.
