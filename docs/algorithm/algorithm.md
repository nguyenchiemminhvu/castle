# Castle algorithm

## Overview

`castle/algorithm/algorithm.h` provides the core sequence, lookup, sorting, and min/max algorithms used by Castle 2.0.

The API is intentionally close to the familiar C++ standard algorithm model, but it is implemented specifically for Castle's embedded requirements:

- no dependency on `<algorithm>` or other STL headers,
- no dynamic allocation,
- no virtual functions or RTTI,
- no exceptions are required,
- algorithms operate directly on Castle iterators,
- iterator requirements are checked at compile time,
- execution is deterministic for the provided algorithms.

The implementation is header-only and intended for C++11 through C++17 toolchains.

## Header

```cpp
#include "castle/algorithm/algorithm.h"
```

Namespace: `castle`

The algorithms use Castle's iterator tags and traits from:

```cpp
#include "castle/iterator/iterator.h"
```

Users normally only need to include `algorithm.h`; its required Castle dependencies are included by the header itself.

## Main API

### Non-modifying sequence operations

| Function | Purpose | Iterator requirement |
| --- | --- | --- |
| `find` | Find the first element equal to a value | Input |
| `find_if` | Find the first element satisfying a predicate | Input |
| `find_if_not` | Find the first element not satisfying a predicate | Input |
| `count` | Count elements equal to a value | Input |
| `count_if` | Count elements satisfying a predicate | Input |
| `all_of` | Test whether all elements satisfy a predicate | Input |
| `any_of` | Test whether any element satisfies a predicate | Input |
| `none_of` | Test whether no element satisfies a predicate | Input |
| `equal` | Compare two sequences for equality | Input |
| `mismatch` | Find the first pair of elements that differs | Input |

### Modifying sequence operations

| Function | Purpose | Iterator requirement |
| --- | --- | --- |
| `copy` | Copy a sequence to an output range | Input + Output |
| `copy_n` | Copy a fixed number of elements | Input + Output |
| `copy_if` | Copy elements satisfying a predicate | Input + Output |
| `move` | Move a sequence to an output range | Input + Output |
| `fill` | Assign one value to every element | Forward |
| `fill_n` | Assign one value to a fixed number of elements | Forward |
| `transform` | Apply a unary or binary operation | Input + Output |
| `generate` | Generate values with a callable | Forward |
| `replace` | Replace elements equal to a value | Forward |
| `replace_if` | Replace elements satisfying a predicate | Forward |
| `remove` | Move unwanted values to the end of a range | Forward |
| `remove_if` | Move predicate-matching elements to the end | Forward |
| `unique` | Remove consecutive duplicate values | Forward |

`remove`, `remove_if`, and `unique` are non-erasing algorithms. They rearrange the range and return the new logical end. The container itself is responsible for changing its size when that operation is supported.

### Sorting and ordered lookup

| Function | Purpose | Iterator requirement |
| --- | --- | --- |
| `sort` | Sort a range in ascending/custom order | Random access |
| `lower_bound` | Find the first position not less than a value | Forward |
| `upper_bound` | Find the first position greater than a value | Forward |
| `binary_search` | Test whether an ordered range contains a value | Forward |

`sort` uses an iterative in-place heapsort. This gives `O(N log N)` worst-case time and constant auxiliary storage without recursion.

### Min/max operations

| Function | Purpose | Iterator requirement |
| --- | --- | --- |
| `min` | Return the smaller of two values | — |
| `max` | Return the larger of two values | — |
| `min_element` | Find the smallest element in a range | Forward |
| `max_element` | Find the largest element in a range | Forward |

Most comparison-based algorithms provide both a default ordering and a custom comparator overload.

## Example

### Basic search and count

```cpp
#include "castle/algorithm/algorithm.h"
#include "castle/container/array.h"

castle::array<int, 8> values = { 4, 1, 7, 1, 9, 2, 1, 5 };

castle::array<int, 8>::iterator first_one =
    castle::find(values.begin(), values.end(), 1);

castle::size_type ones =
    castle::count(values.begin(), values.end(), 1);

bool has_large_value =
    castle::any_of(values.begin(), values.end(),
                   [](int value) { return value > 8; });
```

### Sorting and binary search

```cpp
#include "castle/algorithm/algorithm.h"
#include "castle/container/array.h"

castle::array<int, 8> values = { 9, 2, 7, 1, 5, 4, 8, 3 };

castle::sort(values.begin(), values.end());

bool found = castle::binary_search(values.begin(), values.end(), 5);

castle::array<int, 8>::iterator position =
    castle::lower_bound(values.begin(), values.end(), 6);
```

After sorting, `values` contains:

```text
1 2 3 4 5 7 8 9
```

`lower_bound` returns the first position whose value is greater than or equal to `6`, which is the element `7` in this example.

### Custom ordering

```cpp
castle::sort(values.begin(), values.end(),
             [](int lhs, int rhs) { return lhs > rhs; });
```

The comparator follows the usual Castle/standard-library convention: it returns `true` when the first argument should appear before the second argument.

### Remove without dynamic allocation

```cpp
castle::array<int, 8> values = { 1, 2, 2, 3, 2, 4, 2, 5 };

castle::array<int, 8>::iterator new_end =
    castle::remove(values.begin(), values.end(), 2);

for (castle::array<int, 8>::iterator it = values.begin();
     it != new_end;
     ++it)
{
    // Use the remaining logical sequence.
}
```

