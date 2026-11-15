# segment_tree

## Overview
Fixed-capacity segment trees for interval queries over a sequence. A segment tree keeps the aggregate of every aligned block of the sequence (pairs, quads, and so on), so any interval is the combination of O(log n) blocks. See [cp-algorithms: Segment tree](https://cp-algorithms.com/data_structures/segment_tree.html) and [Efficient and easy segment trees](https://codeforces.com/blog/entry/18051).

The aggregate is a **monoid** policy: a value type, an associative `combine()`, and a neutral `identity()`. Because the operation is pluggable and need not be invertible or commutative, a segment tree solves far more problems than a [Fenwick tree](fenwick_tree.md): sums, minimum, maximum, gcd, bitwise and/or, products of matrices or affine maps, "best subarray sum", counts of maxima, and more.

| Class | Updates | Queries |
| --- | --- | --- |
| `segment_tree<Monoid, Capacity>` | point assignment | inclusive range aggregate, `max_right` / `min_left` binary search (`const`) |
| `lazy_segment_tree<Policy, Capacity>` | point assignment and **range updates** (add, assign, or custom) | the same queries; pending updates are pushed down lazily |

[LeetCode 307, Range Sum Query - Mutable](https://leetcode.com/problems/range-sum-query-mutable/) is `segment_tree<sum_monoid<int32_t>, 30000>` with `build`, `set`, and `query`.

## Header
`#include "castle/dsa/segment_tree.hpp"`

## Dependencies
- [bit_math](../bit/bit_math.md) (`bit::next_power_of_two`, `bit::next_power_of_two_const`)
- [compiler](../core/compiler.md)
- [status](../core/status.md)
- [type_ranges](../core/type_ranges.md) (`castle::numeric_limits` for the min/max identities)
- [types](../core/types.md)
- [array](array.md)

## Public API
### Monoids
| API | `identity()` | `combine(a, b)` |
| --- | --- | --- |
| `sum_monoid<T>` | `T()` | `a + b` |
| `min_monoid<T>` | `numeric_limits<T>::max()` | `min(a, b)` |
| `max_monoid<T>` | `numeric_limits<T>::lowest()` | `max(a, b)` |
| Custom monoid | Any type with `value_type`, `static value_type identity()`, and `static value_type combine(const value_type&, const value_type&)`. `combine` must be associative and `identity` neutral. Operands are always combined left to right. | |

### Lazy policies
A lazy policy is a monoid plus a tag (a pending update):

| Member | Meaning |
| --- | --- |
| `tag_type` | Update type. |
| `tag_identity()` | The no-op update. |
| `apply(tag, value, length)` | The aggregate of `length` elements after `tag` is applied to each of them. |
| `compose(newer, older)` | A single tag equal to applying `older`, then `newer`. |

`apply` must distribute over `combine`.

| Policy | Update | Aggregate |
| --- | --- | --- |
| `range_add_sum<T>` | add `v` | sum (`value + v * length`) |
| `range_add_min<T>`, `range_add_max<T>` | add `v` | min / max (`value + v`) |
| `range_assign_sum<T>` | assign `v` | sum (`v * length`) |
| `range_assign_min<T>`, `range_assign_max<T>` | assign `v` | min / max (`v`) |
| `assign_tag<T>` | Tag of the `range_assign_*` policies. A default-constructed tag is inactive. Converts implicitly from `T`, so `apply(l, r, 7)` assigns 7. | |

### `segment_tree<Monoid, Capacity>`
| API | Description |
| --- | --- |
| `value_type`, `monoid_type` | `Monoid::value_type` and `Monoid`. |
| `static_capacity`, `static_leaf_capacity` | `Capacity` and `next_power_of_two(Capacity)`. |
| `npos` | `size_type(-1)`. Returned by the binary searches for an out-of-range bound. |
| `segment_tree()` | Empty tree. O(Capacity) to value-initialize storage. |
| `status build(values, count)` | Replaces the contents with `values[0 .. count)`. Returns `out_of_range` or `invalid_argument` (null with `count > 0`), leaving the tree unchanged. O(count). |
| `status assign(count, value = identity())` | Replaces the contents with `count` copies of `value`. O(count). |
| `status set(index, value)` | Replaces one element. Returns `out_of_range` when `index >= size()`. O(log n). |
| `value_type get(index)` | Element value, or `identity()` when out of range. O(1). |
| `value_type query(left, right)` | Aggregate of the inclusive interval `[left, right]`, or `identity()` when `left > right` or `right >= size()`. O(log n). |
| `value_type all()` | Aggregate of every element. O(1). |
| `size_type max_right(left, pred)` | Largest `end` in `[left, size()]` with `pred(aggregate of [left, end))` true: the first index whose inclusion breaks `pred`. Returns `npos` when `left > size()`. O(log n). |
| `size_type min_left(end, pred)` | Smallest `begin` in `[0, end]` with `pred(aggregate of [begin, end))` true. Returns `npos` when `end > size()`. O(log n). |
| `clear()`, `size()`, `capacity()`, `empty()` | Standard. |

`pred` must hold for `identity()`, and it must be monotone: once false for an interval, it stays false for every longer interval.

### `lazy_segment_tree<Policy, Capacity>`
The same API as `segment_tree`, with `tag_type` / `policy_type` and:

| API | Description |
| --- | --- |
| `status apply(left, right, tag)` | Applies `tag` to every element of `[left, right]`. Returns `out_of_range` when `left > right` or `right >= size()`. O(log n). |
| `get`, `query`, `max_right`, `min_left` | O(log n). Non-`const`, because they push pending tags toward the leaves they visit. |
| `build`, `assign` | Also drop all pending tags. |

## Usage Example
```cpp
#include "castle/dsa/segment_tree.hpp"

// LeetCode 307.
castle::dsa::segment_tree<castle::dsa::sum_monoid<int32_t>, 30000U> sums;
const int32_t nums[] = {1, 3, 5};
sums.build(nums, 3U);
sums.query(0U, 2U);     // 9
sums.set(1U, 2);
sums.query(0U, 2U);     // 8

// How many leading packets fit 1500 bytes?
castle::dsa::segment_tree<castle::dsa::sum_monoid<uint16_t>, 8U> queue;
const uint16_t bytes[] = {400U, 300U, 500U, 200U, 600U};
queue.build(bytes, 5U);
queue.max_right(0U, [](uint16_t total) { return total <= 1500U; }); // 4

// Range add, range max (LeetCode 732, My Calendar III).
castle::dsa::lazy_segment_tree<castle::dsa::range_add_max<int32_t>, 64U> calendar;
calendar.assign(60U, 0);
calendar.apply(10U, 19U, 1);   // booking [10, 20)
calendar.apply(5U, 14U, 1);    // booking [5, 15)
calendar.all();                // 2 overlapping bookings
```

`samples/dsa/segment_tree.cpp` contains:
- LeetCode 307 as a `num_array` class.
- A custom non-commutative monoid for maximum subarray sum with updates.
- `max_right` / `min_left` over packet sizes.
- LeetCode 1109 (Corporate Flight Bookings) with `range_add_sum`.
- LeetCode 732 with `range_add_max`.
- An LED strip with `range_assign_min`.

## Constraints & Notes
- No allocation, exceptions, RTTI, virtual dispatch, or recursion.
- **Storage.** `segment_tree` holds `2 * static_leaf_capacity` values. `lazy_segment_tree` additionally holds `static_leaf_capacity` tags. With `Capacity = 30000`, the leaf capacity is 32768, so a `sum_monoid<int32_t>` tree takes 256 KiB. Place large trees in static storage.
- **Layout.** The trees are perfect binary trees in an array: node `k` has children `2k` and `2k + 1`, and leaves start at `base = next_power_of_two(size())`. Unused leaves hold `identity()`. Because `base` follows the runtime `size()`, a small tree in a large `Capacity` costs O(log size()) per operation and O(size()) per `build()` / `assign()`.
- **Order.** Queries accumulate a left and a right partial result and combine them as `left`, then `right`. Non-commutative monoids such as matrix products, affine maps, or string hashes therefore work. The perfect layout also makes every node an aligned interval, which `max_right` / `min_left` rely on.
- **Lazy propagation.** `apply(l, r, tag)` updates the O(log n) blocks covering `[l, r]` and records `tag` on internal blocks. A pending tag is pushed to the children only when a later operation passes through that block. The policy's `apply` receives the block length, so sums scale correctly. Padding leaves are never inside an applied interval, so they keep `identity()`.
- Out-of-range arguments are rejected in every build. Mutators return `out_of_range`, `get()` / `query()` return `identity()`, and the binary searches return `npos`.
- `min_monoid` / `max_monoid` need a `castle::numeric_limits<T>` specialization. For other types, write a monoid with your own sentinel.
- Arithmetic overflow follows the rules of `value_type`. `range_add_sum` multiplies the tag by the block length, so size the type for `tag * size()`.
- Not thread-safe. Lazy queries write tags, so even reads need external synchronization.
