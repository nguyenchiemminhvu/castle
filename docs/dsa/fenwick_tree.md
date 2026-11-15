# fenwick_tree

## Overview
Fixed-capacity Fenwick tree, also called a binary indexed tree (BIT), over a sequence of additive values. It supports point updates, prefix sums, and inclusive range sums in O(log n), and stores exactly one `T` per element. Typical uses: frequency tables, running totals, inversion counting, order statistics (the k-th smallest), and counting range sums over compressed coordinates ([LeetCode 327, Count of Range Sum](https://leetcode.com/problems/count-of-range-sum/)). See [cp-algorithms: Fenwick tree](https://cp-algorithms.com/data_structures/fenwick.html).

A Fenwick tree needs an invertible operation, because `range_sum` is computed as `prefix(right + 1) - prefix(left)`. For minimum, maximum, gcd, non-commutative aggregates, or range updates combined with range queries, use [`segment_tree` / `lazy_segment_tree`](segment_tree.md).

## Header
`#include "castle/dsa/fenwick_tree.hpp"`

## Dependencies
- [bit_math](../bit/bit_math.md) (`bit::previous_power_of_two`)
- [bit_utils](../bit/bit_utils.md) (`bit::extract_lowest_set_bit`)
- [compiler](../core/compiler.md)
- [error_handler](../core/error_handler.md)
- [status](../core/status.md)
- [types](../core/types.md)
- [array](array.md)

## Public API
### `fenwick_tree<T, Capacity>`
| API | Description |
| --- | --- |
| `fenwick_tree<T, Capacity>` | Up to `Capacity` elements (`Capacity > 0`). `T()` must be zero, and `T` must support `+=`, `-`, and `<`. |
| `static_capacity` | `Capacity`. |
| `npos` | `size_type(-1)`. Returned by `lower_bound()` when no prefix reaches the target. |
| `explicit fenwick_tree(size_type count = 0)` | `count` zero elements. If `count > Capacity`, asserts and clamps. O(Capacity). |
| `status assign(count)` | Replaces the contents with `count` zeros. Returns `out_of_range` and leaves the tree unchanged when `count > Capacity`. O(count). |
| `status build(values, count)` | Replaces the contents with `values[0 .. count)` in O(count). Returns `out_of_range` or `invalid_argument` (null with `count > 0`), leaving the tree unchanged. |
| `status add(index, delta)` | Adds `delta` to element `index`. Returns `out_of_range` when `index >= size()`. O(log n). |
| `status set(index, value)` | Replaces element `index`, implemented as `add(index, value - get(index))`. O(log n). |
| `T get(index)` | Element value, or `T()` when `index >= size()`. O(log n). |
| `T prefix_sum(count)` | Sum of `[0, count)`. `count` may equal `size()`. Returns `T()` when `count > size()`. O(log n). |
| `T range_sum(left, right)` | Sum of the inclusive interval `[left, right]`. Returns `T()` when `left > right` or `right >= size()`. O(log n). |
| `size_type lower_bound(target)` | Smallest `index` with `prefix_sum(index + 1) >= target`, or `npos`. Requires non-negative elements. O(log n). |
| `clear()`, `size()`, `capacity()`, `empty()` | Remove all elements, the element count, `Capacity`, and `size() == 0`. |

## Usage Example
```cpp
#include "castle/dsa/fenwick_tree.hpp"

castle::dsa::fenwick_tree<int32_t, 8U> sums;
const int32_t values[] = {2, 3, 5, 7};
sums.build(values, 4U);
sums.add(1U, 10);          // {2, 13, 5, 7}
sums.range_sum(1U, 2U);    // 18
sums.set(3U, 0);           // {2, 13, 5, 0}

// Frequency table: the k-th smallest sample.
castle::dsa::fenwick_tree<uint16_t, 16U> histogram(16U);
histogram.add(7U, 1U);
histogram.add(3U, 1U);
histogram.add(9U, 1U);
histogram.lower_bound(2U); // 7: the median of {3, 7, 9}
```

`samples/dsa/fenwick_tree.cpp` solves Count of Range Sum. It builds 64-bit prefix sums `P`, sorts and deduplicates them with `castle::sort` / `castle::unique`, and walks `j` from left to right. For each `j` it counts earlier prefixes in `[P[j] - upper, P[j] - lower]` with two `prefix_sum` calls, then adds `P[j]` to the tree. The total cost is O(n log n). The sample also tracks a running median with `lower_bound()`.

## Constraints & Notes
- No allocation, exceptions, RTTI, or virtual dispatch. The object is `Capacity` values plus one `size_type`.
- **Layout.** Internally, node `i` (one-based) stores the sum of the `lowbit(i)` elements that end at element `i - 1`, where `lowbit(i) = i & -i` is the lowest set bit. A prefix sum walks `i -= lowbit(i)`, and an update walks `i += lowbit(i)`. Each walk visits at most `log2(n) + 1` nodes.
- `build()` is linear: each node adds its total to its parent `i + lowbit(i)` once.
- `lower_bound()` descends from the highest power of two not above `size()`. It needs no repeated prefix sums. Prefix sums must never decrease, so elements must be non-negative.
- **Range updates with point queries.** Store differences: to add `d` to `[l, r]`, call `add(l, d)` and, when `r + 1 < size()`, `add(r + 1, -d)`. Element `i` is then `prefix_sum(i + 1)`.
- Out-of-range arguments are rejected in every build, whatever `CASTLE_NO_CHECKS` or `NDEBUG` says. Mutators return `out_of_range`, and queries return `T()`. Check against `size()` first when a zero result must be told apart from an invalid request.
- Arithmetic overflow follows the rules of `T`. Choose a type wide enough for the largest prefix sum.
- Not thread-safe. Concurrent readers are safe only when no thread writes.
