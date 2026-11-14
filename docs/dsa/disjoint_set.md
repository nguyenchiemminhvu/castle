# disjoint_set

## Overview
Fixed-capacity disjoint-set union (union-find) over the dense node indices `0 .. size() - 1`. It tracks a partition of nodes into groups that merge over time. Typical uses: connected components, Kruskal's minimum spanning tree, network islands, and equivalence classes of ids. Nodes are plain indices, so they map one-to-one onto vertex indices of a graph container. `unite()` balances trees by rank and `find()` compresses paths, which gives amortized O(α(n)) per operation (see [cp-algorithms: DSU](https://cp-algorithms.com/data_structures/disjoint_set_union.html)).

## Header
`#include "castle/dsa/disjoint_set.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [error_handler](../core/error_handler.md)
- [traits](../core/traits.md) (`meta::smallest_uint_t`)
- [types](../core/types.md)
- [array_view](array_view.md)
- [status](../core/status.md)
- [swap](../utility/swap.md)

## Public API
### `disjoint_set<N>`
| API | Description |
| --- | --- |
| `disjoint_set<N>` | Union-find over up to `N` nodes (`N > 0`). |
| `index_type` | Smallest unsigned type holding `N` (`uint8_t` up to 255 nodes, `uint16_t` up to 65535, ...). Used for parent links, set sizes, and group members. |
| `rank_type` | `uint8_t`. A rank never exceeds `log2(N)`. |
| `static_capacity` | Compile-time capacity `N`. |
| `npos` | `size_type(-1)`. Returned by index queries when the node argument is out of range. |
| `explicit disjoint_set(size_type nodes = N)` | Creates `nodes` singleton groups. If `nodes > N`, asserts and clamps to `N`. O(N). |
| `status assign(size_type nodes)` | Replaces the partition with `nodes` singletons. Returns `status::out_of_range` and leaves the set unchanged when `nodes > N`. O(nodes). |
| `size()`, `capacity()`, `empty()` | Node count, `N`, and `size() == 0`. O(1). |
| `group_count()` | Number of disjoint groups. O(1). |
| `size_type find(node)` | Root (representative) of `node`'s group. Compresses the path. Amortized O(α(n)). |
| `bool unite(lhs, rhs)` | Merges the two groups, attaching the lower-rank root below the higher-rank one. Returns `true` only if two different groups were merged. Amortized O(α(n)). |
| `bool same_set(lhs, rhs)` | `true` when both nodes share a group. Amortized O(α(n)). |
| `size_type group_size(node)` | Number of nodes in `node`'s group. Amortized O(α(n)). |
| `void reset(node)` | Moves `node` back into its own group (own parent, size 1, rank 0). The other members stay together. O(size()). |
| `void reset()` | Returns every node to the starting state (singletons, rank 0) and keeps `size()`. O(size()). |
| `group_list groups()` | Snapshot of all groups. O(N + n·α(n)). |

### `disjoint_set<N>::group_list`
| API | Description |
| --- | --- |
| `value_type` | `array_view<const index_type>`, a view over the members of one group. |
| `size()`, `empty()` | Number of groups in the snapshot. |
| `operator[](group)` | Members of `group`, in ascending order. Returns an empty view when `group >= size()`. O(1). |
| `size_type group_of(node)` | Group index containing `node`, or `npos` when `node` is out of range. O(1). |
| `begin()`, `end()` | `const_iterator` over groups. Dereferencing yields `value_type`, so range-`for` works. |

Groups are numbered by their smallest member, so the order is deterministic and does not depend on which node ended up as root.

## Usage Example
```cpp
#include "castle/dsa/disjoint_set.hpp"

castle::dsa::disjoint_set<6U> net;
net.unite(0U, 1U);
net.unite(3U, 4U);
// net.group_count() == 4, net.same_set(0U, 1U) == true

for (auto members : net.groups()) // {0,1} {2} {3,4} {5}
{
    for (castle::size_type node : members)
    {
        // visit node
    }
}

net.reset(4U);   // {0,1} {2} {3} {4} {5}
net.reset();     // six singletons again
```

See `samples/dsa/disjoint_set.cpp` for Kruskal's MST, island detection, and node reset.

## Constraints & Notes
- No allocation, exceptions, RTTI, or virtual dispatch. The object stores `2 * N` `index_type` values, `N` ranks, and two counters stored as `index_type`. For example, `disjoint_set<200>` takes 600 bytes plus the counters.
- `find()` is iterative, so stack use is constant and there is no recursion.
- `find()`, `unite()`, `same_set()`, `group_size()`, and `groups()` mutate internal links (path compression), so they are non-const. The visible partition does not change.
- Group roots are implementation details: they can change after `unite()` or `reset(node)`. Compare groups with `same_set()`, or compare `find()` results taken at the same time.
- `reset(node)` does not know which edges made up the group. The remaining members stay in one group even if `node` was their only link. To rebuild from a changed edge list, call `reset()` and re-`unite()`.
- `reset(node)` costs O(size()) because a linear scan finds the nodes linked through `node`. When `node` is the root, its first child becomes the new root and inherits the rank, which keeps the rank bound valid.
- `group_list` is a by-value snapshot of `3 * N + 1` indices plus two `index_type` counters, about the same size as the set. `groups()` returns it through guaranteed copy elision. Views from `operator[]` alias the snapshot and must not outlive it.
- Only nodes `0 .. size() - 1` exist. The runtime node count set by the constructor or `assign()` is the limit, not the compile-time capacity `N`. Choose `N` as a generous upper bound and set the real count at runtime; reserved slots beyond `size()` never count as groups and are never touched.
- Node arguments `>= size()` are rejected in every build, regardless of `CASTLE_NO_CHECKS` or `NDEBUG`. The call returns `npos`, `false`, `0`, or an empty view, or for `reset(node)` does nothing, and leaves the state unchanged. A `false` from `unite()` or `same_set()` can therefore also mean an invalid index. Check `node < size()` first when that distinction matters.
- The type is trivially copyable, and copies are independent.
- Not thread-safe. Even read-style queries write links, so every access from multiple threads needs external synchronization.