The algorithm does not resize or destroy the container. `new_end` defines the resulting logical range.

### Transform

```cpp
castle::array<int, 4> input = { 1, 2, 3, 4 };
castle::array<int, 4> output = {};

castle::transform(input.begin(), input.end(), output.begin(),
                  [](int value) { return value * value; });
```

`output` becomes:

```text
1 4 9 16
```

## Complexity

Castle algorithms are designed so their normal complexity is easy to reason about in embedded code.

| Algorithm group | Typical time complexity | Auxiliary storage |
| --- | --- | --- |
| `find`, `find_if` | `O(N)` | `O(1)` |
| `count`, `count_if` | `O(N)` | `O(1)` |
| `all_of`, `any_of`, `none_of` | `O(N)` worst case | `O(1)` |
| `equal`, `mismatch` | `O(N)` | `O(1)` |
| `copy`, `copy_n`, `copy_if` | `O(N)` | `O(1)` |
| `fill`, `fill_n` | `O(N)` | `O(1)` |
| `transform` | `O(N)` | `O(1)` |
| `generate` | `O(N)` | `O(1)` |
| `replace`, `replace_if` | `O(N)` | `O(1)` |
| `remove`, `remove_if` | `O(N)` | `O(1)` |
| `unique` | `O(N)` | `O(1)` |
| `sort` | `O(N log N)` worst case | `O(1)` |
| `lower_bound` | `O(log N)` comparisons | `O(1)` auxiliary storage |
| `upper_bound` | `O(log N)` comparisons | `O(1)` auxiliary storage |
| `binary_search` | `O(log N)` comparisons | `O(1)` auxiliary storage |
| `min_element`, `max_element` | `O(N)` | `O(1)` |

For forward iterators, physical iterator movement during binary search may be linear even though the number of comparisons is logarithmic. Random-access iterators provide the expected efficient positional movement.

## Iterator requirements

Castle algorithms use the iterator category system provided by `castle/iterator/iterator.h` rather than `std::iterator_traits`.

The implementation checks category compatibility using `static_assert`. This makes unsupported calls fail during compilation instead of introducing runtime checks.

The important requirements are:

```text
InputIterator
    find, count, predicates, equal, mismatch, copy, transform, move

ForwardIterator
    fill, generate, replace, remove, unique,
    lower_bound, upper_bound, binary_search,
    min_element, max_element

RandomAccessIterator
    sort
```

A Castle random-access iterator also satisfies the weaker iterator categories through Castle's iterator-tag hierarchy.

## Embedded notes

### No allocation

The algorithms never allocate memory. `sort` is specifically implemented without recursion or an auxiliary array, so its auxiliary storage remains constant.

This is important for Castle's fixed-memory model: an algorithm call does not introduce hidden heap usage.

### Deterministic sorting

`castle::sort` uses iterative heapsort rather than recursive quicksort.

The design intentionally favors:

```text
bounded stack usage
O(N log N) worst-case execution
in-place operation
no heap allocation
```

over the potentially better average-case behavior of quicksort.

### Predicates and comparators

Predicates and comparators are passed by value. They should therefore be lightweight, deterministic, and free of hidden ownership or allocation.

For embedded code, prefer small stateless functors or lambdas.

### `noexcept`

Castle algorithms are declared with Castle's `CASTLE_NOEXCEPT` contract where applicable. This does not make an arbitrary user predicate logically non-throwing; user-supplied operations should themselves obey the project's no-exception policy.

### `remove` and `unique` do not resize containers

These algorithms follow the standard algorithm model rather than container semantics. They return an iterator representing the new logical end:

```cpp
iterator new_end = castle::remove(first, last, value);
```

No memory is allocated and no container capacity is changed.

### Ordered lookup requires an ordered range

`lower_bound`, `upper_bound`, and `binary_search` assume that the input range is already sorted according to the same ordering used by the comparator.

For example:

```cpp
castle::sort(values.begin(), values.end());

bool found = castle::binary_search(values.begin(), values.end(), value);
```

Calling binary-search algorithms on an unsorted range does not perform an automatic sort and produces no runtime diagnostic.

### Empty ranges

Algorithms are designed to handle empty `[first, last)` ranges where their normal semantics permit it. Element-returning algorithms such as `min_element` and `max_element` return `last` for an empty range.

Value-based `min` and `max` operate on their two supplied values and therefore always have a result.

### STL independence

Do not add:

```cpp
#include <algorithm>
#include <iterator>
#include <utility>
#include <type_traits>
```

to a Castle translation unit just to use these algorithms. `castle/algorithm/algorithm.h` is intended to provide the required functionality using Castle's own iterator, utility, trait, and type infrastructure.

### Recommended usage in embedded code

For safety-oriented firmware, prefer:

1. fixed-capacity Castle containers,
2. explicit iterator ranges,
3. small stateless predicates/comparators,
4. prevalidated sorted ranges for binary search,
5. `sort` when a bounded worst-case `O(N log N)` sort is required,
6. explicit handling of returned iterators from `remove`/`unique`,
7. compile-time iterator-category checking rather than runtime dispatch.

The algorithm subsystem is intended to remain a small generic layer between Castle iterators and Castle containers, consistent with the project's deterministic, heap-free architecture.
